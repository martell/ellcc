/* Initialize a simple memory allocation handler.
 */
#include <unistd.h>
#include <syscalls.h>           // For syscall numbers.
#include <sys/uio.h>            // For writev (used by printf().
#include <sys/ioctl.h>
#include <kernel.h>

// Make the simple console a loadable feature.
FEATURE(simple_memman, memman)

extern char __end[];            // The end of the .bss area.
extern char *__heap_end__;      // The bottom of the allocated stacks.

static char *brk_ptr = __end;

#include <stdio.h>
static char *sys_brk(char *addr)
{
  if (addr == 0) {
    return brk_ptr;         // Return the current value.
  }
  if (addr >= __end && addr < __heap_end__) {
    // A good request, update the pointer.
    brk_ptr = addr;
  }
  return brk_ptr;
}

/* Initialize the simple memory allocator.
 */
CONSTRUCTOR()
{
  // Set up a simple brk system call.
  __elk_set_syscall(SYS_brk, sys_brk);
}
