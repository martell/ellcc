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
 * vm.c - virtual memory allocator
 */

/*
 * A process owns its private virtual address space. All threads in
 * a process  share one same memory space.
 * When a new process is made, the address mapping of the parent process
 * is copied to child process. In this time, the read-only space
 * is shared with old map.
 *
 * Since this kernel does not do page out to the physical storage,
 * it is guaranteed that the allocated memory is always continuing
 * and existing. Thereby, a kernel and drivers can be constructed
 * very simply.
 */

#include <errno.h>
#include <string.h>

#include "config.h"
#include "kernel.h"
#include "hal.h"
#include "thread.h"
#include "page.h"
#include "kmem.h"
#include "vm.h"

// Virtual memory is a loadable feature.
FEATURE_CLASS(vm, vm)

/* forward declarations */
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
static vm_map_t do_dup(vm_map_t);

static struct vm_map kernel_map;  	// VM mapping for kernel.

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
  void *uaddr;

  sched_lock();

  if (!pid_valid(pid)) {
    sched_unlock();
    return -ESRCH;
  }

  if (pid != getpid() && !capable(CAP_SYS_PTRACE)) {
    sched_unlock();
    return -EPERM;
  }

  if (copyin(addr, &uaddr, sizeof(uaddr))) {
    sched_unlock();
    return -EFAULT;
  }

  if (anywhere == 0 && !user_area(*addr)) {
    sched_unlock();
    return -EACCES;
  }

  error = do_allocate(getmap(pid), &uaddr, size, anywhere);
  if (!error) {
    if (copyout(&uaddr, addr, sizeof(uaddr)))
      error = -EFAULT;
  }

  sched_unlock();
  return error;
}

static int do_allocate(vm_map_t map, void **addr, size_t size, int anywhere)
{
  struct seg *seg;
  vaddr_t start, end;
  paddr_t pa;

  if (size == 0)
    return -EINVAL;

#if RICH // MAXMEM defined in param.h as 4MB. Why?
  if (map->total + size >= MAXMEM)
    return -ENOMEM;
#endif

  // Allocate segment.
  if (anywhere) {
    size = round_page(size);
    if ((seg = seg_alloc(&map->head, size)) == NULL)
      return -ENOMEM;
  } else {
    start = trunc_page((vaddr_t)*addr);
    end = round_page(start + size);
    size = (size_t)(end - start);

    if ((seg = seg_reserve(&map->head, start, size)) == NULL)
      return -ENOMEM;
  }

  seg->flags = SEG_READ | SEG_WRITE;

  // Allocate physical pages, and map them into the virtual address.
  if ((pa = page_alloc(size)) == 0)
    goto err1;

  if (mmu_map(map->pgd, pa, seg->addr, size, PG_WRITE))
    goto err2;

  seg->phys = pa;

  // Zero fill.
  memset(ptokv(pa), 0, seg->size);
  *addr = (void *)seg->addr;
  map->total += size;
  return 0;

 err2:
  page_free(pa, size);
 err1:
  seg_free(&map->head, seg);
  return -ENOMEM;
}

/** Deallocate a memory segment at a specified address.
 *
 * The "addr" argument points to a memory segment previously
 * allocated through a call to vm_allocate() or vm_map(). The
 * number of bytes freed is the number of bytes of the
 * allocated segment. If one of the segment of previous and next
 * are free, it combines with them, and larger free segment is
 * created.
 */
used static int int_free(pid_t pid, void *addr)
{
  int error;

  sched_lock();
  if (!pid_valid(pid)) {
    sched_unlock();
    return -ESRCH;
  }

  if (pid != getpid() && !capable(CAP_SYS_PTRACE)) {
    sched_unlock();
    return -EPERM;
  }

  if (!user_area(addr)) {
    sched_unlock();
    return -EFAULT;
  }

  error = do_free(getmap(pid), addr);
  sched_unlock();
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

  // Unmap pages of the segment.
  mmu_map(map->pgd, seg->phys, seg->addr,  seg->size, PG_UNMAP);

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

  sched_lock();
  if (attr == 0 || attr & ~(PROT_READ | PROT_WRITE)) {
    sched_unlock();
    return -EINVAL;
  }

  if (!pid_valid(pid)) {
    sched_unlock();
    return -ESRCH;
  }

  if (pid != getpid() && !capable(CAP_SYS_PTRACE)) {
    sched_unlock();
    return -EPERM;
  }

  if (!user_area(addr)) {
    sched_unlock();
    return -EFAULT;
  }

  error = do_attribute(getmap(pid), addr, attr);
  sched_unlock();
  return error;
}

