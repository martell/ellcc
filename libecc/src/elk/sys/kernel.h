/** Kernel definitions.
 */

#ifndef _kernel_h_
#define _kernel_h_

#include <stdint.h>
#include <stdlib.h>
#include "target.h"
#include "constructor.h"

#if RICH
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include "config.h"
#endif


#ifndef NULL
#define NULL 0
#endif

#define SYSCALL(name) __elk_set_syscall(SYS_ ## name, sys_ ## name)

// RICH: For now.
#include <assert.h>
#define ASSERT(arg) assert(arg)
#define DPRINTF(arg) printf arg
#define copyinstr(src, dst, len) strlcpy(dst, src, len)
#define copyout(dst, src, len) (memcpy(dst, src, len), len)
#define copyin(src, dst, len) (memcpy(dst, src, len), len)
#define user_area(buf) 1
extern int puts(const char *s);
#define panic(arg) do { puts(arg); exit(1); } while(1)

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
 * This function is defined in crt1.S.
 */
int __elk_set_syscall(int nr, void *fn);

#endif // _kernel_h_
