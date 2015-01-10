
/*
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Berkeley Software Design, Inc.
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
 *	@(#)cdefs.h	8.8 (Berkeley) 1/9/95
 */

/*
 * This is a much truncated version of the NetBSD version of cdefs.h. It
 * is not meant to be used in system header files and is only provided to
 * allow BSD sources to compile unmodified. The only definitions that should
 * be here are those that typical BSD userland sources need to compile.
 */
#ifndef	_SYS_CDEFS_H_
#define	_SYS_CDEFS_H_

#define _BSD_SOURCE
#define _POSIX_SOURCE
#define _GNU_SOURCE
#define __COPYRIGHT(arg)
#define __RCSID(arg)
#define __KERNEL_RCSID(arg1, arg2)
#define __IDSTRING(arg1, arg2)
extern char *__progname;
#define setprogname(arg) (__progname = arg)
#define getprogname() __progname
#define _DIAGASSERT(arg)

#if defined(__cplusplus)
#define __BEGIN_DECLS           extern "C" {
#define __END_DECLS             }
#else
#define __BEGIN_DECLS
#define __END_DECLS
#endif

#define __dead
#define  __arraycount(__x) (sizeof(__x) / sizeof(__x[0]))
#define __unused __attribute__((__unused__))

#define MAXBSIZE 8192

#define __RENAME(f)
#define __printflike(a, b) __attribute__ ((__format__ (__printf__, (a), (b))))
#define __UNCONST(a)    ((void *)(unsigned long)(const void *)(a))
#define __UNVOLATILE(a)    ((void *)(unsigned long)(volatile void *)(a))
#define strtoq strtoll
#define SIZE_T_MAX SIZE_MAX

#define TIOCGSIZE TIOCGWINSZ
#define ttysize winsize
#define ts_cols ws_col
#define ts_rows ws_row
#define ts_lines ws_row
#define INFTIM (-1)
#define IPPORT_ANONMAX (65536)
#define EFTYPE EINVAL
#define GLOB_BRACE 0
#define GLOB_TILDE 0
#define MAXLOGNAME LOGIN_NAME_MAX
#define __mode_t mode_t
#define MAXNAMLEN 255

#include <stdint.h>
#include <inttypes.h>
#include <sys/types.h>

/*
 * The __CONCAT macro is used to concatenate parts of symbol names, e.g.
 * with "#define OLD(foo) __CONCAT(old,foo)", OLD(foo) produces oldfoo.
 * The __CONCAT macro is a bit tricky -- make sure you don't put spaces
 * in between its arguments.  __CONCAT can also concatenate double-quoted
 * strings produced by the __STRING macro, but this only works with ANSI C.
 */

#define ___STRING(x)    __STRING(x)
#define ___CONCAT(x,y)  __CONCAT(x,y)

#define __P(protos)     protos          /* full-blown ANSI C */
#define __CONCAT(x,y)   x ## y
#define __STRING(x)     #x

#include <sys/sysmacros.h>
#include <sys/types.h>

#define _PATH_DEFTAPE "/dev/tape"
#define _PATH_CSHELL "/bin/csh"

#define CTRL(x) ((x)&0x1F)

#define GLOB_BRACE 0
#define GLOB_TILDE 0


#endif /* !_SYS_CDEFS_H_ */