static int do_attribute(vm_map_t map, void *addr, int attr)
{
  struct seg *seg;
  int new_flags, map_type;
  paddr_t old_pa, new_pa;
  vaddr_t va;

  va = trunc_page((vaddr_t)addr);

  // Find the target segment.
  seg = seg_lookup(&map->head, va, 1);
  if (seg == NULL || seg->addr != va || (seg->flags & SEG_FREE)) {
    return -EINVAL;  			// Not allocated.
  }

  // The attribute of the mapped segment can not be changed.
  if (seg->flags & SEG_MAPPED)
    return -EINVAL;

  // Check new and old flag.
  new_flags = 0;
  if (seg->flags & SEG_WRITE) {
    if (!(attr & PROT_WRITE))
      new_flags = SEG_READ;
  } else {
    if (attr & PROT_WRITE)
      new_flags = SEG_READ | SEG_WRITE;
  }

  if (new_flags == 0)
    return 0;  /* same attribute */

  map_type = (new_flags & SEG_WRITE) ? PG_WRITE : PG_READ;

  // If it is shared segment, duplicate it.
  if (seg->flags & SEG_SHARED) {
    old_pa = seg->phys;

    // Allocate new physical page.
    if ((new_pa = page_alloc(seg->size)) == 0)
      return -ENOMEM;

    // Copy source page.
    memcpy(ptokv(new_pa), ptokv(old_pa), seg->size);

    // Map new segment.
    if (mmu_map(map->pgd, new_pa, seg->addr, seg->size, map_type)) {
      page_free(new_pa, seg->size);
      return -ENOMEM;
    }

    seg->phys = new_pa;

    // Unlink from shared list.
    seg->sh_prev->sh_next = seg->sh_next;
    seg->sh_next->sh_prev = seg->sh_prev;
    if (seg->sh_prev == seg->sh_next)
      seg->sh_prev->flags &= ~SEG_SHARED;

    seg->sh_next = seg->sh_prev = seg;
  } else {
    if (mmu_map(map->pgd, seg->phys, seg->addr, seg->size,
          map_type))
      return -ENOMEM;
  }

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

  sched_lock();
  if (!pid_valid(target)) {
    sched_unlock();
    return -ESRCH;
  }

  if (target == getpid()) {
    sched_unlock();
    return -EINVAL;
  }

  if (!capable(CAP_SYS_PTRACE)) {
    sched_unlock();
    return -EPERM;
  }

  if (!user_area(addr)) {
    sched_unlock();
    return -EFAULT;
  }

  error = do_map(getmap(target), addr, size, alloc);

  sched_unlock();
  return error;
}

static int do_map(vm_map_t map, void *addr, size_t size, void **alloc)
{
  struct seg *seg, *cur, *tgt;
  vm_map_t curmap;
  vaddr_t start, end;
  paddr_t pa;
  size_t offset;
  int map_type;
  void *tmp;

  if (size == 0)
    return -EINVAL;

#if RICH // MAXMEM defined in param.h as 4MB. Why?
  if (map->total + size >= MAXMEM)
    return -ENOMEM;
#endif

  /* check fault */
  tmp = NULL;
  if (copyout(&tmp, alloc, sizeof(tmp)))
    return -EFAULT;

  start = trunc_page((vaddr_t)addr);
  end = round_page((vaddr_t)addr + size);
  size = (size_t)(end - start);
  offset = (size_t)((vaddr_t)addr - start);

  /*
   * Find the segment that includes target address
   */
  seg = seg_lookup(&map->head, start, size);
  if (seg == NULL || (seg->flags & SEG_FREE))
    return -EINVAL;  /* not allocated */

  tgt = seg;

  // Find the free segment in current process.
  curmap = getcurmap();
  if ((seg = seg_alloc(&curmap->head, size)) == NULL)
    return -ENOMEM;
  cur = seg;

  // Try to map into current memory.
  if (tgt->flags & SEG_WRITE)
    map_type = PG_WRITE;
  else
    map_type = PG_READ;

  pa = tgt->phys + (paddr_t)(start - tgt->addr);
  if (mmu_map(curmap->pgd, pa, cur->addr, size, map_type)) {
    seg_free(&curmap->head, seg);
    return -ENOMEM;
  }

  cur->flags = tgt->flags | SEG_MAPPED;
  cur->phys = pa;

  tmp = (void *)(cur->addr + offset);
  copyout(&tmp, alloc, sizeof(tmp));

  curmap->total += size;
  return 0;
}

/** Create new virtual memory space.
 * No memory is inherited.
 *
 * Must be called with scheduler locked.
 */
