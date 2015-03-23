//===-- tsan_sync.cc ------------------------------------------------------===//
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
#include "sanitizer_common/sanitizer_placement_new.h"
#include "tsan_sync.h"
#include "tsan_rtl.h"
#include "tsan_mman.h"

namespace __tsan {

void DDMutexInit(ThreadState *thr, uptr pc, SyncVar *s);

SyncVar::SyncVar()
    : mtx(MutexTypeSyncVar, StatMtxSyncVar) {
  Reset(0);
}

void SyncVar::Init(ThreadState *thr, uptr pc, uptr addr, u64 uid) {
  this->addr = addr;
  this->uid = uid;
  this->next = 0;

  creation_stack_id = 0;
  if (kCppMode)  // Go does not use them
    creation_stack_id = CurrentStackId(thr, pc);
  if (common_flags()->detect_deadlocks)
    DDMutexInit(thr, pc, this);
}

void SyncVar::Reset(ThreadState *thr) {
  uid = 0;
  creation_stack_id = 0;
  owner_tid = kInvalidTid;
  last_lock = 0;
  recursion = 0;
  is_rw = 0;
  is_recursive = 0;
  is_broken = 0;
  is_linker_init = 0;

  if (thr == 0) {
    CHECK_EQ(clock.size(), 0);
    CHECK_EQ(read_clock.size(), 0);
  } else {
    clock.Reset(&thr->clock_cache);
    read_clock.Reset(&thr->clock_cache);
  }
}

SyncVar* SyncTab::GetIfExistsAndLock(uptr addr, bool write_lock) {
  return GetAndLock(0, 0, addr, write_lock, false);
}

SyncVar* SyncTab::Create(ThreadState *thr, uptr pc, uptr addr) {
  StatInc(thr, StatSyncCreated);
  void *mem = internal_alloc(MBlockSync, sizeof(SyncVar));
  const u64 uid = atomic_fetch_add(&uid_gen_, 1, memory_order_relaxed);
  SyncVar *res = new(mem) SyncVar(addr, uid);
  res->creation_stack_id = 0;
  if (!kGoMode)  // Go does not use them
    res->creation_stack_id = CurrentStackId(thr, pc);
  if (flags()->detect_deadlocks)
    DDMutexInit(thr, pc, res);
  return res;
}

SyncVar* SyncTab::GetAndLock(ThreadState *thr, uptr pc,
                             uptr addr, bool write_lock, bool create) {
#ifndef TSAN_GO
  {  // NOLINT
    SyncVar *res = GetJavaSync(thr, pc, addr, write_lock, create);
    if (res)
      return res;
  }

  // Here we ask only PrimaryAllocator, because
  // SecondaryAllocator::PointerIsMine() is slow and we have fallback on
  // the hashmap anyway.
  if (PrimaryAllocator::PointerIsMine((void*)addr)) {
    MBlock *b = user_mblock(thr, (void*)addr);
    CHECK_NE(b, 0);
    MBlock::ScopedLock l(b);
    SyncVar *res = 0;
    for (res = b->ListHead(); res; res = res->next) {
      if (res->addr == addr)
        break;
    }
    if (res == 0) {
      if (!create)
        return 0;
      res = Create(thr, pc, addr);
      b->ListPush(res);
    }
    if (write_lock)
      res->mtx.Lock();
    else
      res->mtx.ReadLock();
    return res;
  }
#endif

  Part *p = &tab_[PartIdx(addr)];
  {
    ReadLock l(&p->mtx);
    for (SyncVar *res = p->val; res; res = res->next) {
      if (res->addr == addr) {
        if (write_lock)
          res->mtx.Lock();
        else
          res->mtx.ReadLock();
        return res;
      }
    }
  }
  if (!create)
    return 0;
  {
    Lock l(&p->mtx);
    SyncVar *res = p->val;
    for (; res; res = res->next) {
      if (res->addr == addr)
        break;
    }
    if (res == 0) {
      res = Create(thr, pc, addr);
      res->next = p->val;
      p->val = res;
    }
    if (write_lock)
      res->mtx.Lock();
    else
      res->mtx.ReadLock();
    return res;
  }
}

bool MetaMap::FreeRange(ThreadState *thr, uptr pc, uptr p, uptr sz) {
  bool has_something = false;
  u32 *meta = MemToMeta(p);
  u32 *end = MemToMeta(p + sz);
  if (end == meta)
    end++;
  for (; meta < end; meta++) {
    u32 idx = *meta;
    *meta = 0;
    for (;;) {
      if (idx == 0)
        break;
      has_something = true;
      if (idx & kFlagBlock) {
        block_alloc_.Free(&thr->block_cache, idx & ~kFlagMask);
        break;
      } else if (idx & kFlagSync) {
        DCHECK(idx & kFlagSync);
        SyncVar *s = sync_alloc_.Map(idx & ~kFlagMask);
        u32 next = s->next;
        s->Reset(thr);
        sync_alloc_.Free(&thr->sync_cache, idx & ~kFlagMask);
        idx = next;
      } else {
        CHECK(0);
      }
      prev = &res->next;
      res = *prev;
    }
  }
  return has_something;
}

// ResetRange removes all meta objects from the range.
// It is called for large mmap-ed regions. The function is best-effort wrt
// freeing of meta objects, because we don't want to page in the whole range
// which can be huge. The function probes pages one-by-one until it finds a page
// without meta objects, at this point it stops freeing meta objects. Because
// thread stacks grow top-down, we do the same starting from end as well.
void MetaMap::ResetRange(ThreadState *thr, uptr pc, uptr p, uptr sz) {
  const uptr kMetaRatio = kMetaShadowCell / kMetaShadowSize;
  const uptr kPageSize = GetPageSizeCached() * kMetaRatio;
  if (sz <= 4 * kPageSize) {
    // If the range is small, just do the normal free procedure.
    FreeRange(thr, pc, p, sz);
    return;
  }
  // First, round both ends of the range to page size.
  uptr diff = RoundUp(p, kPageSize) - p;
  if (diff != 0) {
    FreeRange(thr, pc, p, diff);
    p += diff;
    sz -= diff;
  }
  diff = p + sz - RoundDown(p + sz, kPageSize);
  if (diff != 0) {
    FreeRange(thr, pc, p + sz - diff, diff);
    sz -= diff;
  }
  // Now we must have a non-empty page-aligned range.
  CHECK_GT(sz, 0);
  CHECK_EQ(p, RoundUp(p, kPageSize));
  CHECK_EQ(sz, RoundUp(sz, kPageSize));
  const uptr p0 = p;
  const uptr sz0 = sz;
  // Probe start of the range.
  while (sz > 0) {
    bool has_something = FreeRange(thr, pc, p, kPageSize);
    p += kPageSize;
    sz -= kPageSize;
    if (!has_something)
      break;
  }
  // Probe end of the range.
  while (sz > 0) {
    bool has_something = FreeRange(thr, pc, p - kPageSize, kPageSize);
    sz -= kPageSize;
    if (!has_something)
      break;
  }
  // Finally, page out the whole range (including the parts that we've just
  // freed). Note: we can't simply madvise, because we need to leave a zeroed
  // range (otherwise __tsan_java_move can crash if it encounters a left-over
  // meta objects in java heap).
  uptr metap = (uptr)MemToMeta(p0);
  uptr metasz = sz0 / kMetaRatio;
  UnmapOrDie((void*)metap, metasz);
  MmapFixedNoReserve(metap, metasz);
}

MBlock* MetaMap::GetBlock(uptr p) {
  u32 *meta = MemToMeta(p);
  u32 idx = *meta;
  for (;;) {
    if (idx == 0)
      return 0;
    if (idx & kFlagBlock)
      return block_alloc_.Map(idx & ~kFlagMask);
    DCHECK(idx & kFlagSync);
    SyncVar * s = sync_alloc_.Map(idx & ~kFlagMask);
    idx = s->next;
  }
}

StackTrace::StackTrace()
    : n_()
    , s_()
    , c_() {
}

StackTrace::StackTrace(uptr *buf, uptr cnt)
    : n_()
    , s_(buf)
    , c_(cnt) {
  CHECK_NE(buf, 0);
  CHECK_NE(cnt, 0);
}

SyncVar* MetaMap::GetAndLock(ThreadState *thr, uptr pc,
                             uptr addr, bool write_lock, bool create) {
  u32 *meta = MemToMeta(addr);
  u32 idx0 = *meta;
  u32 myidx = 0;
  SyncVar *mys = 0;
  for (;;) {
    u32 idx = idx0;
    for (;;) {
      if (idx == 0)
        break;
      if (idx & kFlagBlock)
        break;
      DCHECK(idx & kFlagSync);
      SyncVar * s = sync_alloc_.Map(idx & ~kFlagMask);
      if (s->addr == addr) {
        if (myidx != 0) {
          mys->Reset(thr);
          sync_alloc_.Free(&thr->sync_cache, myidx);
        }
        if (write_lock)
          s->mtx.Lock();
        else
          s->mtx.ReadLock();
        return s;
      }
      idx = s->next;
    }
    if (!create)
      return 0;
    if (*meta != idx0) {
      idx0 = *meta;
      continue;
    }

void StackTrace::Reset() {
  if (s_ && !c_) {
    CHECK_NE(n_, 0);
    internal_free(s_);
    s_ = 0;
  }
  n_ = 0;
}

void MetaMap::MoveMemory(uptr src, uptr dst, uptr sz) {
  // src and dst can overlap,
  // there are no concurrent accesses to the regions (e.g. stop-the-world).
  CHECK_NE(src, dst);
  CHECK_NE(sz, 0);
  uptr diff = dst - src;
  u32 *src_meta = MemToMeta(src);
  u32 *dst_meta = MemToMeta(dst);
  u32 *src_meta_end = MemToMeta(src + sz);
  uptr inc = 1;
  if (dst > src) {
    src_meta = MemToMeta(src + sz) - 1;
    dst_meta = MemToMeta(dst + sz) - 1;
    src_meta_end = MemToMeta(src) - 1;
    inc = -1;
  }
  for (; src_meta != src_meta_end; src_meta += inc, dst_meta += inc) {
    CHECK_EQ(*dst_meta, 0);
    u32 idx = *src_meta;
    *src_meta = 0;
    *dst_meta = idx;
    // Patch the addresses in sync objects.
    while (idx != 0) {
      if (idx & kFlagBlock)
        break;
      CHECK(idx & kFlagSync);
      SyncVar *s = sync_alloc_.Map(idx & ~kFlagMask);
      s->addr += diff;
      idx = s->next;
    }
  } else {
    // Cap potentially huge stacks.
    if (n_ + !!toppc > kTraceStackSize) {
      start = n_ - kTraceStackSize + !!toppc;
      n_ = kTraceStackSize - !!toppc;
    }
    s_ = (uptr*)internal_alloc(MBlockStackTrace,
                               (n_ + !!toppc) * sizeof(s_[0]));
  }
  for (uptr i = 0; i < n_; i++)
    s_[i] = thr->shadow_stack[start + i];
  if (toppc) {
    s_[n_] = toppc;
    n_++;
  }
}

void StackTrace::CopyFrom(const StackTrace& other) {
  Reset();
  Init(other.Begin(), other.Size());
}

bool StackTrace::IsEmpty() const {
  return n_ == 0;
}

uptr StackTrace::Size() const {
  return n_;
}

uptr StackTrace::Get(uptr i) const {
  CHECK_LT(i, n_);
  return s_[i];
}

const uptr *StackTrace::Begin() const {
  return s_;
}

}  // namespace __tsan
