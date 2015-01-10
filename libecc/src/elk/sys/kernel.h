/** Kernel definitions.
 */

#ifndef _kernel_h_
#define _kernel_h_

#include <stdint.h>
#include <stdlib.h>

#include "config.h"
#include "hal.h"
#include "constructor.h"

#ifndef NULL
#define NULL 0
#endif

#define __packed __attribute__((__packed__))

#if ELK_NAMESPACE
#define system_init __elk_system_init
#define system_c_init __elk_system_c_init
#endif

/** An optiona system initialization function.
 * Called from __elk_start() before C library initialization.
 */
void (*system_init)(void);

/** An optiona system initialization function.
 * Called from __elk_start() after C library initialization.
 */
void (*system_c_init)(void);

#define ELK_ASSERT 1
#if ELK_ASSERT
#include <assert.h>
#define ASSERT(x) assert(x)
#else
#define ASSERT(x)
#endif

#define ELK_DEBUG 1
#ifdef ELK_DEBUG

#undef DPRINTF

#define debug __elk_debug
extern int debug;

// Always send a message.
#define MSG             0x00000000

// Thread debug flags.
#define THRDB_CORE      0x00000001
#define THRDB_FAULT     0x00000002

// Memory debug flags.
#define MEMDB_CORE      0x00000010
#define MEMDB_KMEM      0x00000020
#define MEMDB_VM        0x00000040
#define MEMDB_MMAP      0x00000080

// Device debug flags.
#define DEVDB_CORE      0x00000100
#define DEVDB_IRQ       0x00000200
#define DEVDB_DRV       0x00000400

// TTY debug flags.
#define TTYDB_CORE      0x00001000

// Virtual file system debug flags.
#define	VFSDB_CORE	0x00010000
#define	VFSDB_SYSCALL	0x00020000
#define	VFSDB_VNODE	0x00040000
#define	VFSDB_BIO	0x00080000
#define	VFSDB_CAP	0x00100000
#define VFSDB_FLAGS	0x00130000

// Additional file system debug flags.
#define AFSDB_CORE      0x01000000

// Network debug flags.
#define NETDB_CODE      0x10000000
#define NETDB_INET      0x20000000
#define NETDB_BSD       0x40000000

#if 0
  #include <stdio.h>
  #define DPRINTF(_m,X)	if (!_m || (debug & (_m))) printf X
#else
  #include "hal.h"
  #define DPRINTF(_m,X)	if (!_m || (debug & (_m))) diag_printf X
#endif

#else
#define	DPRINTF(_m, X)
#endif  // !ELK_DEBUG

extern int puts(const char *s);
#define panic(arg) do { puts(arg); for ( ;; ); } while(1)

/* RICH: Validate a system call address argument.
 */
enum {
  VALID_RD = 0x01,
  VALID_WR = 0x02,
  VALID_RW = 0x03,
  VALID_EX = 0x04
};

#define VALIDATE_ADDRESS(addr, size, access)

#endif // _kernel_h_
