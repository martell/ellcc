/** Kernel definitions.
 */

#ifndef _kernel_h_
#define _kernel_h_

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "target.h"

#ifndef NULL
#define NULL 0
#endif

#undef weak_alias
#define weak_alias(old, new) \
    extern __typeof(old) new __attribute__((weak, alias(#old)))

#undef alias
#define alias(old, new) \
    extern __typeof(old) new __attribute__((alias(#old)))

#define FEATURE(feature, function) \
char __elk_ ## feature = 0; \
alias(__elk_ ## feature, __elk_feature_ ## function);

#define USE_FEATURE(feature) do \
{ \
 extern char __elk_ ## feature; \
 __elk_ ## feature = 1; \
} while (0)

#if 0
#define CONSTRUCTOR() \
static void __elk_init(void) \
    __attribute__((__constructor__, __used__)); \
static void __elk_init(void)

#define CONSTRUCTOR_BY_NAME(returns, name) \
returns name(void) \
    __attribute__((__constructor__, __used__)); \
returns name(void)
#else
#define CONSTRUCTOR() \
static void __elk_init(void); \
static void (*__elk_init_p)(void) \
  __attribute((section (".elk_init_array"), __used__)) \
     = __elk_init; \
static void __elk_init(void)

#define CONSTRUCTOR_BY_NAME(returns, name) \
static returns name(void); \
static returns (*name ## _p)(void) \
  __attribute((section (".elk_init_array"), __used__)) \
     = name; \
static returns name(void)
#endif

typedef struct lock
{
    int lock;
    int level;
} Lock;

#define LOCK_INITIALIZER { 0, 0 }

static inline void lock_aquire(Lock *lock)
{
// RICH:
#if !defined(__microblaze__)
    while(!__atomic_test_and_set(&lock->lock, __ATOMIC_SEQ_CST))
        continue;
#endif
    lock->level = splhigh();
}

static inline void lock_release(Lock *lock)
{
    splx(lock->level);
// RICH:
#if !defined(__microblaze__)
    __atomic_clear(&lock->lock, __ATOMIC_SEQ_CST);
#endif
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
