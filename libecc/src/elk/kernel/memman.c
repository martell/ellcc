/* Initialize a memory allocation handler.
 */
#include <sys/mman.h>
#include <errno.h>

#include "config.h"
#include "kernel.h"
#include "syscalls.h"           // For syscall numbers.
#include "thread.h"
#include "page.h"
#include "vm.h"

// Make memory management a loadable feature.
FEATURE_CLASS(memman, memman)

extern char __end[];            // The end of the .bss area.
static char *brk_ptr;

static char *sys_brk(char *addr)
{
  if (addr == 0) {
    return brk_ptr;         // Return the current value.
  }

  // All other calls to brk fail.
  return brk_ptr;
}

#include <string.h>     // RICH
static int sys_mprotect(void *addr, size_t len, int prot)
{
  // RICH: what to do with len?
  int s = vm_attribute(getpid(), addr, prot);
  printf("mprotect: %s\n", strerror(-s));
  return s;
}

#ifdef SYS_mmap
static void *sys_mmap(void *addr, size_t length, int prot, int flags,
                      int fd, off_t offset)
{
  return (void *)-ENOSYS;
}
#endif

#ifdef SYS_mmap2
static void *sys_mmap2(void *addr, size_t length, int prot, int flags,
                      int fd, off_t offset)
{
  // RICH: Need to check flags, fd, offset.

  void *p = addr;
  pid_t pid = getpid();

  int s = vm_allocate(pid, &p, length, addr == NULL);
  if (s != 0) {
    printf("mmap allocate: %s\n", strerror(-s));
    return (void *)s;
  }

  s = vm_attribute(pid, p, prot);
  if (s != 0) {
    printf("mmap attribute: %s\n", strerror(-s));
    return (void *)s;
  }

  return p;
}
#endif

static int sys_munmap(void *addr, size_t length)
{
  return -ENOSYS;
}

/* Initialize the simple memory allocator.
 */
ELK_CONSTRUCTOR()
{
  brk_ptr = (char *)round_page((uintptr_t)__end);
  SYSCALL(brk);
#ifdef SYS_mmap
  SYSCALL(mmap);
#endif
#ifdef SYS_mmap2
  SYSCALL(mmap2);
#endif
  SYSCALL(mprotect);
  SYSCALL(munmap);
}
