// RUN: %clangxx_tsan -O1 %s -o %t && not %run %t 2>&1 | FileCheck %s
extern "C" void AnnotateRWLockAcquired(const char *f, int l, void *m, long rw);

int main() {
  int m = 0;
  AnnotateRWLockAcquired(__FILE__, __LINE__, &m, 1);
  AnnotateRWLockAcquired(__FILE__, __LINE__, &m, 0);
  return 0;
}

// CHECK: WARNING: ThreadSanitizer: read lock of a write locked mutex
// CHECK:     #0 AnnotateRWLockAcquired
// CHECK:     #1 main
// CHECK: Location is stack of main thread.
// CHECK:   Mutex {{.*}}) created at:
// CHECK:     #0 AnnotateRWLockAcquired
// CHECK:     #1 main
// CHECK: SUMMARY: ThreadSanitizer: read lock of a write locked mutex

