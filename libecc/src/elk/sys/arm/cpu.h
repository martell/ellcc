/*-
 * Copyright (c) 2005-2008, Kohsuke Ohtani
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

#ifndef _cpu_h_
#define _cpu_h_

/** Processor Status Register
 */
#define PSR_MODE        0x0000001f
#define PSR_USR_MODE    0x00000010
#define PSR_FIQ_MODE    0x00000011
#define PSR_IRQ_MODE    0x00000012
#define PSR_SVC_MODE    0x00000013
#define PSR_ABT_MODE    0x00000017
#define PSR_UND_MODE    0x0000001b
#define PSR_SYS_MODE    0x0000001f

#define PSR_KRN_MODE    PSR_UND_MODE    // Kernel thread mode.
#define INITIAL_PSR     PSR_SYS_MODE

#define PSR_THUMB       0x00000020

#define PSR_INT_MASK    0x000000c0
#define PSR_FIQ_DIS     0x00000040
#define PSR_IRQ_DIS     0x00000080

#ifdef __gba__
#define PSR_APP_MODE  PSR_SYS_MODE
#else
#define PSR_APP_MODE  PSR_USR_MODE
#endif

/** Control register CP15 register 1
 */
#define CTL_MMU         0x000000001  // M: MMU/Protection unit enable.
#define CTL_AFLT        0x000000002  // A: Alignment fault enable.
#define CTL_CACHE       0x000000004  // C: Cache enable.
#define CTL_WBUF        0x000000008  // W: Write buffer enable.
#define CTL_32BP        0x000000010  // P: 32-bit exception handlers.
#define CTL_32BD        0x000000020  // D: 32-bit addressing.
#define CTL_LABT        0x000000040  // L: Late abort enable.
#define CTL_BIGEND      0x000000080  // B: Big-endian mode.
#define CTL_SYSP        0x000000100  // S: System protection bit.
#define CTL_ROMP        0x000000200  // R: ROM protection bit.
#define CTL_BPRD        0x000000800  // Z: Branch prediction enable.
#define CTL_ICACHE      0x000001000  // I: Instruction cache enable.
#define CTL_HIVEC       0x000002000  // V: Vector relocation.

#define CTL_DEFAULT (CTL_32BP|CTL_32BD|CTL_LABT)

#endif // !_cpu_h_
