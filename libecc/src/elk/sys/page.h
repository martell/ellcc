/*-
 * Copyright (c) 2005, Kohsuke Ohtani
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

#ifndef _page_h_
#define _page_h_

#include <types.h>
#include <limits.h>                     // For PAGE_SIZE.

/** Memory information
 */
struct meminfo {
  psize_t total;                        // Total memory size in bytes.
  psize_t free;                         // Current free memory in bytes.
  psize_t bootdisk;                     // Total size of boot disk.
};

// Memory page.
// RICH: Not defined for some processors.
#if __microblaze__ || __mips__ || __ppc__
#define PAGE_SIZE (4096 + 0)    //  Defined like this to error if defined.
#endif

#define PAGE_MASK       (PAGE_SIZE-1)
#define trunc_page(x)   ((x) & ~PAGE_MASK)
#define round_page(x)   (((x) + PAGE_MASK) & ~PAGE_MASK)

#if ELK_NAMESPACE
#define page_alloc __elk_page_alloc
#define page_free __elk_page_free
#define page_reserve __elk_page_reserve
#define page_info __elk_page_info
#define page_init __elk_page_init
#endif

paddr_t page_alloc(psize_t);
void page_free(paddr_t, psize_t);
int page_reserve(paddr_t, psize_t);
void page_info(struct meminfo *);
void page_init(void);

#endif // !_page_h_
