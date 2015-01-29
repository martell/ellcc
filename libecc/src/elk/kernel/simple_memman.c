/* Initialize a simple memory allocation handler.
 */
#include "syscalls.h"           // For syscall numbers.
#include "kernel.h"
#include "page.h"
#include "crt1.h"

// Make simple memory management a loadable feature.
FEATURE_CLASS(simple_memman, memman)

extern char __end[];            // The end of the .bss area.
extern char *__heap_end__;      // The bottom of the allocated stacks.

static char *brk_ptr;

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
ELK_PRECONSTRUCTOR()
{
  // Set up a simple brk system call.
  SYSCALL(brk);
  brk_ptr = (char *)round_page((uintptr_t)__end);
}
