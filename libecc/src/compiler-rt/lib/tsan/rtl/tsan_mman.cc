//===-- tsan_mman.cc ------------------------------------------------------===//
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
#include "sanitizer_common/sanitizer_allocator_interface.h"
#include "sanitizer_common/sanitizer_common.h"
#include "sanitizer_common/sanitizer_placement_new.h"
#include "tsan_mman.h"
#include "tsan_rtl.h"
#include "tsan_report.h"
#include "tsan_flags.h"

// May be overriden by front-end.
SANITIZER_WEAK_DEFAULT_IMPL
void __sanitizer_malloc_hook(void *ptr, uptr size) {
  (void)ptr;
  (void)size;
}

SANITIZER_WEAK_DEFAULT_IMPL
void __sanitizer_free_hook(void *ptr) {
  (void)ptr;
}

namespace __tsan {

COMPILER_CHECK(sizeof(MBlock) == 16);

void MBlock::Lock() {
  atomic_uintptr_t *a = reinterpret_cast<atomic_uintptr_t*>(this);
  uptr v = atomic_load(a, memory_order_relaxed);
  for (int iter = 0;; iter++) {
    if (v & 1) {
      if (iter < 10)
        proc_yield(20);
      else
        internal_sched_yield();
      v = atomic_load(a, memory_order_relaxed);
      continue;
    }
    if (atomic_compare_exchange_weak(a, &v, v | 1, memory_order_acquire))
      break;
  }
}

void MBlock::Unlock() {
  atomic_uintptr_t *a = reinterpret_cast<atomic_uintptr_t*>(this);
  uptr v = atomic_load(a, memory_order_relaxed);
  DCHECK(v & 1);
  atomic_store(a, v & ~1, memory_order_relaxed);
}

struct MapUnmapCallback {
  void OnMap(uptr p, uptr size) const { }
  void OnUnmap(uptr p, uptr size) const {
    // We are about to unmap a chunk of user memory.
    // Mark the corresponding shadow memory as not needed.
    DontNeedShadowFor(p, size);
    // Mark the corresponding meta shadow memory as not needed.
    // Note the block does not contain any meta info at this point
    // (this happens after free).
    const uptr kMetaRatio = kMetaShadowCell / kMetaShadowSize;
    const uptr kPageSize = GetPageSizeCached() * kMetaRatio;
    // Block came from LargeMmapAllocator, so must be large.
    // We rely on this in the calculations below.
    CHECK_GE(size, 2 * kPageSize);
    uptr diff = RoundUp(p, kPageSize) - p;
    if (diff != 0) {
      p += diff;
      size -= diff;
    }
    diff = p + size - RoundDown(p + size, kPageSize);
    if (diff != 0)
      size -= diff;
    FlushUnneededShadowMemory((uptr)MemToMeta(p), size / kMetaRatio);
  }
};

static char allocator_placeholder[sizeof(Allocator)] ALIGNED(64);
Allocator *allocator() {
  return reinterpret_cast<Allocator*>(&allocator_placeholder);
}

void InitializeAllocator() {
  allocator()->Init(common_flags()->allocator_may_return_null);
}

void AllocatorProcStart(Processor *proc) {
  allocator()->InitCache(&proc->alloc_cache);
  internal_allocator()->InitCache(&proc->internal_alloc_cache);
}

void AllocatorProcFinish(Processor *proc) {
  allocator()->DestroyCache(&proc->alloc_cache);
  internal_allocator()->DestroyCache(&proc->internal_alloc_cache);
}

void AllocatorPrintStats() {
  allocator()->PrintStats();
}

static void SignalUnsafeCall(ThreadState *thr, uptr pc) {
  if (atomic_load_relaxed(&thr->in_signal_handler) == 0 ||
      !flags()->report_signal_unsafe)
    return;
  VarSizeStackTrace stack;
  ObtainCurrentStack(thr, pc, &stack);
  if (IsFiredSuppression(ctx, ReportTypeSignalUnsafe, stack))
    return;
  ThreadRegistryLock l(ctx->thread_registry);
  ScopedReport rep(ReportTypeSignalUnsafe);
  rep.AddStack(stack, true);
  OutputReport(thr, rep);
}

void *user_alloc(ThreadState *thr, uptr pc, uptr sz, uptr align, bool signal) {
  if ((sz >= (1ull << 40)) || (align >= (1ull << 40)))
    return allocator()->ReturnNullOrDie();
  void *p = allocator()->Allocate(&thr->proc()->alloc_cache, sz, align);
  if (p == 0)
    return 0;
  MBlock *b = new(allocator()->GetMetaData(p)) MBlock;
  b->Init(sz, thr->tid, CurrentStackId(thr, pc));
  if (ctx && ctx->initialized) {
    if (thr->ignore_reads_and_writes == 0)
      MemoryRangeImitateWrite(thr, pc, (uptr)p, sz);
    else
      MemoryResetRange(thr, pc, (uptr)p, sz);
  }
  DPrintf("#%d: alloc(%zu) = %p\n", thr->tid, sz, p);
  if (signal)
    SignalUnsafeCall(thr, pc);
  return p;
}

void *user_calloc(ThreadState *thr, uptr pc, uptr size, uptr n) {
  if (CallocShouldReturnNullDueToOverflow(size, n))
    return allocator()->ReturnNullOrDie();
  void *p = user_alloc(thr, pc, n * size);
  if (p)
    internal_memset(p, 0, n * size);
  return p;
}

void user_free(ThreadState *thr, uptr pc, void *p, bool signal) {
  CHECK_NE(p, (void*)0);
  DPrintf("#%d: free(%p)\n", thr->tid, p);
  MBlock *b = (MBlock*)allocator()->GetMetaData(p);
  if (b->ListHead()) {
    MBlock::ScopedLock l(b);
    for (SyncVar *s = b->ListHead(); s;) {
      SyncVar *res = s;
      s = s->next;
      StatInc(thr, StatSyncDestroyed);
      res->mtx.Lock();
      res->mtx.Unlock();
      DestroyAndFree(res);
    }
    b->ListReset();
  }
  if (ctx && ctx->initialized) {
    if (thr->ignore_reads_and_writes == 0)
      MemoryRangeFreed(thr, pc, (uptr)p, b->Size());
  }
  allocator()->Deallocate(&thr->proc()->alloc_cache, p);
  if (signal)
    SignalUnsafeCall(thr, pc);
}

void OnUserAlloc(ThreadState *thr, uptr pc, uptr p, uptr sz, bool write) {
  DPrintf("#%d: alloc(%zu) = %p\n", thr->tid, sz, p);
  ctx->metamap.AllocBlock(thr, pc, p, sz);
  if (write && thr->ignore_reads_and_writes == 0)
    MemoryRangeImitateWrite(thr, pc, (uptr)p, sz);
  else
    MemoryResetRange(thr, pc, (uptr)p, sz);
}

void OnUserFree(ThreadState *thr, uptr pc, uptr p, bool write) {
  CHECK_NE(p, (void*)0);
  uptr sz = ctx->metamap.FreeBlock(thr->proc(), p);
  DPrintf("#%d: free(%p, %zu)\n", thr->tid, p, sz);
  if (write && thr->ignore_reads_and_writes == 0)
    MemoryRangeFreed(thr, pc, (uptr)p, sz);
}

void *user_realloc(ThreadState *thr, uptr pc, void *p, uptr sz) {
  void *p2 = 0;
  // FIXME: Handle "shrinking" more efficiently,
  // it seems that some software actually does this.
  if (sz) {
    p2 = user_alloc(thr, pc, sz);
    if (p2 == 0)
      return 0;
    if (p) {
      uptr oldsz = user_alloc_usable_size(p);
      internal_memcpy(p2, p, min(oldsz, sz));
    }
  }
  if (p)
    user_free(thr, pc, p);
  return p2;
}

uptr user_alloc_usable_size(const void *p) {
  if (p == 0)
    return 0;
  MBlock *b = ctx->metamap.GetBlock((uptr)p);
  if (!b)
    return 0;  // Not a valid pointer.
  if (b->siz == 0)
    return 1;  // Zero-sized allocations are actually 1 byte.
  return b->siz;
}

MBlock *user_mblock(ThreadState *thr, void *p) {
  CHECK_NE(p, 0);
  Allocator *a = allocator();
  void *b = a->GetBlockBegin(p);
  if (b == 0)
    return 0;
  return (MBlock*)a->GetMetaData(b);
}

void invoke_malloc_hook(void *ptr, uptr size) {
  ThreadState *thr = cur_thread();
  if (ctx == 0 || !ctx->initialized || thr->ignore_interceptors)
    return;
  __sanitizer_malloc_hook(ptr, size);
}

void invoke_free_hook(void *ptr) {
  ThreadState *thr = cur_thread();
  if (ctx == 0 || !ctx->initialized || thr->ignore_interceptors)
    return;
  __sanitizer_free_hook(ptr);
}

void *internal_alloc(MBlockType typ, uptr sz) {
  ThreadState *thr = cur_thread();
  if (thr->nomalloc) {
    thr->nomalloc = 0;  // CHECK calls internal_malloc().
    CHECK(0);
  }
  return InternalAlloc(sz, &thr->proc()->internal_alloc_cache);
}

void internal_free(void *p) {
  ThreadState *thr = cur_thread();
  if (thr->nomalloc) {
    thr->nomalloc = 0;  // CHECK calls internal_malloc().
    CHECK(0);
  }
  InternalFree(p, &thr->proc()->internal_alloc_cache);
}

}  // namespace __tsan

