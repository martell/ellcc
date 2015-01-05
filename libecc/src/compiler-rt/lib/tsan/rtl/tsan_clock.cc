//===-- tsan_clock.cc -----------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file is a part of ThreadSanitizer (TSan), a race detector.
//
//===----------------------------------------------------------------------===//
#include "tsan_clock.h"
#include "tsan_rtl.h"
#include "sanitizer_common/sanitizer_placement_new.h"

// It's possible to optimize clock operations for some important cases
// so that they are O(1). The cases include singletons, once's, local mutexes.
// First, SyncClock must be re-implemented to allow indexing by tid.
// It must not necessarily be a full vector clock, though. For example it may
// be a multi-level table.
// Then, each slot in SyncClock must contain a dirty bit (it's united with
// the clock value, so no space increase). The acquire algorithm looks
// as follows:
// void acquire(thr, tid, thr_clock, sync_clock) {
//   if (!sync_clock[tid].dirty)
//     return;  // No new info to acquire.
//              // This handles constant reads of singleton pointers and
//              // stop-flags.
//   acquire_impl(thr_clock, sync_clock);  // As usual, O(N).
//   sync_clock[tid].dirty = false;
//   sync_clock.dirty_count--;
// }
// The release operation looks as follows:
// void release(thr, tid, thr_clock, sync_clock) {
//   // thr->sync_cache is a simple fixed-size hash-based cache that holds
//   // several previous sync_clock's.
//   if (thr->sync_cache[sync_clock] >= thr->last_acquire_epoch) {
//     // The thread did no acquire operations since last release on this clock.
//     // So update only the thread's slot (other slots can't possibly change).
//     sync_clock[tid].clock = thr->epoch;
//     if (sync_clock.dirty_count == sync_clock.cnt
//         || (sync_clock.dirty_count == sync_clock.cnt - 1
//           && sync_clock[tid].dirty == false))
//       // All dirty flags are set, bail out.
//       return;
//     set all dirty bits, but preserve the thread's bit.  // O(N)
//     update sync_clock.dirty_count;
//     return;
//   }
//   release_impl(thr_clock, sync_clock);  // As usual, O(N).
//   set all dirty bits, but preserve the thread's bit.
//   // The previous step is combined with release_impl(), so that
//   // we scan the arrays only once.
//   update sync_clock.dirty_count;
// }

// We don't have ThreadState in these methods, so this is an ugly hack that
// works only in C++.
#ifndef SANITIZER_GO
# define CPP_STAT_INC(typ) StatInc(cur_thread(), typ)
#else
# define CPP_STAT_INC(typ) (void)0
#endif

namespace __tsan {

ThreadClock::ThreadClock() {
  nclk_ = 0;
  for (uptr i = 0; i < (uptr)kMaxTidInClock; i++)
    clk_[i] = 0;
}

void ThreadClock::acquire(ClockCache *c, const SyncClock *src) {
  DCHECK_LE(nclk_, kMaxTid);
  DCHECK_LE(src->size_, kMaxTid);

  const uptr nclk = src->size_;
  if (nclk == 0)
    return;
  }

  // Check if we've already acquired src after the last release operation on src
  bool acquired = false;
  if (nclk > tid_) {
    CPP_STAT_INC(StatClockAcquireLarge);
    if (src->elem(tid_).reused == reused_) {
      CPP_STAT_INC(StatClockAcquireRepeat);
      for (unsigned i = 0; i < kDirtyTids; i++) {
        unsigned tid = src->dirty_tids_[i];
        if (tid != kInvalidTid) {
          u64 epoch = src->elem(tid).epoch;
          if (clk_[tid].epoch < epoch) {
            clk_[tid].epoch = epoch;
            acquired = true;
          }
        }
      }
      if (acquired) {
        CPP_STAT_INC(StatClockAcquiredSomething);
        last_acquire_ = clk_[tid_].epoch;
      }
      return;
    }
  }

  // O(N) acquire.
  CPP_STAT_INC(StatClockAcquireFull);
  nclk_ = max(nclk_, nclk);
  for (uptr i = 0; i < nclk; i++) {
    u64 epoch = src->elem(i).epoch;
    if (clk_[i].epoch < epoch) {
      clk_[i].epoch = epoch;
      acquired = true;
    }
  }

  // Remember that this thread has acquired this clock.
  if (nclk > tid_)
    src->elem(tid_).reused = reused_;

  if (acquired) {
    CPP_STAT_INC(StatClockAcquiredSomething);
    last_acquire_ = clk_[tid_].epoch;
  }
}

