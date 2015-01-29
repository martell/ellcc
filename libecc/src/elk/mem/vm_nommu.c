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
 * vm_nommu.c - virtual memory alloctor for no MMU systems
 */

/*
 * When the platform does not support memory management unit (MMU)
 * all virtual memories are mapped to the physical memory. So, the
 * memory space is shared among all threads and kernel.
 *
 * Important: The lists of segments are not sorted by address.
 */

#include <sys/mman.h>
#include <errno.h>
#include <string.h>

#include "config.h"
#include "kernel.h"
#include "thread.h"
#include "page.h"
#include "kmem.h"
#include "vm.h"

// Virtual memory without MMU is a loadable feature.
FEATURE_CLASS(vm_nommu, vm)

// Forward declarations.
static void seg_init(struct seg *);
static struct seg *seg_create(struct seg *, vaddr_t, size_t);
static void seg_delete(struct seg *, struct seg *);
static struct seg *seg_merge(struct seg *, vaddr_t, size_t);
static struct seg *seg_lookup(struct seg *, vaddr_t, size_t);
static struct seg *seg_alloc(struct seg *, size_t);
static void seg_free(struct seg *, struct seg *);
static struct seg *seg_split(struct seg *, struct seg *, vaddr_t, size_t);
static struct seg *seg_reserve(struct seg *, vaddr_t, size_t);
static int do_allocate(vm_map_t, void **, size_t, int);
static int do_free(vm_map_t, void *, size_t);
static int do_attribute(vm_map_t, void *, size_t, int);
static int do_map(vm_map_t, void *, size_t, void **);

static struct vm_map kernel_map;    // VM mapping for kernel.

#include "vm_common.c"

/** Allocate zero-filled memory for specified address.
 *
 * If "anywhere" argument is true, the "addr" argument will be
 * ignored.  In this case, the address of free space will be
 * found automatically.
 *
 * The allocated area has writable, user-access attribute by
 * default.  The "addr" and "size" argument will be adjusted
 * to page boundary.
 */
static int int_allocate(pid_t pid, void **addr, size_t size, int anywhere)
{
  int error;

  if (!pid_valid(pid)) {
    return -ESRCH;
  }

  if (pid != getpid() && !capable(CAP_SYS_PTRACE)) {
    return -EPERM;
  }

  void *uaddr = *addr;
  if (pid && anywhere == 0 && !user_area(*addr)) {
    return -EACCES;
  }

  vm_map_t map = getmap(pid);
  pthread_mutex_lock(&map->lock);
  error = do_allocate(map, &uaddr, size, anywhere);
  if (!error) {
    *addr = uaddr;
  }

  pthread_mutex_unlock(&map->lock);
  return error;
}

static int do_allocate(vm_map_t map, void **addr, size_t size, int anywhere)
{
  if (size == 0)
    return -EINVAL;

#if RICH // MAXMEM defined in param.h as 4MB. Why?
  if (map->total + size >= MAXMEM)
    return -ENOMEM;
#endif

  // Allocate segment, and reserve pages for it.
  struct seg *seg;
  vaddr_t start;
  if (anywhere) {
    size = round_page(size);
    if ((seg = seg_alloc(&map->head, size)) == NULL)
      return -ENOMEM;

    start = seg->addr;
  } else {
    start = trunc_page((vaddr_t)*addr);
    vaddr_t end = round_page(start + size);
    size = (size_t)(end - start);

    if ((seg = seg_reserve(&map->head, start, size)) == NULL)
      return -ENOMEM;
  }

  seg->flags = SEG_READ|SEG_WRITE;

  // Zero fillA.
  memset((void *)start, 0, size);
  *addr = (void *)seg->addr;
  map->total += size;
  return 0;
}

/** Deallocate a memory segment at a specified address.
 *
 * The "addr" argument points to a memory segment previously
 * allocated through a call to vm_allocate() or vm_map(). The
 * number of bytes freed is the number of bytes of the
 * allocated segment.  If the previous and next segments are free,
 * the current segment is combined with them, and a larger free
 * segment is created.
 */
static int int_free(pid_t pid, void *addr, size_t size)
{
  int error;

  if (!pid_valid(pid)) {
    return -ESRCH;
  }

  if (pid != getpid() && !capable(CAP_SYS_PTRACE)) {
    return -EPERM;
  }

  if (pid && !user_area(addr)) {
    return -EFAULT;
  }

  vm_map_t map = getmap(pid);
  pthread_mutex_lock(&map->lock);
  error = do_free(map, addr, size);
  pthread_mutex_unlock(&map->lock);
  return error;
}

