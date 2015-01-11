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

/*
 * page.c - physical page allocator
 */

/*
 * Simple list-based page allocator:
 *
 * When the remaining page is exhausted, what should we do ?
 * If the system can stop with panic() here, the error check of
 * many portions in kernel is not necessary, and kernel code can
 * become more simple. But, in general, even if a page is
 * exhausted, a kernel can not be stopped but it should return an
 * error and continue processing.  If the memory becomes short
 * during boot time, kernel and drivers can use panic() in that
 * case.
 */

#include <errno.h>
#include <stdio.h>
#include <pthread.h>

#include "kernel.h"
#include "vm.h"
#include "bootinfo.h"
#include "command.h"
#include "page.h"

/** The page structure is put on the head of the first page of
 * each free block.
 */
struct page
{
  struct page *next;
  struct page *prev;
  vsize_t size;    			// Number of bytes of this block.
};

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
#define LOCK()  pthread_mutex_lock(&mutex)
#define UNLOCK()  pthread_mutex_unlock(&mutex)

static struct page page_head;  		// First free block.
static psize_t total_size;  		// Size of memory in the system.
static psize_t used_size;  		// Current used size.

/** Allocate continuous pages of the specified size.
 *
 * This routine returns the physical address of a new free page
 * block, or returns NULL on failure. The requested size is
 * automatically round up to the page boundary.  The allocated
 * memory is _not_ filled with 0.
 */
paddr_t page_alloc(psize_t psize)
{
  struct page *blk, *tmp;
  vsize_t size;

  ASSERT(psize != 0);

  LOCK();

  // * Find the free block that has enough size.
  size = round_page(psize);
  blk = &page_head;
  do {
    blk = blk->next;
    if (blk == &page_head) {
      UNLOCK();
      DPRINTF(MEMDB_CORE, ("page_alloc: out of memory\n"));
      return 0;  /* Not found. */
    }
  } while (blk->size < size);

  /* If found block size is exactly as requested,
   * just remove it from a free list. Otherwise, the
   * found block is divided into two and first half is
   * used for allocation.
   */
  if (blk->size == size) {
    blk->prev->next = blk->next;
    blk->next->prev = blk->prev;
  } else {
    tmp = (struct page *)((vaddr_t)blk + size);
    tmp->size = blk->size - size;
    tmp->prev = blk->prev;
    tmp->next = blk->next;
    blk->prev->next = tmp;
    blk->next->prev = tmp;
  }

  used_size += (psize_t)size;
  UNLOCK();
  return kvtop(blk);
}

/** Free page block.
 *
 * This allocator does not maintain the size of allocated page
 * block. The caller must provide the size information of the
 * block.
 */
void page_free(paddr_t paddr, psize_t psize)
{
  struct page *blk, *prev;
  vsize_t size;

  ASSERT(psize != 0);

  LOCK();

  size = round_page(psize);
  blk = ptokv(paddr);

  // Find the target position in list.
  for (prev = &page_head; prev->next < blk; prev = prev->next) {
    if (prev->next == &page_head)
      break;
  }

  // Insert new block into list.
  blk->size = size;
  blk->prev = prev;
  blk->next = prev->next;
  prev->next->prev = blk;
  prev->next = blk;

  // If the adjoining block is free, combine the blocks.
  if (blk->next != &page_head &&
      ((vaddr_t)blk + blk->size) == (vaddr_t)blk->next) {
    blk->size += blk->next->size;
    blk->next = blk->next->next;
    blk->next->prev = blk;
  }

  if (blk->prev != &page_head &&
      (vaddr_t)blk->prev + blk->prev->size == (vaddr_t)blk) {
    blk->prev->size += blk->size;
    blk->prev->next = blk->next;
    blk->next->prev = blk->prev;
  }

  used_size -= (psize_t)size;
  UNLOCK();
}

/** The function to reserve pages in specific address.
 */
