/** Generic ARM specific definitions.
 */
#ifndef _target_h_
#define _target_h_

#include "config.h"

#if ELK_NAMESPACE
#define context_set_return __elk_context_set_return
#endif

#define Mode_USR    0x10
#define Mode_FIQ    0x11
#define Mode_IRQ    0x12
#define Mode_SVC    0x13
#define Mode_ABT    0x17
#define Mode_UND    0x1B
#define Mode_SYS    0x1F

#define INITIAL_PSR Mode_USR

#define T_bit 0x20
#define F_bit 0x40
#define I_bit 0x80

#if !defined(__ASSEMBLER__)

#if RICH
typedef struct context
{
  union {
    uint32_t r0;
    uint32_t a1;
  };
  union {
    uint32_t r1;
    uint32_t a2;
  };
  union {
    uint32_t r2;
    uint32_t a3;
  };
  union {
    uint32_t r3;
    uint32_t a4;
  };
  union {
    uint32_t r4;
    uint32_t v1;
  };
  union {
    uint32_t r5;
    uint32_t v2;
  };
  union {
    uint32_t r6;
    uint32_t v3;
  };
  union {
    uint32_t r7;
    uint32_t v4;
  };
  union {
    uint32_t r8;
    uint32_t v5;
  };
  union {
    uint32_t r9;
    uint32_t v6;
  };
  union {
    uint32_t r10;
    uint32_t v7;
  };
  union {
    uint32_t r11;
    uint32_t v8;
  };
  union {
    uint32_t r12;
    uint32_t ip;
  };
  union {
    uint32_t r14;
    uint32_t lr;
  };
  union {
    uint32_t r13;
    uint32_t sp;
  };
  uint32_t pad;
  union {
    uint32_t r15;
    uint32_t pc;
  };
  uint32_t cpsr;
} context_t;

static inline void context_set_return(context_t *cp, int value)
{
  cp->a1 = value;
}
#endif

#endif // !defined(__ASSEMBLER__)

#endif // _target_h_