used static vm_map_t int_create(void)
{
  struct vm_map *map;

  // Allocate new map structure.
  if ((map = kmem_alloc(sizeof(*map))) == NULL)
    return NULL;

  map->refcnt = 1;
  map->total = 0;

  // Allocate new page directory.
  if ((map->pgd = mmu_newmap()) == NO_PGD) {
    kmem_free(map);
    return NULL;
  }

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

  sched_lock();
  seg = &map->head;
  do {
    if (seg->flags != SEG_FREE) {
      // Unmap segment.
      mmu_map(map->pgd, seg->phys, seg->addr, seg->size, PG_UNMAP);

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

  if (map == getcurmap()) {
    /* Switch to the kernel page directory before
     * deleting current page directory.
     */
    mmu_switch(kernel_map.pgd);
  }

  mmu_terminate(map->pgd);
  kmem_free(map);
  sched_unlock();
}

/** Duplicate specified virtual memory space.
 * This is called when new process is created.
 *
 * Returns new map id, NULL if it fails.
 *
 * All segments of original memory map are copied to new memory map.
 * If the segment is read-only, executable, or shared segment, it is
 * no need to copy. These segments are physically shared with the
 * original map.
 */
used static vm_map_t int_dup(vm_map_t org_map)
{
  vm_map_t new_map;

  sched_lock();
  new_map = do_dup(org_map);
  sched_unlock();
  return new_map;
}

static vm_map_t do_dup(vm_map_t org_map)
{
  vm_map_t new_map;
  struct seg *tmp, *src, *dest;
  int map_type;

  if ((new_map = vm_create()) == NULL)
    return NULL;

  new_map->total = org_map->total;
  // Copy all segments.
  tmp = &new_map->head;
  src = &org_map->head;

  // Copy top segment.
  *tmp = *src;
  tmp->next = tmp->prev = tmp;

  if (src == src->next)  		// Blank memory?
    return new_map;

  do {
    ASSERT(src != NULL);
    ASSERT(src->next != NULL);

    if (src == &org_map->head) {
      dest = tmp;
    } else {
      // Create new segment struct.
      dest = kmem_alloc(sizeof(*dest));
      if (dest == NULL)
        return NULL;

      *dest = *src;  // memcpy

      dest->prev = tmp;
      dest->next = tmp->next;
      tmp->next->prev = dest;
      tmp->next = dest;
      tmp = dest;
    }

    if (src->flags == SEG_FREE) {
      // Skip free segment.
    } else {
      // Check if the segment can be shared.
      if (!(src->flags & SEG_WRITE) &&
          !(src->flags & SEG_MAPPED)) {
        dest->flags |= SEG_SHARED;
      }

      if (!(dest->flags & SEG_SHARED)) {
        // Allocate new physical page.
        dest->phys = page_alloc(src->size);
        if (dest->phys == 0)
          return NULL;

        // Copy source page.
        memcpy(ptokv(dest->phys), ptokv(src->phys),
               src->size);
      }

      // Map the segment to virtual address.
      if (dest->flags & SEG_WRITE)
        map_type = PG_WRITE;
      else
        map_type = PG_READ;

      if (mmu_map(new_map->pgd, dest->phys, dest->addr, dest->size, map_type))
        return NULL;
    }

    src = src->next;
  } while (src != &org_map->head);

  // No error. Now, link all shared segments.
  dest = &new_map->head;
  src = &org_map->head;
  do {
    if (dest->flags & SEG_SHARED) {
      src->flags |= SEG_SHARED;
      dest->sh_prev = src;
      dest->sh_next = src->sh_next;
      src->sh_next->sh_prev = dest;
      src->sh_next = dest;
    }

    dest = dest->next;
    src = src->next;
  } while (src != &org_map->head);

  return new_map;
}

/** Switch VM mapping.
 *
 * Since a kernel process does not have user mode memory image, we
 * don't have to setup the page directory for it. Thus, an idle
 * thread and interrupt threads can be switched quickly.
 */
used static void int_switch(vm_map_t map)
{
  if (map != &kernel_map)
    mmu_switch(map->pgd);
}

/** Increment reference count of VM mapping.
 */
used static int int_reference(vm_map_t map)
{
  map->refcnt++;
  return 0;
}

#if RICH	// Don;t think I need this.
/** Load process image for boot process.
 * Return 0 on success, or errno on failure.
 */
used static int int_load(vm_map_t map, struct module *mod, void **stack)
{
  char *src;
  void *text, *data;
  int error;

  DPRINTF(("Loading process: %s\n", mod->name));

  /* We have to switch VM mapping to touch the virtual
   * memory space of a target process without page fault.
   */
  vm_switch(map);

  src = ptokv(mod->phys);
  text = (void *)mod->text;
  data = (void *)mod->data;

  // Create text segment.
  error = do_allocate(map, &text, mod->textsz, 0);
  if (error)
    return error;

  memcpy(text, src, mod->textsz);
  error = do_attribute(map, text, PROT_READ);
  if (error)
    return error;

  // Create data & BSS segment.
  if (mod->datasz + mod->bsssz != 0) {
    error = do_allocate(map, &data, mod->datasz + mod->bsssz, 0);
    if (error)
      return error;
    if (mod->datasz > 0) {
      src = src + (mod->data - mod->text);
      memcpy(data, src, mod->datasz);
    }
  }

  // Create stack.
  *stack = (void *)USRSTACK;
  error = do_allocate(map, stack, DFLSTKSZ, 0);
  if (error)
    return error;

  // Free original pages.
  page_free(mod->phys, mod->size);
  return 0;
}
#endif

/** Translate virtual address of current process to physical address.
 * Returns physical address on success, or NULL if no mapped memory.
 */
used static paddr_t int_translate(vaddr_t addr, size_t size)
{
  return mmu_extract(getcurmap()->pgd, addr, size);
}

used static int int_info(struct vminfo *info)
{
  u_long target = info->cookie;
  pid_t pid = info->pid;
  u_long i;
  vm_map_t map;
  struct seg *seg;

  sched_lock();
  if (!pid_valid(pid)) {
    sched_unlock();
    return -ESRCH;
  }

  map = getmap(pid);
  seg = &map->head;
  i = 0;
  do {
    if (i++ == target) {
      info->cookie = i;
      info->virt = seg->addr;
      info->size = seg->size;
      info->flags = seg->flags;
      info->phys = seg->phys;
      sched_unlock();
      return 0;
    }
    seg = seg->next;
  } while (seg != &map->head);

  sched_unlock();
  return -ESRCH;
}

used static vm_map_t int_init(void)
{
  pgd_t pgd;

  // Setup vm mapping for the kernel process.
  if ((pgd = mmu_newmap()) == NO_PGD)
    panic("vm_init");

  kernel_map.pgd = pgd;
  mmu_switch(pgd);

  seg_init(&kernel_map.head);
  return &kernel_map;
}


/** Initialize segment.
 */
static void seg_init(struct seg *seg)
{
  extern struct bootinfo bootinfo;
  seg->next = seg->prev = seg;
  seg->sh_next = seg->sh_prev = seg;
  seg->addr = PAGE_SIZE;
  seg->phys = 0;
  seg->size = bootinfo.userlimit - PAGE_SIZE;
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
  seg->phys = 0;
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

  seg = head;
  do {
    if ((seg->flags & SEG_FREE) && seg->size >= size) {
      if (seg->size != size) {
        /*
         * Split this segment and return its head.
         */
        if (seg_create(seg,
                 seg->addr + size,
                 seg->size - size) == NULL)
          return NULL;
      }

      seg->size = size;
      return seg;
    }

    seg = seg->next;
  } while (seg != head);

  return NULL;
}

/** Delete specified free segment.
 */
static void seg_free(struct seg *head, struct seg *seg)
{
  struct seg *prev, *next;

  ASSERT(seg->flags != SEG_FREE);

  seg->flags = SEG_FREE;

  // If it is shared segment, unlink from shared list.
  if (seg->flags & SEG_SHARED) {
    seg->sh_prev->sh_next = seg->sh_next;
    seg->sh_next->sh_prev = seg->sh_prev;
    if (seg->sh_prev == seg->sh_next)
      seg->sh_prev->flags &= ~SEG_SHARED;
  }

  // If next segment is free, merge with it.
  next = seg->next;
  if (next != head && (next->flags & SEG_FREE)) {
    seg->next = next->next;
    next->next->prev = seg;
    seg->size += next->size;
    kmem_free(next);
  }

  // If previous segment is free, merge with it.
  prev = seg->prev;
  if (seg != head && (prev->flags & SEG_FREE)) {
    prev->next = seg->next;
    seg->next->prev = prev;
    prev->size += seg->size;
    kmem_free(seg);
  }
}

/** Reserve the segment at the specified address/size.
 */
static struct seg *seg_reserve(struct seg *head, vaddr_t addr, size_t size)
{
  struct seg *seg, *prev, *next;
  size_t diff;

  // Find the block which includes specified block.
  seg = seg_lookup(head, addr, size);
  if (seg == NULL || !(seg->flags & SEG_FREE))
    return NULL;

  // Check previous segment to split segment.
  prev = NULL;
  if (seg->addr != addr) {
    prev = seg;
    diff = (size_t)(addr - seg->addr);
    seg = seg_create(prev, addr, prev->size - diff);
    if (seg == NULL)
      return NULL;

    prev->size = diff;
  }

  // Check next segment to split segment.
  if (seg->size != size) {
    next = seg_create(seg, seg->addr + size, seg->size - size);
    if (next == NULL) {
      if (prev) {
        // Undo previous seg_create() operation.
        seg_free(head, seg);
      }

      return NULL;
    }

    seg->size = size;
  }

  seg->flags = 0;
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
