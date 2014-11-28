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

static int sys_mprotect(void *addr, size_t length, int prot)
{
  int s = vm_attribute(getpid(), addr, length, prot);
  return s;
}

static void *do_mmap(void *addr, size_t length, int prot, int flags,
                      int fd, off_t offset)
{
  // RICH: Need to check flags, fd, offset.

  void *p = addr;
  pid_t pid = getpid();

  int s = vm_allocate(pid, &p, length, addr == NULL);
  if (s != 0) {
    return (void *)(intptr_t)s;
  }

  s = vm_attribute(pid, p, length, prot);
  if (s != 0) {
    vm_free(pid, p, length);
    return (void *)(intptr_t)s;
  }

  return p;
}

#ifdef SYS_mmap
static void *sys_mmap(void *addr, size_t length, int prot, int flags,
                      int fd, off_t offset)
{
  return do_mmap(addr, length, prot, flags, fd, offset);
}
#endif

#ifdef SYS_mmap2
static void *sys_mmap2(void *addr, size_t length, int prot, int flags,
                       int fd, off_t offset)
{
  return do_mmap(addr, length, prot, flags, fd, offset * 4096);
}
#endif

static int sys_munmap(void *addr, size_t length)
{
  return -ENOSYS;
}

static int sys_mremap(void *old_addr, size_t old_length,
                    size_t new_length, int flags, void *new_address)
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
  SYSCALL(mremap);
}
