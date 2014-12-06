/** Generic Mips specific definitions.
 */
#ifndef _target_h_
#define _target_h_

#include "config.h"

#if ELK_NAMESPACE
#define context_set_return __elk_context_set_return
#endif

#define INITIAL_PSR 0 // RICH

// Saved context register offsets.
#define CTX_R1  0
#define CTX_R2  4
#define CTX_R3  8
#define CTX_R4  12
#define CTX_R5  16
#define CTX_R6  20
#define CTX_R7  24
#define CTX_R8  28
#define CTX_R9  32
#define CTX_R10 36
#define CTX_R11 40
#define CTX_R12 44
#define CTX_R13 48
#define CTX_R14 52
#define CTX_R15 56
#define CTX_R16 60
#define CTX_R17 64
#define CTX_R18 68
#define CTX_R19 72
#define CTX_R20 76
#define CTX_R21 80
#define CTX_R22 84
#define CTX_R23 88
#define CTX_R24 92
#define CTX_R25 96
#define CTX_GP  100
#define CTX_SP  104
#define CTX_FP  108
#define CTX_RA  112
#define CTX_LO  116
#define CTX_HI  120
#define CTX_CP0_STATUS 124
#define CTX_PC  128

#define CTX_SIZE 132            // Size of the saved context.

#if !defined(__ASSEMBLER__)

#if RICH
typedef struct context
{
  union {
    uint32_t r1;
    uint32_t at;
  };
  union {
    uint32_t r2;
    uint32_t v0;
  };
  union {
    uint32_t r3;
    uint32_t v1;
  };
  union {
    uint32_t r4;
    uint32_t a0;
  };
  union {
    uint32_t r5;
    uint32_t a1;
  };
  union {
    uint32_t r6;
    uint32_t a2;
  };
  union {
    uint32_t r7;
    uint32_t a3;
  };
  union {
    uint32_t r8;
    uint32_t t0;
  };
  union {
    uint32_t r9;
    uint32_t t1;
  };
  union {
    uint32_t r10;
    uint32_t t2;
  };
  union {
    uint32_t r11;
    uint32_t t3;
  };
  union {
    uint32_t r12;
    uint32_t t4;
  };
  union {
    uint32_t r13;
    uint32_t t5;
  };
  union {
    uint32_t r14;
    uint32_t t6;
  };
  union {
    uint32_t r15;
    uint32_t t7;
  };
  union {
    uint32_t r16;
    uint32_t s0;
  };
  union {
    uint32_t r17;
    uint32_t s1;
  };
  union {
    uint32_t r18;
    uint32_t s2;
  };
  union {
    uint32_t r19;
    uint32_t s3;
  };
  union {
    uint32_t r20;
    uint32_t s4;
  };
  union {
    uint32_t r21;
    uint32_t s5;
  };
  union {
    uint32_t r22;
    uint32_t s6;
  };
  union {
    uint32_t r23;
    uint32_t s7;
  };
  union {
    uint32_t r24;
    uint32_t k0;
  };
  union {
    uint32_t r25;
    uint32_t k1;
  };
  uint32_t gp;
  uint32_t sp;
  uint32_t fp;
  uint32_t ra;
  uint32_t lo;
  uint32_t hi;
  uint32_t cp0_status;
  uint32_t pc;
} context_t;

static inline void context_set_return(context_t *cp, int value)
{
  cp->v0 = value;
}
#endif

#endif // !defined(__ASSEMBLER__)

#endif // _target_h_
