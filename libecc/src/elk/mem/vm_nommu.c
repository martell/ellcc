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
static struct seg *seg_lookup(struct seg *, vaddr_t, size_t);
static struct seg *seg_alloc(struct seg *, size_t);
static void seg_free(struct seg *, struct seg *);
static struct seg *seg_reserve(struct seg *, vaddr_t, size_t);
static int do_allocate(vm_map_t, void **, size_t, int);
static int do_free(vm_map_t, void *);
static int do_attribute(vm_map_t, void *, int);
static int do_map(vm_map_t, void *, size_t, void **);

static struct vm_map kernel_map;    // VM mapping for kernel.

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
used static int int_allocate(pid_t pid, void **addr, size_t size, int anywhere)
{
  int error;

  if (!pid_valid(pid)) {
    return -ESRCH;
  }

  if (pid != getpid() && !capable(CAP_SYS_PTRACE)) {
    return -EPERM;
  }

  void *uaddr = *addr;
  if (anywhere == 0 && !user_area(*addr)) {
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
 * allocated segment.  If the previous and* next segments are free,
 * the current segment is combined with them, and a larger free
 * segment is created.
 */
used static int int_free(pid_t pid, void *addr)
{
  int error;

  if (!pid_valid(pid)) {
    return -ESRCH;
  }

  if (pid != getpid() && !capable(CAP_SYS_PTRACE)) {
    return -EPERM;
  }

  if (!user_area(addr)) {
    return -EFAULT;
  }

  vm_map_t map = getmap(pid);
  pthread_mutex_lock(&map->lock);
  error = do_free(map, addr);
  pthread_mutex_unlock(&map->lock);
  return error;
}

static int do_free(vm_map_t map, void *addr)
{
  struct seg *seg;
  vaddr_t va;

  va = trunc_page((vaddr_t)addr);

  // Find the target segment.
  seg = seg_lookup(&map->head, va, 1);
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
used static int int_attribute(pid_t pid, void *addr, int attr)
{
  int error;

  if (attr == 0 || attr & ~(PROT_READ | PROT_WRITE)) {
    return -EINVAL;
  }

  if (!pid_valid(pid)) {
    return -ESRCH;
  }

  if (pid != getpid() && !capable(CAP_SYS_PTRACE)) {
    return -EPERM;
  }

  if (!user_area(addr)) {
    return -EFAULT;
  }

  vm_map_t map = getmap(pid);
  pthread_mutex_lock(&map->lock);
  error = do_attribute(map, addr, attr);
  pthread_mutex_unlock(&map->lock);
  return error;
}

static int do_attribute(vm_map_t map, void *addr, int attr)
{
  struct seg *seg;
  vaddr_t va;

  va = trunc_page((vaddr_t)addr);

  // Find the target segment.
  seg = seg_lookup(&map->head, va, 1);
  if (seg == NULL || seg->addr != va || (seg->flags & SEG_FREE)) {
    return -EINVAL;  /* not allocated */
  }

  // The attribute of the mapped or shared segment can not be changed.
  if ((seg->flags & SEG_MAPPED) || (seg->flags & SEG_SHARED))
    return -EINVAL;

  // Check new and old flag.
  int new_flags = 0;
  if (seg->flags & SEG_WRITE) {
    if (!(attr & PROT_WRITE))
      new_flags = SEG_READ;
  } else {
    if (attr & PROT_WRITE)
      new_flags = SEG_READ|SEG_WRITE;
  }

  if (new_flags == 0)
    return 0;                           // Same attribute.

  seg->flags = new_flags;
  return 0;
}

/** Map another process's memory to current process.
 *
 * Note: This routine does not support mapping to the specific address.
 */
used static int int_map(pid_t target, void *addr, size_t size, void **alloc)
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

  if (!user_area(addr)) {
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
used static vm_map_t int_create(void)
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
used static void int_terminate(vm_map_t map)
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
used static vm_map_t int_dup(vm_map_t org_map)
{
  // This function is not supported with no MMU system.
  return NULL;
}

/** Switch VM mapping.
 */
used static void int_switch(vm_map_t map)
{
  // This is a NOOP on a non-MMU system.
}

/** Increment reference count of VM mapping.
 */
used static int int_reference(vm_map_t map)
{
  map->refcnt++;
  return 0;
}

/** Translate virtual address of current process to physical address.
 * Returns physical address on success, or NULL if no mapped memory.
 */
used static paddr_t int_translate(vaddr_t addr, size_t size)
{
  return (paddr_t)addr;
}

used static int int_info(struct vminfo *info)
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

used static vm_map_t int_init(void)
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

weak_alias(int_allocate, vm_allocate);
weak_alias(int_free, vm_free);
weak_alias(int_attribute, vm_fattribute);
weak_alias(int_map, vm_map);
weak_alias(int_reference, vm_reference);
weak_alias(int_create, vm_create);
weak_alias(int_terminate, vm_terminate);
weak_alias(int_dup, vm_dup);
weak_alias(int_switch, vm_switch);
weak_alias(int_translate, vm_translate);
weak_alias(int_info, vm_info);
weak_alias(int_init, vm_init);
