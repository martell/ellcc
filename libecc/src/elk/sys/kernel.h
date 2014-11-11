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

#undef strong_alias
#define strong_alias(old, new) \
    extern __typeof(old) new __attribute__((alias(#old)))

#define FEATURE(feature, function) \
char __elk_ ## feature = 0; \
strong_alias(__elk_ ## feature, __elk_feature_ ## function);

#define USE_FEATURE(feature) do \
{ \
 extern char __elk_ ## feature; \
 __elk_ ## feature = 1; \
} while (0)

#if !defined(__ELK__)
// Building ELK to run under Linux.
// We'll use C constructors for everything.
#define ELK_CONSTRUCTOR() \
static void __elk_init(void) \
    __attribute__((__constructor__, __used__)); \
static void __elk_init(void)

#define ELK_CONSTRUCTOR_BY_NAME(returns, name) \
returns name(void) \
    __attribute__((__constructor__, __used__)); \
returns name(void)

#define C_CONSTRUCTOR() \
static void __elk_c_init(void) \
    __attribute__((__constructor__, __used__)); \
static void __elk_c_init(void)

#define C_CONSTRUCTOR_BY_NAME(returns, name) \
returns name(void) \
    __attribute__((__constructor__, __used__)); \
returns name(void)

#else
// Building ELK to run bare bones.
// The ELK constructors come before C library initialization.

#define ELK_CONSTRUCTOR() \
static void __elk_init(void); \
static void (*__elk_init_p)(void) \
  __attribute((section (".elk_init_array"), __used__)) \
     = __elk_init; \
static void __elk_init(void)

#define ELK_CONSTRUCTOR_BY_NAME(returns, name) \
returns name(void); \
static returns (*name ## _p)(void) \
  __attribute((section (".elk_init_array"), __used__)) \
     = name; \
returns name(void)

#define C_CONSTRUCTOR() \
static void __elk_c_init(void) \
    __attribute__((__constructor__, __used__)); \
static void __elk_c_init(void)

#define C_CONSTRUCTOR_BY_NAME(returns, name) \
returns name(void) \
    __attribute__((__constructor__, __used__)); \
returns name(void)

#endif

typedef struct
{
  int lock;
  int level;
} lock_t;

#define LOCK_INITIALIZER { 0, 0 }

static inline void __elk_lock_aquire(lock_t *lock)
{
// RICH:
#if !defined(__microblaze__)
  while(!__atomic_test_and_set(&lock->lock, __ATOMIC_SEQ_CST))
      continue;
#endif
  lock->level = splhigh();
}

static inline void __elk_lock_release(lock_t *lock)
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
int __elk_set_syscall(int nr, void *fn);

#endif // _kernel_h_