static int do_free(vm_map_t map, void *addr, size_t size)
{
  struct seg *seg;
  vaddr_t va;

  va = trunc_page((vaddr_t)addr);

  // Merge adjacent segments.
  seg = seg_merge(&map->head, va, size);
  if (seg == NULL)
    return -EINVAL;

  // Find the target segment.
  seg = seg_lookup(&map->head, va, size);
  if (seg == NULL || seg->addr != va || (seg->flags & SEG_FREE))
    return -EINVAL;

  // Relinquish use of the page if it is not shared and mapped.
  if (!(seg->flags & SEG_SHARED) && !(seg->flags & SEG_MAPPED))
    page_free(seg->phys, seg->size);

  map->total -= seg->size;
  seg_free(&map->head, seg);
  return 0;
}

/** Change attribute of specified virtual address.
 *
 * The "addr" argument points to a memory segment previously
 * allocated through a call to vm_allocate(). The attribute
 * type can be chosen a combination of PROT_READ, PROT_WRITE.
 * Note: PROT_EXEC is not supported, yet.
 */
#define PROT_ALL (PROT_READ|PROT_WRITE|PROT_EXEC|PROT_GROWSDOWN|PROT_GROWSUP)
static int int_attribute(pid_t pid, void *addr, size_t size, int attr)
{
  int error;

  if (attr & ~PROT_ALL) {
    return -EINVAL;
  }

  if (!pid_valid(pid)) {
    return -ESRCH;
  }

  if (pid != getpid() && !capable(CAP_SYS_PTRACE)) {
    return -EPERM;
  }

  if (pid && !user_area(addr)) {
    return -ENOMEM;
  }

  vm_map_t map = getmap(pid);
  pthread_mutex_lock(&map->lock);
  error = do_attribute(map, addr, size, attr);
  pthread_mutex_unlock(&map->lock);
  return error;
}

static int do_attribute(vm_map_t map, void *addr, size_t size, int attr)
{
  struct seg *seg;
  vaddr_t va;

  va = trunc_page((vaddr_t)addr);
  if (va != (vaddr_t)addr) {
    // The address must be a multiple of the system page size.
    return -EINVAL;
  }

  // Find the target segment.
  seg = seg_lookup(&map->head, va, size);
  if (seg == NULL || (seg->flags & SEG_FREE)) {
    return -EINVAL;     // Not allocated.
  }

  // The attribute of the mapped or shared segment can not be changed.
  if ((seg->flags & SEG_MAPPED) || (seg->flags & SEG_SHARED))
    return -EINVAL;

  if (seg->addr != va) {
    // Not at the beginning, try to split up the segment.
    seg = seg_split(&map->head, seg, va, size);
    if (seg == NULL) {
      // The requested area isn't within the original segment.
      return -EINVAL;
    }
  }

  // Check new and old flags.
  int new_flags = 0;
  if (attr & PROT_READ)
    new_flags |= SEG_READ;
  if (attr & PROT_WRITE)
    new_flags |= SEG_WRITE;
  if (attr & PROT_EXEC)
    new_flags |= SEG_EXEC;

  if (new_flags == (seg->flags & SEG_ACCESS))
    return 0;                           // Same attributes.

  // RICH: We lose the SEG_EXEC flag here.
  seg->flags &= ~SEG_ACCESS;
  seg->flags |= new_flags;
  return 0;
}

/** Map another process's memory to current process.
 *
 * Note: This routine does not support mapping to the specific address.
 */
static int int_map(pid_t target, void *addr, size_t size, void **alloc)
{
  int error;

  if (!pid_valid(target)) {
    return -ESRCH;
  }

  if (target == getpid()) {
    return -EINVAL;
  }

  if (!capable(CAP_SYS_PTRACE)) {
    return -EPERM;
  }

  if (getpid() && !user_area(addr)) {
    return -EFAULT;
  }

  vm_map_t map = getmap(target);
  pthread_mutex_lock(&map->lock);
  error = do_map(map, addr, size, alloc);
  pthread_mutex_unlock(&map->lock);
  return error;
}

static int do_map(vm_map_t map, void *addr, size_t size, void **alloc)
{
  if (size == 0)
    return -EINVAL;

#if RICH // MAXMEM defined in param.h as 4MB. Why?
  if (map->total + size >= MAXMEM)
    return -ENOMEM;
#endif

  vaddr_t start = trunc_page((vaddr_t)addr);
  vaddr_t end = round_page((vaddr_t)addr + size);
  size = (size_t)(end - start);

  // Find the segment that includes target address.
  struct seg *seg = seg_lookup(&map->head, start, size);
  if (seg == NULL || (seg->flags & SEG_FREE))
    return -EINVAL;             // Not allocated.

  struct seg *tgt = seg;

  // Create new segment to map.
  vm_map_t curmap = getcurmap();
  if ((seg = seg_create(&curmap->head, start, size)) == NULL)
    return -ENOMEM;

  seg->flags = tgt->flags | SEG_MAPPED;
  *alloc = addr;
  curmap->total += size;
  return 0;
}

