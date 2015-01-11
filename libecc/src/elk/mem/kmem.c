/*
 * Copyright (c) 2014 Richard Pennington.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
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
 * kmem.c - kernel memory allocator
 */

/*
 * This is a memory allocator optimized for the low foot print
 * kernel. It works on top of the underlying page allocator, and
 * manages more smaller memory than page size. It will divide one page
 * into two or more blocks, and each page is linked as a kernel page.
 *
 * There are following 3 linked lists to manage used/free blocks.
 *  1) All pages allocated for the kernel memory are linked.
 *  2) All blocks divided in the same page are linked.
 *  3) All free blocks of the same size are linked.
 *
 * Currently, it can not handle the memory size exceeding one page.
 * Instead, a driver can use page_alloc() to allocate larger memory.
 *
 * The kmem functions are used by not only the kernel core but also by
 * the buggy drivers. If such kernel code illegally writes data in
 * exceeding the allocated area, the system will crash easily. In
 * order to detect the memory over run, each free block has a magic
 * ID.
 */

#include <sys/types.h>
#include <limits.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>

#include "kernel.h"
#include "bootinfo.h"
#include "types.h"
#include "list.h"
#include "page.h"
#include "vm.h"
#include "command.h"
#include "kmem.h"

/** Free Page list.
 * In an MMU enabled system, kmem pages are preallocated and kept in the free
 * page list until needed so they don't interfere with kernel virtual memory
 * allocations.
 */
struct free_page
{
  union
  {
    struct free_page *next;
    char page[PAGE_SIZE];
  };
};

static size_t reserved_size;
static size_t used_size;
static struct free_page *free_pages;

/** Block header
 *
 * All free blocks that have same size are linked each other.
 * In addition, all free blocks within same page are also linked.
 */
struct block_hdr
{
  u_short magic;                        // Magic number.
  u_short size;                         // Size of this block.
  struct list link;                     // Link to the free list.
  struct block_hdr *pg_next;            // Next block in same page.
};

/** Page header
 *
 * The page header is placed at the top of each page. This
 * header is used in order to free the page when there are no
 * used block left in the page. If 'nallocs' value becomes zero,
 * that page can be removed from kernel page.
 */
struct page_hdr
{
  u_short magic;                        // Magic number.
  u_short nallocs;                      // Number of allocated blocks.
  u_int pad[3];                         // Align to 16 bytes.
  struct block_hdr first_blk;           // First block in this page.
};

#define ALIGN_SIZE      16
#define ALIGN_MASK      (ALIGN_SIZE - 1)
#define ALLOC_SIZE(n)  (size_t) \
  (((vaddr_t)(n) + ALIGN_MASK) & (vaddr_t)~ALIGN_MASK)

// Number of free block list.
#define NR_BLOCK_LIST   (PAGE_SIZE / ALIGN_SIZE)

#define BLOCK_MAGIC     0xdead
#define PAGE_MAGIC      0xbeef

#define BLKHDR_SIZE     (sizeof(struct block_hdr))
#define PGHDR_SIZE      (sizeof(struct page_hdr))
#define MAX_ALLOC_SIZE  (size_t)(PAGE_SIZE - PGHDR_SIZE)

#define MIN_BLOCK_SIZE  (BLKHDR_SIZE + 16)
#define MAX_BLOCK_SIZE  (u_short)(PAGE_SIZE - (PGHDR_SIZE - BLKHDR_SIZE))

// Macro to point to the page header for a specific address.
#define PAGETOP(n)      (struct page_hdr *) \
  ((vaddr_t)(n) & (vaddr_t)~(PAGE_SIZE - 1))

// Macro to get the index of the free block list.
#define BLKNDX(b)       ((u_int)((b)->size) >> 4)

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
#define LOCK()  pthread_mutex_lock(&mutex)
#define UNLOCK()  pthread_mutex_unlock(&mutex)

/** Array of the head block of free block list.
 *
 * The index of array is decided by the size of each block.
 * All block has the size of the multiple of 16.
 *
 * ie. free_blocks[0] = list for 16 byte block
 *     free_blocks[1] = list for 32 byte block
 *     free_blocks[2] = list for 48 byte block
 *         .
 *         .
 *     free_blocks[255] = list for 4096 byte block
 *
 * Generally, only one list is used to search the free block with
 * a first fit algorithm. Basically, this allocator also uses a
 * first fit method. However it uses multiple lists corresponding
 * to each block size. A search is started from the list of the
 * requested size. So, it is not necessary to search smaller
 * block's list wastefully.
 *
 * Most of kernel memory allocator is using 2^n as block size.
 * But, these implementation will throw away much memory that
 * the block size is not fit. This is not suitable for the
 * embedded system with low foot print.
 */
