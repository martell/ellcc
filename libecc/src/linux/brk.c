#include <syscall.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stdint.h>         // For uintptr_t.

static void *current;           // The current break pointer.

int brk(void *addr)
{
    current = (void *)INLINE_SYSCALL(brk, 1, addr);
    if (current < addr) {
        __set_errno(ENOMEM);
        return -1;
    }

    return 0;
}

void *sbrk(intptr_t increment)
{
    if (current == NULL && brk(0)) {
        // Get the first brk pointer.
        return (void*)-1;
    }

    void* next = current;
    if (increment > 0) {
        if ((uintptr_t)next + (uintptr_t)increment < (uintptr_t)next) {
            // Overflow.
            return (void*)-1;
        }
    } else if ((uintptr_t)next < (uintptr_t)-increment) {
        // Underflow.
        return (void*)-1;
    }

    if (brk(current + increment)) {
        return (void*)-1;
    }
    return next;
}