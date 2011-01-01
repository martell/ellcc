/*	$NetBSD: flt_rounds.c,v 1.3 2006/02/25 00:58:35 wiz Exp $	*/

/*
 * Copyright (c) 1996 Mark Brinicombe
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Mark Brinicombe
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: flt_rounds.c,v 1.3 2006/02/25 00:58:35 wiz Exp $");
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>
#include <ieeefp.h>

static const int map[] = {
	1,	/* round to nearest */
	3,	/* round to negative infinity */
	2,	/* round to positive infinity */
	0	/* round to zero */
};

/*
 * Return the current FP rounding mode
 *
 * Returns:
 *	0 - round to zero
 *	1 - round to nearest
 *	2 - round to postive infinity
 *	3 - round to negative infinity
 *
 * ok all we need to do is get the current FP rounding mode
 * index our map table and return the appropriate value.
 */

int
__flt_rounds(void)
{
        int result;
        asm volatile ("subl $4,%%esp\n\t"
	              "fnstcw (%%esp)\n\t"
	              "movl (%%esp),%%eax\n\t"
	              "shrl $10,%%eax\n\t"
	              "andl $3,%%eax\n\t"
	              "addl $4,%%esp"
                      : "=a" (result));
	return map[result];
}