static struct list free_blocks[NR_BLOCK_LIST];

/** Find the free block for the specified size.
 * Returns pointer to free block, or NULL on failure.
 *
 * First, it searches from the list of same size. If it does not
 * exists, then it will search the list of larger one.
 * It will use the block of smallest size that satisfies the
 * specified size.
 */
static struct block_hdr *block_find(size_t size)
{
  int i;
  list_t n;

  for (i = (int)((u_int)size >> 4); i < NR_BLOCK_LIST; i++) {
    if (!list_empty(&free_blocks[i]))
      break;
  }

  if (i >= NR_BLOCK_LIST)
    return NULL;

  n = list_first(&free_blocks[i]);
  return list_entry(n, struct block_hdr, link);
}

/** Allocate a memory block for the kernel.
 *
 * This function does not fill the allocated block with 0 for performance.
 * kmem_alloc() returns NULL on failure.
 *
 * => must not be called from interrupt context.
 */
void *kmem_alloc(size_t size)
{
  struct block_hdr *blk, *newblk;
  struct page_hdr *pg;
  paddr_t pa;
  void *p;

  ASSERT(size != 0);

  LOCK();

  /* First, the free block of enough size is searched
   * from the page already used. If it does not exist,
   * new page is allocated for free block.
   */
  size = ALLOC_SIZE(size + BLKHDR_SIZE);
  if (size > MAX_ALLOC_SIZE)
    panic("kmem_alloc: too large allocation");

  blk = block_find(size);
  if (blk) {
    // Block found.
    list_remove(&blk->link);            // Remove from free list.
    pg = PAGETOP(blk);                  // Get the page address.
  } else {
    // No block found. Allocate a new page.
    if (mmu_enabled()) {
      // Grab a preallocated page.
      struct free_page *fp = free_pages;
      if (fp == NULL) {
        UNLOCK();
        return NULL;
      }
      free_pages = fp->next;
      pg = (struct page_hdr *)fp;
      reserved_size -= PAGE_SIZE;
      used_size += PAGE_SIZE;
    } else {
      if ((pa = page_alloc(PAGE_SIZE)) == 0) {
        UNLOCK();
        return NULL;
      }

      used_size += PAGE_SIZE;
      DPRINTF(MEMDB_KMEM, ("kmem_alloc: physical page allocated 0x%08lx (%zu)\n",
                           pa, PAGE_SIZE));
      pg = ptokv(pa);
    }

    pg->nallocs = 0;
    pg->magic = PAGE_MAGIC;

    // Setup the first block.
    blk = &pg->first_blk;
    blk->magic = BLOCK_MAGIC;
    blk->size = MAX_BLOCK_SIZE;
    blk->pg_next = NULL;
  }

  // Sanity check.
  if (pg->magic != PAGE_MAGIC || blk->magic != BLOCK_MAGIC)
    panic("kmem_alloc: overrun");

  // If the found block is large enough, split it.
  if (blk->size - size >= MIN_BLOCK_SIZE) {
    // Make a new block.
    newblk = (struct block_hdr *)((vaddr_t)blk + size);
    newblk->magic = BLOCK_MAGIC;
    newblk->size = (u_short)(blk->size - size);
    list_insert(&free_blocks[BLKNDX(newblk)], &newblk->link);

    // Update the page list.
    newblk->pg_next = blk->pg_next;
    blk->pg_next = newblk;
    blk->size = (u_short)size;
  }

  // Increment the allocation count of this page.
  ++pg->nallocs;
  p = (void *)((vaddr_t)blk + BLKHDR_SIZE);

  UNLOCK();
  DPRINTF(MEMDB_KMEM, ("kmem_alloc: address 0x%08lx, size = %zu, "
                       "blk = %p magic = 0x%04x\n",
                       p, size, blk, blk->magic));
  return p;
}

/** Reallocate a memory block for the kernel.
 *
 * => must not be called from interrupt context.
 */
