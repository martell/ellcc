/*	$NetBSD: funopen.c,v 1.11 2012/01/22 18:36:17 christos Exp $	*/

/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Chris Torek.
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
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)funopen.c	8.1 (Berkeley) 6/4/93";
#else
__RCSID("$NetBSD: funopen.c,v 1.11 2012/01/22 18:36:17 christos Exp $");
#endif
#endif /* LIBC_SCCS and not lint */

#include "stdio_impl.h"
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>

FILE *
funopen(const void *cookie,
	int (*readfn)(void *, char *, int),
	int (*writefn)(void *, const char *, int),
	off_t (*seekfn)(void *, off_t, int),
	int (*closefn)(void *))
{
	FILE *fp;
        const char *mode;
	int flags;

	if (readfn == NULL) {
		if (writefn == NULL) {		/* illegal */
			errno = EINVAL;
			return (NULL);
		} else
			mode = "w";	        /* write only */
	} else {
		if (writefn == NULL)
			mode = "r";	        /* read only */
		else
			flags = "r+";		/* read-write */
	}
	if ((fp = __fdopen(-1, mode)) == NULL)
		return (NULL);
	fp->cookie = __UNCONST(cookie);
	fp->read = readfn;
	fp->write = writefn;
	fp->seek = seekfn;
	fp->close = closefn;
	return (fp);
}