void ThreadClock::release(ClockCache *c, SyncClock *dst) const {
  DCHECK_LE(nclk_, kMaxTid);
  DCHECK_LE(dst->size_, kMaxTid);

  if (dst->size_ == 0) {
    // ReleaseStore will correctly set release_store_tid_,
    // which can be important for future operations.
    ReleaseStore(c, dst);
    return;
  }

  CPP_STAT_INC(StatClockRelease);
  // Check if we need to resize dst.
  if (dst->size_ < nclk_)
    dst->Resize(c, nclk_);

  // Check if we had not acquired anything from other threads
  // since the last release on dst. If so, we need to update
  // only dst->elem(tid_).
  if (dst->elem(tid_).epoch > last_acquire_) {
    UpdateCurrentThread(dst);
    if (dst->release_store_tid_ != tid_ ||
        dst->release_store_reused_ != reused_)
      dst->release_store_tid_ = kInvalidTid;
    return;
  }

  // O(N) release.
  CPP_STAT_INC(StatClockReleaseFull);
  // First, remember whether we've acquired dst.
  bool acquired = IsAlreadyAcquired(dst);
  if (acquired)
    CPP_STAT_INC(StatClockReleaseAcquired);
  // Update dst->clk_.
  for (uptr i = 0; i < nclk_; i++) {
    ClockElem &ce = dst->elem(i);
    ce.epoch = max(ce.epoch, clk_[i].epoch);
    ce.reused = 0;
  }
  // Clear 'acquired' flag in the remaining elements.
  if (nclk_ < dst->size_)
    CPP_STAT_INC(StatClockReleaseClearTail);
  for (uptr i = nclk_; i < dst->size_; i++)
    dst->elem(i).reused = 0;
  for (unsigned i = 0; i < kDirtyTids; i++)
    dst->dirty_tids_[i] = kInvalidTid;
  dst->release_store_tid_ = kInvalidTid;
  dst->release_store_reused_ = 0;
  // If we've acquired dst, remember this fact,
  // so that we don't need to acquire it on next acquire.
  if (acquired)
    dst->elem(tid_).reused = reused_;
}

void ThreadClock::ReleaseStore(ClockCache *c, SyncClock *dst) const {
  DCHECK_LE(nclk_, kMaxTid);
  DCHECK_LE(dst->size_, kMaxTid);

  // Check if we need to resize dst.
  if (dst->size_ < nclk_)
    dst->Resize(c, nclk_);

  if (dst->release_store_tid_ == tid_ &&
      dst->release_store_reused_ == reused_ &&
      dst->elem(tid_).epoch > last_acquire_) {
    CPP_STAT_INC(StatClockStoreFast);
    UpdateCurrentThread(dst);
    return;
  }

  // O(N) release-store.
  CPP_STAT_INC(StatClockStoreFull);
  for (uptr i = 0; i < nclk_; i++) {
    ClockElem &ce = dst->elem(i);
    ce.epoch = clk_[i].epoch;
    ce.reused = 0;
  }
  // Clear the tail of dst->clk_.
  if (nclk_ < dst->size_) {
    for (uptr i = nclk_; i < dst->size_; i++) {
      ClockElem &ce = dst->elem(i);
      ce.epoch = 0;
      ce.reused = 0;
    }
    CPP_STAT_INC(StatClockStoreTail);
  }
  for (unsigned i = 0; i < kDirtyTids; i++)
    dst->dirty_tids_[i] = kInvalidTid;
  dst->release_store_tid_ = tid_;
  dst->release_store_reused_ = reused_;
  // Rememeber that we don't need to acquire it in future.
  dst->elem(tid_).reused = reused_;
}

void ThreadClock::acq_rel(ClockCache *c, SyncClock *dst) {
  acquire(c, dst);
  ReleaseStore(c, dst);
}

// Updates only single element related to the current thread in dst->clk_.
void ThreadClock::UpdateCurrentThread(SyncClock *dst) const {
  // Update the threads time, but preserve 'acquired' flag.
  dst->elem(tid_).epoch = clk_[tid_].epoch;

  for (unsigned i = 0; i < kDirtyTids; i++) {
    if (dst->dirty_tids_[i] == tid_) {
      CPP_STAT_INC(StatClockReleaseFast1);
      return;
    }
    if (dst->dirty_tids_[i] == kInvalidTid) {
      CPP_STAT_INC(StatClockReleaseFast2);
      dst->dirty_tids_[i] = tid_;
      return;
    }
  }
  // Reset all 'acquired' flags, O(N).
  CPP_STAT_INC(StatClockReleaseSlow);
  for (uptr i = 0; i < dst->size_; i++)
    dst->elem(i).reused = 0;
  for (unsigned i = 0; i < kDirtyTids; i++)
    dst->dirty_tids_[i] = kInvalidTid;
}

// Checks whether the current threads has already acquired src.
bool ThreadClock::IsAlreadyAcquired(const SyncClock *src) const {
  if (src->elem(tid_).reused != reused_)
    return false;
  for (unsigned i = 0; i < kDirtyTids; i++) {
    unsigned tid = src->dirty_tids_[i];
    if (tid != kInvalidTid) {
      if (clk_[tid].epoch < src->elem(tid).epoch)
        return false;
    }
  }
  return true;
}

