//===-- tsan_clock_test.cc ------------------------------------------------===//
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
#include "gtest/gtest.h"

namespace __tsan {

TEST(Clock, VectorBasic) {
  ThreadClock clk;
  CHECK_EQ(clk.size(), 0);
  clk.tick(0);
  CHECK_EQ(clk.size(), 1);
  CHECK_EQ(clk.get(0), 1);
  clk.tick(3);
  CHECK_EQ(clk.size(), 4);
  CHECK_EQ(clk.get(0), 1);
  CHECK_EQ(clk.get(1), 0);
  CHECK_EQ(clk.get(2), 0);
  CHECK_EQ(clk.get(3), 1);
  clk.tick(3);
  CHECK_EQ(clk.get(3), 2);
}

TEST(Clock, ChunkedBasic) {
  ThreadClock vector;
  SyncClock chunked;
  CHECK_EQ(vector.size(), 0);
  CHECK_EQ(chunked.size(), 0);
  vector.acquire(&chunked);
  CHECK_EQ(vector.size(), 0);
  CHECK_EQ(chunked.size(), 0);
  vector.release(&chunked);
  CHECK_EQ(vector.size(), 0);
  CHECK_EQ(chunked.size(), 0);
  vector.acq_rel(&chunked);
  CHECK_EQ(vector.size(), 0);
  CHECK_EQ(chunked.size(), 0);
}

TEST(Clock, AcquireRelease) {
  ThreadClock vector1;
  vector1.tick(100);
  SyncClock chunked;
  vector1.release(&chunked);
  CHECK_EQ(chunked.size(), 101);
  ThreadClock vector2;
  vector2.acquire(&chunked);
  CHECK_EQ(vector2.size(), 101);
  CHECK_EQ(vector2.get(0), 0);
  CHECK_EQ(vector2.get(1), 0);
  CHECK_EQ(vector2.get(99), 0);
  CHECK_EQ(vector2.get(100), 1);
}

TEST(Clock, ManyThreads) {
  SyncClock chunked;
  for (int i = 0; i < 100; i++) {
    ThreadClock vector;
    vector.tick(i);
    vector.release(&chunked);
    CHECK_EQ(chunked.size(), i + 1);
    vector.acquire(&chunked);
    CHECK_EQ(vector.size(), i + 1);
  }
  ThreadClock vector;
  vector.acquire(&chunked);
  CHECK_EQ(vector.size(), 100);
  for (int i = 0; i < 100; i++)
    CHECK_EQ(vector.get(i), 1);
}

TEST(Clock, DifferentSizes) {
  {
    ThreadClock vector1;
    vector1.tick(10);
    ThreadClock vector2;
    vector2.tick(20);
    {
      SyncClock chunked;
      vector1.release(&chunked);
      CHECK_EQ(chunked.size(), 11);
      vector2.release(&chunked);
      CHECK_EQ(chunked.size(), 21);
    }
    {
      SyncClock chunked;
      vector2.release(&chunked);
      CHECK_EQ(chunked.size(), 21);
      vector1.release(&chunked);
      CHECK_EQ(chunked.size(), 21);
    }
    {
      SyncClock chunked;
      vector1.release(&chunked);
      vector2.acquire(&chunked);
      CHECK_EQ(vector2.size(), 21);
    }
    {
      SyncClock chunked;
      vector2.release(&chunked);
      vector1.acquire(&chunked);
      CHECK_EQ(vector1.size(), 21);
    }
  }
}

const int kThreads = 4;
const int kClocks = 4;

// SimpleSyncClock and SimpleThreadClock implement the same thing as
// SyncClock and ThreadClock, but in a very simple way.
struct SimpleSyncClock {
  u64 clock[kThreads];
  uptr size;

  SimpleSyncClock() {
    Reset();
  }

  void Reset() {
    size = 0;
    for (uptr i = 0; i < kThreads; i++)
      clock[i] = 0;
  }

  bool verify(const SyncClock *other) const {
    for (uptr i = 0; i < min(size, other->size()); i++) {
      if (clock[i] != other->get(i))
        return false;
    }
    for (uptr i = min(size, other->size()); i < max(size, other->size()); i++) {
      if (i < size && clock[i] != 0)
        return false;
      if (i < other->size() && other->get(i) != 0)
        return false;
    }
    return true;
  }
};

struct SimpleThreadClock {
  u64 clock[kThreads];
  uptr size;
  unsigned tid;

  explicit SimpleThreadClock(unsigned tid) {
    this->tid = tid;
    size = tid + 1;
    for (uptr i = 0; i < kThreads; i++)
      clock[i] = 0;
  }

  void tick() {
    clock[tid]++;
  }

  void acquire(const SimpleSyncClock *src) {
    if (size < src->size)
      size = src->size;
    for (uptr i = 0; i < kThreads; i++)
      clock[i] = max(clock[i], src->clock[i]);
  }

  void release(SimpleSyncClock *dst) const {
    if (dst->size < size)
      dst->size = size;
    for (uptr i = 0; i < kThreads; i++)
      dst->clock[i] = max(dst->clock[i], clock[i]);
  }

