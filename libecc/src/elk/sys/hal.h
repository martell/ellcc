/*
 * Copyright (c) 2005-2009, Kohsuke Ohtani
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

#ifndef _hal_h_
#define _hal_h_

#include <sys/types.h>
#include "types.h"
#include "mmu.h"
#include "bootinfo.h"

#if RICH
#include <context.h>
#endif

#define NO_PGD    ((pgd_t)0)    // Non-existent pgd.

// Types for context_set().
#define CTX_KSTACK      0       // Set kernel mode stack address.
#define CTX_KENTRY      1       // Set kernel mode entry address.
#define CTX_KARG        2       // Set kernel mode argument.
#define CTX_USTACK      3       // Set user mode stack address.
#define CTX_UENTRY      4       // Set user mode entry addres.
#define CTX_UARG        5       // Set user mode argument.

// Page types for mmu_map().
#define PG_UNMAP        0       // No page.
#define PG_READ         1       // User - read only.
#define PG_WRITE        2       // User - read/write.
#define PG_SYSTEM       3       // System.
#define PG_IOMEM        4       // System - no cache.

// Virtual/physical address mapping.
struct mmumap
{
  vaddr_t virt;                 // Virtual address.
  paddr_t phys;                 // Physical address.
  psize_t size;                 // Size.
  int type;                     // Mapping type.
};

#define AUTOSIZE  0

// type of virtual memory mappings
#define VMT_NULL        0
#define VMT_RAM         1
#define VMT_ROM         2
#define VMT_DMA         3
#define VMT_IO          4


// Return value of an ISR.
#define INT_DONE        0       // Done.
#define INT_ERROR       1       // Error.
#define INT_CONTINUE    2       // Continue to IST.

// No IST for irq_attach().
#define IST_NONE        ((void (*)(void *)) -1)

// Interrupt mode for interrupt_setup().
#define IMODE_EDGE      0       // Edge trigger.
#define IMODE_LEVEL     1       // Level trigger.


#if ELK_NAMESPACE
#define interrupt_mask __elk_interrupt_mask
#define interrupt_unmask __elk_interrupt_unmask
#define interrupt_setup __elk_interrupt_setup
#define interrupt_init __elk_interrupt_init
#endif

void interrupt_mask(int);
void interrupt_unmask(int, int);
void interrupt_setup(int, int);
void interrupt_init(void);

#if ELK_NAMESPACE
#define mmu_init __elk_mmu_init
#define mmu_premap __elk_mmu_premap
#define mmu_newmap __elk_mmu_newmap
#define mmu_terminate __elk_mmu_terminate
#define mmu_map __elk_mmu_map
#define mmu_switch __elk_mmu_switch
#define mmu_extract __elk_mmu_extract
#endif

void mmu_init(const struct mmumap *);
void mmu_premap(paddr_t, vaddr_t);
pgd_t mmu_newmap(void);
void mmu_terminate(pgd_t);
int mmu_map(pgd_t, paddr_t, vaddr_t, size_t, int);
void mmu_switch(pgd_t);
paddr_t mmu_extract(pgd_t, vaddr_t, size_t);

#if RICH
void context_set(context_t, int, register_t);
void context_switch(context_t, context_t);
void context_save(context_t);
void context_restore(context_t);
void context_dump(context_t);
void syscall_ret(void);
#endif

#if ELK_NAMESPACE
#define copyin __elk_copyin
#define copyout __elk_copyout
#define copyinstr __elk_copyinstr
#endif

int copyin(const void *, void *, size_t);
int copyout(const void *, void *, size_t);
int copyinstr(const void *, void *, size_t);

#if ELK_NAMESPACE
#define splhigh __elk_splhigh
#define spl0 __elk_spl0
#define splx __elk_splx
#endif

int splhigh(void);
int spl0(void);
void splx(int);

#if ELK_NAMESPACE
#define machine_startup __elk_machine_startup
#define machine_idle __elk_machine_idle
#define machine_powerdown __elk_machine_powerdown
#define machine_abort __elk_machine_abort
#define machine_bootinfo __elk_machine_bootinfo
#endif

void machine_startup(void);
void machine_idle(void);
void machine_powerdown(int);
void machine_abort(void);
void machine_bootinfo(struct bootinfo **);

#ifdef HAL_NAMESPACE
#define clock_init __elk_clock_init
#endif

void clock_init(void);

#if ELK_NAMESPACE
#define interrupt_mask __elk_interrupt_mask
#define interrupt_unmask __elk_interrupt_unmask
#define interrupt_setup __elk_interrupt_setup
#define interrupt_init __elk_interrupt_init
#define mmu_init __elk_mmu_init
#define mmu_premap __elk_mmu_premap
#define mmu_newmap __elk_mmu_newmap
#define mmu_terminate __elk_mmu_terminate
#define mmu_map __elk_mmu_map
#define mmu_switch __elk_mmu_switch
#define mmu_extract __elk_mmu_extract
#endif

#ifdef HAL_NAMESPACE
#define diag_puts __elk_diag_puts
#define diag_printf __elk_diag_printf
#endif

void diag_init(void);
void diag_puts(char *);
int diag_printf(const char *__restrict, ...);

#endif // !_hal_h_