int page_reserve(paddr_t paddr, psize_t psize)
{
  struct page *blk, *tmp;
  vaddr_t start, end;
  vsize_t size;

  if (psize == 0)
    return 0;

  start = trunc_page((vaddr_t)ptokv(paddr));
  end = round_page((vaddr_t)ptokv(paddr + psize));
  size = end - start;

  // Find the block which includes specified block.
  blk = page_head.next;
  for (;;) {
    if (blk == &page_head)
      return -ENOMEM;

    if ((vaddr_t)blk <= start
        && end <= (vaddr_t)blk + blk->size)
      break;

    blk = blk->next;
  }

  if ((vaddr_t)blk == start && blk->size == size) {
    // Unlink the block from free list.
    blk->prev->next = blk->next;
    blk->next->prev = blk->prev;
  } else {
    // Split this block.
    if ((vaddr_t)blk + blk->size != end) {
      tmp = (struct page *)end;
      tmp->size = (vaddr_t)blk + blk->size - end;
      tmp->next = blk->next;
      tmp->prev = blk;

      blk->size -= tmp->size;
      blk->next->prev = tmp;
      blk->next = tmp;
    }

    if ((vaddr_t)blk == start) {
      blk->prev->next = blk->next;
      blk->next->prev = blk->prev;
    } else {
      blk->size = start - (vaddr_t)blk;
    }
  }
  used_size += (psize_t)size;
  return 0;
}

void page_info(struct meminfo *info)
{
  info->total = total_size;
  info->free = total_size - used_size;
}

/** Initialize page allocator.
 * page_init() must be called prior to other memory manager's
 * initializations.
 */
void page_init(void)
{
  struct physmem *ram;
  struct bootinfo *bi = &bootinfo;
  int i;

  total_size = 0;
  page_head.next = page_head.prev = &page_head;

  // First, create a free list from the boot information.
  for (i = 0; i < bi->nr_rams; i++) {
    ram = &bi->ram[i];
    if (ram->type == MT_USABLE) {
      page_free(ram->base, ram->size);
      total_size += ram->size;
    }
  }

  // Then, reserve un-usable memory.
  for (i = 0; i < bi->nr_rams; i++) {
    ram = &bi->ram[i];
    switch (ram->type) {
    case MT_MEMHOLE:
      total_size -= ram->size;
      /* FALLTHROUGH */
    case MT_RESERVED:
      if (page_reserve(ram->base, ram->size))
        panic("page_init");
      break;
    }
  }

  used_size = 0;
  DPRINTF(MEMDB_CORE, ("Memory size=%ld\n", total_size));
}

#if CONFIG_PM_COMMANDS
/** Display the free page list.
 */
static int pmCommand(int argc, char **argv)
{
  if (argc <= 0) {
    printf("show free memory page information\n");
    return COMMAND_OK;
  }

  printf("Total size: %9lu (%lu pages)\n", total_size, total_size / PAGE_SIZE);
  printf("Used size:  %9lu (%lu pages)\n", used_size, used_size / PAGE_SIZE);
  printf("Free blocks:\n");
  printf("%5.5s %10.10s %10.10s\n", "BLOCK", "VADDR", "SIZE");

  int i = 0;
  struct page *page = page_head.next;
  do {
    printf("%5d %8p %10zd bytes (%zd pages)\n", ++i, page, page->size,
           page->size / PAGE_SIZE);
    page = page->next;
  } while(page && page != &page_head);

  return COMMAND_OK;
}

/** Create a section heading for the help command.
 */
static int sectionCommand(int argc, char **argv)
{
  if (argc <= 0 ) {
    printf("Paged Memory Allocation Commands:\n");
  }
  return COMMAND_OK;
}

C_CONSTRUCTOR()
{
  command_insert(NULL, sectionCommand);
  command_insert("pm", pmCommand);
}

#endif  // CONFIG_PM_COMMANDS