void SyncClock::Resize(ClockCache *c, uptr nclk) {
  CPP_STAT_INC(StatClockReleaseResize);
  if (RoundUpTo(nclk, ClockBlock::kClockCount) <=
      RoundUpTo(size_, ClockBlock::kClockCount)) {
    // Growing within the same block.
    // Memory is already allocated, just increase the size.
    size_ = nclk;
    return;
  }
  if (nclk <= ClockBlock::kClockCount) {
    // Grow from 0 to one-level table.
    CHECK_EQ(size_, 0);
    CHECK_EQ(tab_, 0);
    CHECK_EQ(tab_idx_, 0);
    size_ = nclk;
    tab_idx_ = ctx->clock_alloc.Alloc(c);
    tab_ = ctx->clock_alloc.Map(tab_idx_);
    internal_memset(tab_, 0, sizeof(*tab_));
    return;
  }
  // Growing two-level table.
  if (size_ == 0) {
    // Allocate first level table.
    tab_idx_ = ctx->clock_alloc.Alloc(c);
    tab_ = ctx->clock_alloc.Map(tab_idx_);
    internal_memset(tab_, 0, sizeof(*tab_));
  } else if (size_ <= ClockBlock::kClockCount) {
    // Transform one-level table to two-level table.
    u32 old = tab_idx_;
    tab_idx_ = ctx->clock_alloc.Alloc(c);
    tab_ = ctx->clock_alloc.Map(tab_idx_);
    internal_memset(tab_, 0, sizeof(*tab_));
    tab_->table[0] = old;
  }
  // At this point we have first level table allocated.
  // Add second level tables as necessary.
  for (uptr i = RoundUpTo(size_, ClockBlock::kClockCount);
      i < nclk; i += ClockBlock::kClockCount) {
    u32 idx = ctx->clock_alloc.Alloc(c);
    ClockBlock *cb = ctx->clock_alloc.Map(idx);
    internal_memset(cb, 0, sizeof(*cb));
    CHECK_EQ(tab_->table[i/ClockBlock::kClockCount], 0);
    tab_->table[i/ClockBlock::kClockCount] = idx;
  }
  size_ = nclk;
}

// Sets a single element in the vector clock.
// This function is called only from weird places like AcquireGlobal.
void ThreadClock::set(unsigned tid, u64 v) {
  DCHECK_LT(tid, kMaxTid);
  DCHECK_GE(v, clk_[tid].epoch);
  clk_[tid].epoch = v;
  if (nclk_ <= tid)
    nclk_ = tid + 1;
  last_acquire_ = clk_[tid_].epoch;
}

void ThreadClock::DebugDump(int(*printf)(const char *s, ...)) {
  printf("clock=[");
  for (uptr i = 0; i < nclk_; i++)
    printf("%s%llu", i == 0 ? "" : ",", clk_[i].epoch);
  printf("] reused=[");
  for (uptr i = 0; i < nclk_; i++)
    printf("%s%llu", i == 0 ? "" : ",", clk_[i].reused);
  printf("] tid=%u/%u last_acq=%llu",
      tid_, reused_, last_acquire_);
}

SyncClock::SyncClock()
    : release_store_tid_(kInvalidTid)
    , release_store_reused_()
    , tab_()
    , tab_idx_()
    , size_() {
  for (uptr i = 0; i < kDirtyTids; i++)
    dirty_tids_[i] = kInvalidTid;
}

SyncClock::~SyncClock() {
  // Reset must be called before dtor.
  CHECK_EQ(size_, 0);
  CHECK_EQ(tab_, 0);
  CHECK_EQ(tab_idx_, 0);
}

void SyncClock::Reset(ClockCache *c) {
  if (size_ == 0) {
    // nothing
  } else if (size_ <= ClockBlock::kClockCount) {
    // One-level table.
    ctx->clock_alloc.Free(c, tab_idx_);
  } else {
    // Two-level table.
    for (uptr i = 0; i < size_; i += ClockBlock::kClockCount)
      ctx->clock_alloc.Free(c, tab_->table[i / ClockBlock::kClockCount]);
    ctx->clock_alloc.Free(c, tab_idx_);
  }
  tab_ = 0;
  tab_idx_ = 0;
  size_ = 0;
  release_store_tid_ = kInvalidTid;
  release_store_reused_ = 0;
  for (uptr i = 0; i < kDirtyTids; i++)
    dirty_tids_[i] = kInvalidTid;
}

ClockElem &SyncClock::elem(unsigned tid) const {
  DCHECK_LT(tid, size_);
  if (size_ <= ClockBlock::kClockCount)
    return tab_->clock[tid];
  u32 idx = tab_->table[tid / ClockBlock::kClockCount];
  ClockBlock *cb = ctx->clock_alloc.Map(idx);
  return cb->clock[tid % ClockBlock::kClockCount];
}

void SyncClock::DebugDump(int(*printf)(const char *s, ...)) {
  printf("clock=[");
  for (uptr i = 0; i < size_; i++)
    printf("%s%llu", i == 0 ? "" : ",", elem(i).epoch);
  printf("] reused=[");
  for (uptr i = 0; i < size_; i++)
    printf("%s%llu", i == 0 ? "" : ",", elem(i).reused);
  printf("] release_store_tid=%d/%d dirty_tids=%d/%d",
      release_store_tid_, release_store_reused_,
      dirty_tids_[0], dirty_tids_[1]);
}
}  // namespace __tsan
