//===-- tsan_sync.h ---------------------------------------------*- C++ -*-===//
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
#ifndef TSAN_SYNC_H
#define TSAN_SYNC_H

#include "sanitizer_common/sanitizer_atomic.h"
#include "sanitizer_common/sanitizer_common.h"
#include "sanitizer_common/sanitizer_deadlock_detector_interface.h"
#include "tsan_clock.h"
#include "tsan_defs.h"
#include "tsan_mutex.h"

namespace __tsan {

class StackTrace {
 public:
  StackTrace();
  // Initialized the object in "static mode",
  // in this mode it never calls malloc/free but uses the provided buffer.
  StackTrace(uptr *buf, uptr cnt);
  ~StackTrace();
  void Reset();

  void Init(const uptr *pcs, uptr cnt);
  void ObtainCurrent(ThreadState *thr, uptr toppc);
  bool IsEmpty() const;
  uptr Size() const;
  uptr Get(uptr i) const;
  const uptr *Begin() const;
  void CopyFrom(const StackTrace& other);

 private:
  uptr n_;
  uptr *s_;
  const uptr c_;

  StackTrace(const StackTrace&);
  void operator = (const StackTrace&);
};

struct SyncVar {
  explicit SyncVar(uptr addr, u64 uid);

  static const int kInvalidTid = -1;

  Mutex mtx;
  uptr addr;
  const u64 uid;  // Globally unique id.
  SyncClock clock;
  SyncClock read_clock;  // Used for rw mutexes only.
  u32 creation_stack_id;
  int owner_tid;  // Set only by exclusive owners.
  u64 last_lock;
  int recursion;
  bool is_rw;
  bool is_recursive;
  bool is_broken;
  bool is_linker_init;
  SyncVar *next;  // In SyncTab hashtable.
  DDMutex dd;

  void Init(ThreadState *thr, uptr pc, uptr addr, u64 uid);
  void Reset(Processor *proc);

  u64 GetId() const {
    // 47 lsb is addr, then 14 bits is low part of uid, then 3 zero bits.
    return GetLsb((u64)addr | (uid << 47), 61);
  }
  bool CheckId(u64 uid) const {
    CHECK_EQ(uid, GetLsb(uid, 14));
    return GetLsb(this->uid, 14) == uid;
  }
  static uptr SplitId(u64 id, u64 *uid) {
    *uid = id >> 47;
    return (uptr)GetLsb(id, 47);
  }
};

class SyncTab {
 public:
  SyncTab();
  ~SyncTab();

  void AllocBlock(ThreadState *thr, uptr pc, uptr p, uptr sz);
  uptr FreeBlock(Processor *proc, uptr p);
  bool FreeRange(Processor *proc, uptr p, uptr sz);
  void ResetRange(Processor *proc, uptr p, uptr sz);
  MBlock* GetBlock(uptr p);

  SyncVar* GetOrCreateAndLock(ThreadState *thr, uptr pc,
                              uptr addr, bool write_lock);
  SyncVar* GetIfExistsAndLock(uptr addr, bool write_lock);

  // If the SyncVar does not exist, returns 0.
  SyncVar* GetAndRemove(ThreadState *thr, uptr pc, uptr addr);

  void OnProcIdle(Processor *proc);

 private:
  static const u32 kFlagMask  = 3u << 30;
  static const u32 kFlagBlock = 1u << 30;
  static const u32 kFlagSync  = 2u << 30;
  typedef DenseSlabAlloc<MBlock, 1<<16, 1<<12> BlockAlloc;
  typedef DenseSlabAlloc<SyncVar, 1<<16, 1<<10> SyncAlloc;
  BlockAlloc block_alloc_;
  SyncAlloc sync_alloc_;
  atomic_uint64_t uid_gen_;

  int PartIdx(uptr addr);

  SyncVar* GetAndLock(ThreadState *thr, uptr pc,
                      uptr addr, bool write_lock, bool create);

  SyncTab(const SyncTab&);  // Not implemented.
  void operator = (const SyncTab&);  // Not implemented.
};

}  // namespace __tsan

#endif  // TSAN_SYNC_H
