/*-
 * Copyright (c) 2009, Kohsuke Ohtani
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
 * 3. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _trap_h_
#define _trap_h_

#include "config.h"
#include "context.h"

/*
 * Trap ID
 */
#define TRAP_SYSTEM_RESET	1
#define TRAP_MACHINE_CHECK	2
#define TRAP_DSI		3
#define TRAP_ISI		4
#define TRAP_EXT_INTERRUPT	5
#define TRAP_ALIGNMENT		6
#define TRAP_PROGRAM		7
#define TRAP_FP_UNAVAILABLE	8
#define TRAP_DECREMENTER	9
#define TRAP_RESERVED0		10
#define TRAP_RESERVED1		11
#define TRAP_SYSTEM_CALL	12
#define TRAP_PPC_TRACE		13
#define TRAP_FP_ASSIST		14


#ifndef __ASSEMBLER__

#if ELK_NAMESPACE
#define trap_handler __elk_trap_handler
#define trap_dump __elk_trap_dump
#endif

int trap_handler(u_long, context_t *);
void trap_dump(int (*)(const char *__restrict, ...), const char *, context_t *);

#endif // !__ASSEMBLER__
#endif // !_trap_h_

