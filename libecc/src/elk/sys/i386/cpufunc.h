/*-
 * Copyright (c) 2008, Kohsuke Ohtani
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

#ifndef _cpufunc_h_
#define _cpufunc_h_

#include <sys/types.h>

#if ELK_NAMESPACE
#define cpu_idle __elk_cpu_idle
#define flush_tlb __elk_flush_tlb
#define flush_cache __elk_flush_cache
#define load_tr __elk_load_tr
#define load_gdt __elk_load_gdt
#define load_idt __elk_load_idt
#define get_cr2 __elk_get_cr2
#define set_cr3 __elk_set_cr3
#define get_cr3 __elk_get_cr3
#define outb __elk_outb
#define outb_p __elk_outb_p
#define inb_p __elk_inb_p
#endif

void cpu_idle(void);
void flush_tlb(void);
void flush_cache(void);
void load_tr(uint32_t);
void load_gdt(void *);
void load_idt(void *);
uint32_t get_cr2(void);
void set_cr3(uint32_t);
uint32_t get_cr3(void);
void outb(int, u_char);
u_char inb(int);
void outb_p(int, u_char);
u_char inb_p(int);

#endif // !_cpufunc_h_