void *kmem_realloc(void *ptr, size_t size)
{

  ASSERT(ptr != NULL);

  LOCK();

  // Get the block header.
  struct block_hdr *blk;
  blk = (struct block_hdr *)((vaddr_t)ptr - BLKHDR_SIZE);
  if (blk->magic != BLOCK_MAGIC)
    panic("kmem_realloc: invalid address");

  if (blk->size >= size + BLKHDR_SIZE) {
    UNLOCK();
    return ptr;
  }

  // RICH: Can we expand the current block? Is it worth it?
  UNLOCK();
  void *new_ptr = kmem_alloc(size);
  if (new_ptr == NULL) {
    return NULL;
  }

  // Copy the old contents.
  memcpy(new_ptr, ptr, blk->size - BLKHDR_SIZE);
  kmem_free(ptr);
  return new_ptr;
}

/** Free an allocated memory block.
 *
 * Sometimes the kernel does not release the free page for kernel memory
 * because it is allocated immediately later. For example,
 * it is efficient here if the free page is just linked to the list
 * of the biggest size. However, consider the case where a driver
 * requires many small memories temporarily. After these pages are
 * freed, they can not be reused for an application.
 */
void kmem_free(void *ptr)
{
  struct block_hdr *blk;
  struct page_hdr *pg;

  if (ptr == NULL)
    return;

  LOCK();

  DPRINTF(MEMDB_KMEM, ("kmem_free: address 0x%08lx\n", ptr));

  // Get the block header.
  blk = (struct block_hdr *)((vaddr_t)ptr - BLKHDR_SIZE);
  if (blk->magic != BLOCK_MAGIC) {
    DPRINTF(MSG, ("kmem_free: invalid address 0x%08lx\n", ptr));
    panic("kmem_free: invalid address");
  }

  /* Return the block to free list. Since kernel code will
   * request fixed size of memory block, we don't merge the
   * blocks to use it as cache.
   */
  list_insert(&free_blocks[BLKNDX(blk)], &blk->link);

  // Decrement the allocation count of this page.
  pg = PAGETOP(blk);
  if (--pg->nallocs == 0) {
    /* No allocated block in this page.
     * Remove all blocks and deallocate this page.
     */
    for (blk = &pg->first_blk; blk != NULL; blk = blk->pg_next) {
      list_remove(&blk->link);          // Remove from free list.
    }

    pg->magic = 0;
    if (mmu_enabled()) {
      // Put the newly freed page in the free page list.
      struct free_page *fp = (struct free_page *)pg;
      fp->next = free_pages;
      free_pages = fp;
      reserved_size += PAGE_SIZE;
      used_size -= PAGE_SIZE;
    } else {
      page_free(kvtop(pg), PAGE_SIZE);
      used_size -= PAGE_SIZE;
    }
  }
  UNLOCK();
}

/** Map the specified virtual address to the kernel address space.
 * Returns kernel address on success, or NULL if no mapped memory.
 */
void *kmem_map(void *addr, size_t size)
{
  paddr_t pa;

  pa = vm_translate((vaddr_t)addr, size);
  if (pa == 0)
    return NULL;

  return ptokv(pa);
}

void kmem_init(size_t size)
{
  int i;

  for (i = 0; i < NR_BLOCK_LIST; i++)
    list_init(&free_blocks[i]);

  // Preallocate pages in an MMU enabled system.
  if (size && mmu_enabled()) {
    size = round_page(size);
    reserved_size = size;
    paddr_t pa = page_alloc(size);
    if (pa == 0)
      panic("kmem_init: can't allocate kmem");

    free_pages = (struct free_page *)ptokv(pa);
    struct free_page *fp = free_pages;
    // Link all the pages together.
    while(size -= PAGE_SIZE) {
      fp->next = fp + 1;
      ++fp;
    }

    fp->next = NULL;
  }
}

#if CONFIG_KM_COMMANDS
/** Display kernel heap information.
 */
static int kmCommand(int argc, char **argv)
{
  if (argc <= 0) {
    printf("show kernel heap information\n");
    return COMMAND_OK;
  }

  printf("Total size:    %9zu (%zu pages)\n", reserved_size + used_size,
         (reserved_size + used_size) / PAGE_SIZE);
  printf("Reserved size: %9zu (%zu pages)\n", reserved_size,
         reserved_size / PAGE_SIZE);
  printf("Used size:     %9zu (%zu pages)\n", used_size,
         used_size / PAGE_SIZE);
  return COMMAND_OK;
}

/** Create a section heading for the help command.
 */
static int sectionCommand(int argc, char **argv)
{
  if (argc <= 0 ) {
    printf("Kernel Heap Allocation Commands:\n");
  }
  return COMMAND_OK;
}

C_CONSTRUCTOR()
{
  command_insert(NULL, sectionCommand);
  command_insert("km", kmCommand);
}

#endif  // CONFIG_KM_COMMANDS
