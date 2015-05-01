/* ===-- assembly.h - libUnwind assembler support macros -------------------===
 *
 *                     The LLVM Compiler Infrastructure
 *
 * This file is dual licensed under the MIT and the University of Illinois Open
 * Source Licenses. See LICENSE.TXT for details.
 *
 * ===----------------------------------------------------------------------===
 *
 * This file defines macros for use in libUnwind assembler source.
 * This file is not part of the interface of this library.
 *
 * ===----------------------------------------------------------------------===
 */

#ifndef UNWIND_ASSEMBLY_H
#define UNWIND_ASSEMBLY_H

#if (defined(__POWERPC__) || defined(__powerpc__) || defined(__ppc__)) && \
  !defined(__linux__)
#define SEPARATOR @
#elif defined(__arm64__)
#define SEPARATOR %%
#else
#define SEPARATOR ;
#endif

#if (defined(__POWERPC__) || defined(__powerpc__) || defined(__ppc__)) && \
  defined(__linux__)
  #define r0 0
  #define r1 1
  #define r2 2
  #define r3 3
  #define r4 4
  #define r5 5
  #define r6 6
  #define r7 7
  #define r8 8
  #define r9 9
  #define r10 10
  #define r11 11
  #define r12 12
  #define r13 13
  #define r14 14
  #define r15 15
  #define r16 16
  #define r17 17
  #define r18 18
  #define r19 19
  #define r20 20
  #define r21 21
  #define r22 22
  #define r23 23
  #define r24 24
  #define r25 25
  #define r26 26
  #define r27 27
  #define r28 28
  #define r29 29
  #define r30 30
  #define r31 31
  #define f0 0
  #define f1 1
  #define f2 2
  #define f3 3
  #define f4 4
  #define f5 5
  #define f6 6
  #define f7 7
  #define f8 8
  #define f9 9
  #define f10 10
  #define f11 11
  #define f12 12
  #define f13 13
  #define f14 14
  #define f15 15
  #define f16 16
  #define f17 17
  #define f18 18
  #define f19 19
  #define f20 20
  #define f21 21
  #define f22 22
  #define f23 23
  #define f24 24
  #define f25 25
  #define f26 26
  #define f27 27
  #define f28 28
  #define f29 29
  #define f30 30
  #define f31 31
  #define v0 0
  #define v1 1
  #define v2 2
  #define v3 3
  #define v4 4
  #define v5 5
  #define v6 6
  #define v7 7
  #define v8 8
  #define v9 9
  #define v10 10
  #define v11 11
  #define v12 12
  #define v13 13
  #define v14 14
  #define v15 15
  #define v16 16
  #define v17 17
  #define v18 18
  #define v19 19
  #define v20 20
  #define v21 21
  #define v22 22
  #define v23 23
  #define v24 24
  #define v25 25
  #define v26 26
  #define v27 27
  #define v28 28
  #define v29 29
  #define v30 30
  #define v31 31
#endif

#if defined(__APPLE__)
#define HIDDEN_DIRECTIVE .private_extern
#else
#define HIDDEN_DIRECTIVE .hidden
#endif

#define GLUE2(a, b) a ## b
#define GLUE(a, b) GLUE2(a, b)
#define SYMBOL_NAME(name) GLUE(__USER_LABEL_PREFIX__, name)

#if defined(__APPLE__)
#define SYMBOL_IS_FUNC(name)
#elif defined(__ELF__)
#if defined(__arm__)
#define SYMBOL_IS_FUNC(name) .type name,%function
#else
#define SYMBOL_IS_FUNC(name) .type name,@function
#endif
#else
#define SYMBOL_IS_FUNC(name)                                                   \
  .def name SEPARATOR                                                          \
    .scl 2 SEPARATOR                                                           \
    .type 32 SEPARATOR                                                         \
  .endef
#endif

#define DEFINE_LIBUNWIND_FUNCTION(name)                   \
  .globl SYMBOL_NAME(name) SEPARATOR                      \
  SYMBOL_IS_FUNC(SYMBOL_NAME(name)) SEPARATOR             \
  SYMBOL_NAME(name):

#define DEFINE_LIBUNWIND_PRIVATE_FUNCTION(name)           \
  .globl SYMBOL_NAME(name) SEPARATOR                      \
  HIDDEN_DIRECTIVE SYMBOL_NAME(name) SEPARATOR            \
  SYMBOL_IS_FUNC(SYMBOL_NAME(name)) SEPARATOR             \
  SYMBOL_NAME(name):

#if defined(__arm__)
#if !defined(__ARM_ARCH)
#define __ARM_ARCH 4
#endif

#if defined(__ARM_ARCH_4T__) || __ARM_ARCH >= 5
#define ARM_HAS_BX
#endif

#ifdef ARM_HAS_BX
#define JMP(r) bx r
#else
#define JMP(r) mov pc, r
#endif
#endif /* __arm__ */

#endif /* UNWIND_ASSEMBLY_H */
