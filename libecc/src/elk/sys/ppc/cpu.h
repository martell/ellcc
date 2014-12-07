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

#ifndef _cpu_h_
#define _cpu_h_

#define INITIAL_PSR 0 // RICH

#define STKFRAME_LEN    8               // Length for stack frame.

/** Flags in MSR
 */
#define MSR_LE          0x00000001      // Little-endian mode enable.
#define MSR_RI          0x00000002      // Recoverable exception.
#define MSR_DR          0x00000010      // Data address translation.
#define MSR_IR          0x00000020      // Instruction address translation.
#define MSR_IP          0x00000040      // Exception prefix.
#define MSR_FE1         0x00000100      // Floating-point exception mode 1.
#define MSR_BE          0x00000200      // Branch trace enable.
#define MSR_SE          0x00000400      // Single-step trace enable.
#define MSR_FE0         0x00000800      // Floating-point exception mode 0.
#define MSR_ME          0x00001000      // Machine check enable.
#define MSR_FP          0x00002000      // Floating-point available.
#define MSR_PR          0x00004000      // Privilege level (1:USR).
#define MSR_EE          0x00008000      // External interrupt enable.
#define MSR_ILE         0x00010000      // Exception little-endian mode (1:LE).
#define MSR_POW         0x00040000      // Power management enable.

// Default msr for starting a user program.
#if 0
#ifdef CONFIG_MMU
  #define MSR_DFLT  (uint32_t)(MSR_EE|MSR_PR|MSR_ME|MSR_IR|MSR_DR)
#else
  #define MSR_DFLT  (uint32_t)(MSR_EE|MSR_PR|MSR_ME)
#endif
#endif

#define MSR_DFLT  (uint32_t)(MSR_EE|MSR_PR)

/** Special Purpose Register declarations.
 */
#define SPR_XER         1               // Fixed point exception register.
#define SPR_LR          8               // Link register.
#define SPR_CTR         9               // Count register.
#define SPR_DSISR       18              // DSI exception register.
#define SPR_DAR         19              // Data access register.
#define SPR_DEC         22              // Decrementer register.
#define SPR_SRR0        26              // Save/restore register 0.
#define SPR_SRR1        27              // Save/restore register 1.
#define SPR_SPRG0       272             // SPR general 0.
#define SPR_SPRG1       273             // SPR general 1.
#define SPR_SPRG2       274             // SPR general 2.
#define SPR_SPRG3       275             // SPR general 3.
#define SPR_PVR         287             // Processor version register.

#ifndef __ASSEMBLER__

#if ELK_NAMESPACE
#define outb __elk_outb
#define inb __elk_inb
#endif

void outb(int, u_char);
u_char inb(int);

#endif // !__ASSEMBLER__

#endif // !_cpu_h_