  void acq_rel(SimpleSyncClock *dst) {
    acquire(dst);
    release(dst);
  }

  void ReleaseStore(SimpleSyncClock *dst) const {
    if (dst->size < size)
      dst->size = size;
    for (uptr i = 0; i < kThreads; i++)
      dst->clock[i] = clock[i];
  }

  bool verify(const ThreadClock *other) const {
    for (uptr i = 0; i < min(size, other->size()); i++) {
      if (clock[i] != other->get(i))
        return false;
    }
    for (uptr i = min(size, other->size()); i < max(size, other->size()); i++) {
      if (i < size && clock[i] != 0)
        return false;
      if (i < other->size() && other->get(i) != 0)
        return false;
    }
    return true;
  }
};

static bool ClockFuzzer(bool printing) {
  // Create kThreads thread clocks.
  SimpleThreadClock *thr0[kThreads];
  ThreadClock *thr1[kThreads];
  unsigned reused[kThreads];
  for (unsigned i = 0; i < kThreads; i++) {
    reused[i] = 0;
    thr0[i] = new SimpleThreadClock(i);
    thr1[i] = new ThreadClock(i, reused[i]);
  }

  // Create kClocks sync clocks.
  SimpleSyncClock *sync0[kClocks];
  SyncClock *sync1[kClocks];
  for (unsigned i = 0; i < kClocks; i++) {
    sync0[i] = new SimpleSyncClock();
    sync1[i] = new SyncClock();
  }

  // Do N random operations (acquire, release, etc) and compare results
  // for SimpleThread/SyncClock and real Thread/SyncClock.
  for (int i = 0; i < 10000; i++) {
    unsigned tid = rand() % kThreads;
    unsigned cid = rand() % kClocks;
    thr0[tid]->tick();
    thr1[tid]->tick();

    switch (rand() % 6) {
    case 0:
      if (printing)
        printf("acquire thr%d <- clk%d\n", tid, cid);
      thr0[tid]->acquire(sync0[cid]);
      thr1[tid]->acquire(sync1[cid]);
      break;
    case 1:
      if (printing)
        printf("release thr%d -> clk%d\n", tid, cid);
      thr0[tid]->release(sync0[cid]);
      thr1[tid]->release(sync1[cid]);
      break;
    case 2:
      if (printing)
        printf("acq_rel thr%d <> clk%d\n", tid, cid);
      thr0[tid]->acq_rel(sync0[cid]);
      thr1[tid]->acq_rel(sync1[cid]);
      break;
    case 3:
      if (printing)
        printf("rel_str thr%d >> clk%d\n", tid, cid);
      thr0[tid]->ReleaseStore(sync0[cid]);
      thr1[tid]->ReleaseStore(sync1[cid]);
      break;
    case 4:
      if (printing)
        printf("reset clk%d\n", cid);
      sync0[cid]->Reset();
      sync1[cid]->Reset();
      break;
    case 5:
      if (printing)
        printf("reset thr%d\n", tid);
      u64 epoch = thr0[tid]->clock[tid] + 1;
      reused[tid]++;
      delete thr0[tid];
      thr0[tid] = new SimpleThreadClock(tid);
      thr0[tid]->clock[tid] = epoch;
      delete thr1[tid];
      thr1[tid] = new ThreadClock(tid, reused[tid]);
      thr1[tid]->set(epoch);
      break;
    }

    if (printing) {
      for (unsigned i = 0; i < kThreads; i++) {
        printf("thr%d: ", i);
        thr1[i]->DebugDump(printf);
        printf("\n");
      }
      for (unsigned i = 0; i < kClocks; i++) {
        printf("clk%d: ", i);
        sync1[i]->DebugDump(printf);
        printf("\n");
      }

      printf("\n");
    }

    if (!thr0[tid]->verify(thr1[tid]) || !sync0[cid]->verify(sync1[cid])) {
      if (!printing)
        return false;
      printf("differs with model:\n");
      for (unsigned i = 0; i < kThreads; i++) {
        printf("thr%d: clock=[", i);
        for (uptr j = 0; j < thr0[i]->size; j++)
          printf("%s%llu", j == 0 ? "" : ",", thr0[i]->clock[j]);
        printf("]\n");
      }
      for (unsigned i = 0; i < kClocks; i++) {
        printf("clk%d: clock=[", i);
        for (uptr j = 0; j < sync0[i]->size; j++)
          printf("%s%llu", j == 0 ? "" : ",", sync0[i]->clock[j]);
        printf("]\n");
      }
      return false;
    }
  }
  return true;
}

TEST(Clock, Fuzzer) {
  timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  int seed = ts.tv_sec + ts.tv_nsec;
  printf("seed=%d\n", seed);
  srand(seed);
  if (!ClockFuzzer(false)) {
    // Redo the test with the same seed, but logging operations.
    srand(seed);
    ClockFuzzer(true);
    ASSERT_TRUE(false);
  }
}

}  // namespace __tsan