/** Create new virtual memory space.
 * No memory is inherited.
 */
static vm_map_t int_create(void)
{
  struct vm_map *map;

  // Allocate new map structure.
  if ((map = kmem_alloc(sizeof(*map))) == NULL)
    return NULL;

  map->refcnt = 1;
  map->total = 0;
  pthread_mutex_init(&map->lock, NULL);

  seg_init(&map->head);
  return map;
}

/** Terminate specified virtual memory space.
 * This is called when a process is terminated.
 */
static void int_terminate(vm_map_t map)
{
  struct seg *seg, *tmp;

  if (--map->refcnt > 0)
    return;

  pthread_mutex_lock(&map->lock);
  seg = &map->head;
  do {
    if (seg->flags != SEG_FREE) {
      // Free segment if it is not shared and mapped.
      if (!(seg->flags & SEG_SHARED) &&
          !(seg->flags & SEG_MAPPED)) {
        page_free(seg->phys, seg->size);
      }
    }

    tmp = seg;
    seg = seg->next;
    seg_delete(&map->head, tmp);
  } while (seg != &map->head);

  kmem_free(map);
  pthread_mutex_unlock(&map->lock);
}

/** Duplicate specified virtual memory space.
 */
static vm_map_t int_dup(vm_map_t org_map)
{
  // This function is not supported with no MMU system.
  return NULL;
}

/** Switch VM mapping.
 */
static void int_switch(vm_map_t map)
{
  // This is a NOOP on a non-MMU system.
}

/** Increment reference count of VM mapping.
 */
static int int_reference(vm_map_t map)
{
  map->refcnt++;
  return 0;
}

/** Translate virtual address of current process to physical address.
 * Returns physical address on success, or NULL if no mapped memory.
 */
static paddr_t int_translate(vaddr_t addr, size_t size)
{
  return (paddr_t)addr;
}

/** Map a physical page into a virtual page.
 * This is used for early access to I/O registers.
 */
static void int_premap(paddr_t paddr, vaddr_t vaddr)
{
  ASSERT(paddr == vaddr);
}

static int int_info(struct vminfo *info)
{
  u_long target = info->cookie;
  pid_t pid = info->pid;

  if (!pid_valid(pid)) {
    return -ESRCH;
  }

  vm_map_t map = getmap(pid);
  pthread_mutex_lock(&map->lock);
  struct seg *seg = &map->head;
  u_long i = 0;
  do {
    if (i++ == target) {
      info->cookie = i;
      info->virt = seg->addr;
      info->size = seg->size;
      info->flags = seg->flags;
      info->phys = seg->phys;
      pthread_mutex_unlock(&map->lock);
      return 0;
    }
    seg = seg->next;
  } while (seg != &map->head);

  pthread_mutex_unlock(&map->lock);
  return -ESRCH;
}

static void int_mmu_init(void)
{
}

static vm_map_t int_init(size_t kmem)
{
  seg_init(&kernel_map.head);
  return &kernel_map;
}

/** Initialize segment.
 */
static void seg_init(struct seg *seg)
{
  seg->next = seg->prev = seg;
  seg->sh_next = seg->sh_prev = seg;
  seg->addr = 0;
  seg->phys = 0;
  seg->size = 0;
  seg->flags = SEG_FREE;
}

/** Create new free segment after the specified segment.
 * Returns segment on success, or NULL on failure.
 */
static struct seg *seg_create(struct seg *prev, vaddr_t addr, size_t size)
{
  struct seg *seg;

  if ((seg = kmem_alloc(sizeof(*seg))) == NULL)
    return NULL;

  seg->addr = addr;
  seg->size = size;
  seg->phys = (paddr_t)addr;
  seg->flags = SEG_FREE;
  seg->sh_next = seg->sh_prev = seg;

  seg->next = prev->next;
  seg->prev = prev;
  prev->next->prev = seg;
  prev->next = seg;
  return seg;
}

/** Delete specified segment.
 */
static void seg_delete(struct seg *head, struct seg *seg)
{
  // If it is shared segment, unlink from shared list.
  if (seg->flags & SEG_SHARED) {
    seg->sh_prev->sh_next = seg->sh_next;
    seg->sh_next->sh_prev = seg->sh_prev;
    if (seg->sh_prev == seg->sh_next)
      seg->sh_prev->flags &= ~SEG_SHARED;
  }

  if (head != seg)
    kmem_free(seg);
}

