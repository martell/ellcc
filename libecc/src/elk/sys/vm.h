/*-
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

#ifndef _vm_h_
#define _vm_h_

#include <pthread.h>

#include "types.h"
#include "bootinfo.h"
#include "mmu.h"

#if ELK_NAMESPACE
#define vm_allocate __elk_vm_allocate
#define vm_free __elk_vm_free
#define vm_attribute __elk_vm_attribute
#define vm_map __elk_vm_map
#define vm_dup __elk_vm_dup
#define vm_create __elk_vm_create
#define vm_reference __elk_vm_reference
#define vm_terminate __elk_vm_terminate
#define vm_switch __elk_vm_switch
#define vm_load __elk_vm_load
#define vm_translate __elk_vm_translate
#define vm_premap __elk_vm_premap
#define vm_info __elk_vm_info
#define vm_init __elk_vm_init
#define vm_mmu_init __elk_vm_mmu_init
#endif

/** One structure per allocated segment.
 */
struct seg
{
  struct seg *prev;             // Segment list sorted by address.
  struct seg *next;
  struct seg *sh_prev;          // Link for all shared segments.
  struct seg *sh_next;
  vaddr_t addr;                 // Base address.
  size_t size;                  // Size.
  int flags;                    // SEG_* flag.
  paddr_t phys;                 // Physical address.
};

// Segment flags.
#define SEG_READ        0x00000001
#define SEG_WRITE       0x00000002
#define SEG_EXEC        0x00000004
#define SEG_SHARED      0x00000008
#define SEG_MAPPED      0x00000010
#define SEG_FREE        0x00000080
#define SEG_ACCESS      (SEG_READ|SEG_WRITE|SEG_EXEC)

/** VM mapping for one process.
 */
typedef struct vm_map
{
  struct seg head;              // List head of segements.
  int refcnt;                   // Reference count.
  pgd_t pgd;                    // Page directory.
  size_t total;                 // Total used size.
  pthread_mutex_t lock;         // The map lock.
} *vm_map_t;

// Address translation between physical address and kernel virtual address.
#define ptokv(pa)       (void *)((paddr_t)(pa) + VIRTUAL_OFFSET)
#define kvtop(va)       ((paddr_t)(va) - VIRTUAL_OFFSET)

// These function calls are indirect to support MMU vs. non-MMU systems.
int (*vm_allocate)(pid_t, void **, size_t, int);
int (*vm_free)(pid_t, void *, size_t);
int (*vm_attribute)(pid_t, void *, size_t, int);
int (*vm_map)(pid_t, void *, size_t, void **);
vm_map_t (*vm_dup)(vm_map_t);
vm_map_t (*vm_create)(void);
int (*vm_reference)(vm_map_t);
void (*vm_terminate)(vm_map_t);
void (*vm_switch)(vm_map_t);
paddr_t (*vm_translate)(vaddr_t, size_t);
void (*vm_premap)(paddr_t, vaddr_t);

/** VM information
 */
struct vminfo
{
  u_long cookie;                // Index cookie.
  pid_t pid;                    // Process id.
  vaddr_t virt;                 // Virtual address.
  size_t size;                  // Size.
  int flags;                    // Region flags.
  paddr_t phys;                 // Physical address.
};

int (*vm_info)(struct vminfo *);
vm_map_t (*vm_init)(size_t);
void (*vm_mmu_init)(void);

#endif // !_vm_h_
