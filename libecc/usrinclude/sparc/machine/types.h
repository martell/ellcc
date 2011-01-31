/*	$NetBSD: types.h,v 1.45 2009/12/14 00:46:05 matt Exp $	*/

/*-
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)types.h	8.3 (Berkeley) 1/5/94
 */

#ifndef	_MACHTYPES_H_
#define	_MACHTYPES_H_

#include <sys/cdefs.h>
#include <sys/featuretest.h>

/*
 * 7.18.1 Integer types
 */

/* 7.18.1.1 Exact-width integer types */

typedef signed char              __int8_t;
typedef unsigned char           __uint8_t;
typedef short int               __int16_t;
typedef unsigned short int     __uint16_t;
typedef int                     __int32_t;
typedef unsigned int           __uint32_t;
#ifdef __COMPILER_INT64__
typedef __COMPILER_INT64__      __int64_t;
typedef __COMPILER_UINT64__    __uint64_t;
#elif defined(_LP64)
typedef long int                __int64_t;
typedef unsigned long int      __uint64_t;
#else
/* LONGLONG */
typedef long long int           __int64_t;
/* LONGLONG */
typedef unsigned long long int __uint64_t;
#endif

#define __BIT_TYPES_DEFINED__

/* 7.18.1.4 Integer types capable of holding object pointers */

#ifndef __mips_o32
typedef long int               __intptr_t;
typedef unsigned long int     __uintptr_t;
#else
typedef int                    __intptr_t;
typedef unsigned int          __uintptr_t;
#endif

/*
 * Note that mips_reg_t is distinct from the register_t defined
 * in <types.h> to allow these structures to be as hidden from
 * the rest of the operating system as possible.
 */


/* NB: This should probably be if defined(_KERNEL) */
#if defined(_NETBSD_SOURCE)
#if defined(_MIPS_PADDR_T_64BIT) || defined(_LP64)
typedef __uint64_t	paddr_t;
typedef __uint64_t	psize_t;
#define PRIxPADDR	PRIx64
#define PRIxPSIZE	PRIx64
#define PRIdPSIZE	PRId64
#else
typedef __uint32_t	paddr_t;
typedef __uint32_t	psize_t;
#define PRIxPADDR	PRIx32
#define PRIxPSIZE	PRIx32
#define PRIdPSIZE	PRId32
#endif
#ifdef _LP64
typedef __uint64_t	vaddr_t;
typedef __uint64_t	vsize_t;
#define PRIxVADDR	PRIx64
#define PRIxVSIZE	PRIx64
#define PRIdVSIZE	PRId64
#else
typedef __uint32_t	vaddr_t;
typedef __uint32_t	vsize_t;
#define PRIxVADDR	PRIx32
#define PRIxVSIZE	PRIx32
#define PRIdVSIZE	PRId32
#endif
#endif

/* Make sure this is signed; we need pointers to be sign-extended. */
#if defined(__mips_o64) || defined(__mips_o32)
typedef	__uint32_t	fpregister_t;
typedef	__uint32_t	mips_fpreg_t;		/* do not use */
#else
typedef	__uint64_t	fpregister_t;
typedef	__uint64_t	mips_fpreg_t;		/* do not use */
#endif
#if defined(__mips_o32)
typedef __int32_t	register_t;
typedef __uint32_t	uregister_t;
typedef __int32_t	mips_reg_t;		/* do not use */
typedef __uint32_t	mips_ureg_t;		/* do not use */
#define PRIxREGISTER	PRIx32
#define PRIxUREGISTER	PRIx32
#else
typedef __int64_t	register_t;
typedef __uint64_t	uregister_t;
typedef __int64_t	mips_reg_t;		/* do not use */
typedef __uint64_t	mips_ureg_t;		/* do not use */
typedef __int64_t	register32_t;
typedef __uint64_t	uregister32_t;
#define PRIxREGISTER	PRIx64
#define PRIxUREGISTER	PRIx64
#endif /* __mips_o32 */

#if defined(_KERNEL) || defined(_NETBSD_SOURCE)
typedef struct label_t {
	register_t val[14];
} label_t;
#define	_L_S0		0
#define	_L_S1		1
#define	_L_S2		2
#define	_L_S3		3
#define	_L_S4		4
#define	_L_S5		5
#define	_L_S6		6
#define	_L_S7		7
#define	_L_GP		8
#define	_L_SP		9
#define	_L_S8		10
#define	_L_RA		11
#define	_L_SR		12
#endif

typedef	volatile int		__cpu_simple_lock_t;

#define	__SIMPLELOCK_LOCKED	1
#define	__SIMPLELOCK_UNLOCKED	0

#define	__HAVE_AST_PERPROC
#define	__HAVE_SYSCALL_INTERN
#define	__HAVE_PROCESS_XFPREGS
#ifdef MIPS3_PLUS	/* XXX bogus! */
#define	__HAVE_CPU_COUNTER
#endif

#if !defined(__mips_o32)
#define	__HAVE_ATOMIC64_OPS
#endif

#if defined(_KERNEL)
#define	__HAVE_RAS
#endif

#endif	/* _MACHTYPES_H_ */