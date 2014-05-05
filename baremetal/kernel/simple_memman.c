/* Initialize a simple memory allocation handler.
 */
#include <unistd.h>
#include <bits/syscall.h>       // For syscall numbers.
#include <sys/uio.h>            // For writev (used by printf().
#include <sys/ioctl.h>

#include "kernel.h"

extern char __end[];            // The end of the .bss area.
extern char *__heap_end;        // The bottom of the allocated stacks.

static char *brk_ptr = __end;

static char *sys_brk(char *addr)
{
    if (addr == 0) {
        return brk_ptr;         // Return the current value.
    }
    if (addr >= __end && addr < __heap_end) {
        // A good request, update the pointer.
        brk_ptr = addr;
    }
    return brk_ptr;
}

/* Initialize the simple memory allocator.
 */
static void init(void)
    __attribute__((__constructor__, __used__));

static void init(void)
{
    // Set up a simple brk system call.
    __set_syscall(SYS_brk, sys_brk);
}