/** Merge segments at the specified address and size.
 */
static struct seg *seg_merge(struct seg *head, vaddr_t addr, size_t size)
{
  struct seg *seg;

  seg = head;
  do {
    if (!(seg->flags & SEG_FREE) &&
        seg->addr == addr &&
        seg->addr + seg->size >= addr) {
      // The segment starts in this segment.
      struct seg *next = seg->next;
      size_t s = seg->size;
      while (s < size) {
        if (next == head) {
          // No match.
          return NULL;
        }

        // Add the size, and keep looking.
        s += next->size;
        next = next->next;
      }

      // Found segmends to merge.
      for (struct seg *sp = seg->next; sp != next; ) {
        seg->size += sp->size;
        struct seg *next = sp->next;    // Save the next pointer.
        if (seg->size <= size) {
          // This segment is in the middle.
          kmem_free(sp);
          seg->next = next;
          next = sp->next;
        }
        sp = next;
      }

      return seg;
    }
    seg = seg->next;
  } while (seg != head);

  return NULL;
}

/** Find the segment at the specified address.
 */
static struct seg *seg_lookup(struct seg *head, vaddr_t addr, size_t size)
{
  struct seg *seg;

  seg = head;
  do {
    if (seg->addr <= addr &&
        seg->addr + seg->size >= addr + size) {
      return seg;
    }

    seg = seg->next;
  } while (seg != head);

  return NULL;
}

/** Allocate free segment for specified size.
 */
static struct seg *seg_alloc(struct seg *head, size_t size)
{
  struct seg *seg;
  paddr_t pa;

  if ((pa = page_alloc(size)) == 0)
    return NULL;

  DPRINTF(MEMDB_VM, ("vm_nommu: physical page allocated 0x%08lx (%zu)\n",
                     pa, size));
  if ((seg = seg_create(head, (vaddr_t)pa, size)) == NULL) {
         page_free(pa, size);
    return NULL;
  }

  return seg;
}

/** Delete specified free segment.
 */
static void seg_free(struct seg *head, struct seg *seg)
{
  ASSERT(seg->flags != SEG_FREE);

  // If it is shared segment, unlink from shared list.
  if (seg->flags & SEG_SHARED) {
    seg->sh_prev->sh_next = seg->sh_next;
    seg->sh_next->sh_prev = seg->sh_prev;

    if (seg->sh_prev == seg->sh_next)
      seg->sh_prev->flags &= ~SEG_SHARED;
  }

  seg->prev->next = seg->next;
  seg->next->prev = seg->prev;
  kmem_free(seg);
}

/** Split the segment at the specified address/size.
 */
static struct seg *seg_split(struct seg *head, struct seg *seg,
                             vaddr_t addr, size_t size)
{
  struct seg *prev, *next;
  size_t diff;

  // Check previous segment to split segment.
  prev = NULL;
  if (seg->addr != addr) {
    prev = seg;
    diff = (size_t)(addr - seg->addr);
    seg = seg_create(prev, addr, prev->size - diff);
    if (seg == NULL)
      return NULL;

    seg->flags = prev->flags;
    prev->size = diff;
  }

  // Check next segment to split segment.
  if (seg->size != size) {
    next = seg_create(seg, seg->addr + size, seg->size - size);
    if (next == NULL) {
      if (prev) {
        // Undo previous seg_create() operation.
        prev->size += diff;
        seg_free(head, seg);
      }

      return NULL;
    }

    next->flags = seg->flags;
    seg->size = size;
  }

  return seg;
}

/** Reserve the segment at the specified address/size.
 */
static struct seg *seg_reserve(struct seg *head, vaddr_t addr, size_t size)
{
  struct seg *seg;
  paddr_t pa;

  pa = (paddr_t)addr;

  if (page_reserve(pa, size) != 0)
    return NULL;

  if ((seg = seg_create(head, (vaddr_t)pa, size)) == NULL) {
         page_free(pa, size);
    return NULL;
  }

  return seg;
}

ELK_PRECONSTRUCTOR()
{
  vm_allocate = int_allocate;
  vm_free = int_free;
  vm_attribute = int_attribute;
  vm_map = int_map;
  vm_reference = int_reference;
  vm_create = int_create;
  vm_terminate = int_terminate;
  vm_dup = int_dup;
  vm_switch = int_switch;
  vm_translate = int_translate;
  vm_premap = int_premap;
  vm_info = int_info;
  vm_init = int_init;
  vm_mmu_init = int_mmu_init;
}
