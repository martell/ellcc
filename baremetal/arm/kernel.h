/** Kernel definitions.
 */

#ifndef _kernel_h_
#define _kernel_h_

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "arm.h"

#ifndef NULL
#define NULL 0
#endif

typedef struct lock
{
    char lock;
    int level;
} Lock;

#define LOCK_INITIALIZER { 0, 0 }

static inline void lock_aquire(Lock *lock)
{
    while(!__atomic_test_and_set(&lock->lock, __ATOMIC_SEQ_CST))
        continue;
    lock->level = splhigh();
}

static inline void lock_release(Lock *lock)
{
    splx(lock->level);
    __atomic_clear(&lock->lock, __ATOMIC_SEQ_CST);
}

/* RICH: Validate a system call address argument.
 */
enum {
    VALID_RD = 0x01,
    VALID_WR = 0x02,
    VALID_RW = 0x03,
    VALID_EX = 0x04
};

#define VALIDATE_ADDRESS(addr, size, access)

/** Set a system call handler.
 * @param nr The system call number.
 * @param fn The system call handling function.
 * @return 0 on success, -1 on  error.
 */
int __set_syscall(int nr, void *fn);

#endif // _kernel_h_