using namespace __tsan;

extern "C" {
uptr __sanitizer_get_current_allocated_bytes() {
  u64 stats[AllocatorStatCount];
  allocator()->GetStats(stats);
  u64 m = stats[AllocatorStatMalloced];
  u64 f = stats[AllocatorStatFreed];
  return m >= f ? m - f : 1;
}

uptr __sanitizer_get_heap_size() {
  u64 stats[AllocatorStatCount];
  allocator()->GetStats(stats);
  u64 m = stats[AllocatorStatMmapped];
  u64 f = stats[AllocatorStatUnmapped];
  return m >= f ? m - f : 1;
}

uptr __sanitizer_get_free_bytes() {
  return 1;
}

uptr __sanitizer_get_unmapped_bytes() {
  return 1;
}

uptr __sanitizer_get_estimated_allocated_size(uptr size) {
  return size;
}

int __sanitizer_get_ownership(const void *p) {
  return allocator()->GetBlockBegin(p) != 0;
}

uptr __sanitizer_get_allocated_size(const void *p) {
  return user_alloc_usable_size(p);
}

void __tsan_on_thread_idle() {
  ThreadState *thr = cur_thread();
  allocator()->SwallowCache(&thr->proc()->alloc_cache);
  internal_allocator()->SwallowCache(&thr->proc()->internal_alloc_cache);
  ctx->metamap.OnProcIdle(thr->proc());
}
}  // extern "C"
