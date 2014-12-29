/*
 * Copyright (c) 2005-2007, Kohsuke Ohtani
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

/*
 * Boot information
 *
 * The boot information is set by an OS loader, and
 * is accessed by kernel later during boot time.
 */

#ifndef _bootinfo_h_
#define _bootinfo_h_

#include <sys/types.h>

#include "config.h"
#include "types.h"

/** Video information
 */
struct vidinfo
{
  int pixel_x;                          // Screen pixels.
  int pixel_y;
  int text_x;                           // Text size, in characters.
  int text_y;
};

/** Physical memory
 */
struct physmem
{
  paddr_t base;                         // Start address.
  psize_t size;                         // Size in bytes.
  int type;                             // Type.
};

// Memory types.
#define MT_USABLE               1
#define MT_MEMHOLE              2
#define MT_RESERVED             3

#define NMEMS                   8       // Max number of memory slots.

/* Boot information
 */
#if ELK_NAMESPACE
#define bootinfo __elk_bootinfo
#endif

struct bootinfo
{
  struct vidinfo video;                 // Video information.
  int nr_rams;                          // Number of RAM blocks.
  struct physmem ram[NMEMS];            // Physical RAM table.
};

// These symbols are defined at link time.
extern char __user_limit__[];
extern char __kernel_base__[];
extern char __virtual_offset__[];
extern char __mmu_enabled__[];
extern char __device_map_offset__[];
#define USERLIMIT ((paddr_t)__user_limit__)
#define KERNBASE ((paddr_t)__kernel_base__)
#define DEVICEBASE ((paddr_t)__device_base__)
#define VIRTUAL_OFFSET ((paddr_t)__virtual_offset__)
#define user_area(a) (((vaddr_t)(a) < (vaddr_t)USERLIMIT))
#define mmu_enabled() ((int)(intptr_t)__mmu_enabled__)

extern struct bootinfo bootinfo;

#endif // !_bootinfo_h_
