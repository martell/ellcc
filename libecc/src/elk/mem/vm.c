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

#include <sys/mman.h>
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

// Forward declarations.
static void seg_init(struct seg *, size_t);
static struct seg *seg_create(struct seg *, vaddr_t, size_t);
static void seg_delete(struct seg *, struct seg *);
static struct seg *seg_merge(struct seg *, vaddr_t, size_t);
static struct seg *seg_lookup(struct seg *, vaddr_t, size_t);
static struct seg *seg_alloc(struct seg *, size_t);
static void seg_free(struct seg *, struct seg *);
static struct seg *seg_split(struct seg *, struct seg *, vaddr_t,
                             size_t, vm_map_t);
static struct seg *seg_reserve(struct seg *, vaddr_t, size_t);
static int do_allocate(vm_map_t, void **, size_t, int);
static int do_free(vm_map_t, void *, size_t);
static int do_attribute(vm_map_t, void *, size_t, int);
static int do_map(vm_map_t, void *, size_t, void **);
static vm_map_t do_dup(vm_map_t);

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

  // Allocate segment.
  struct seg *seg;
  vaddr_t start;
  if (anywhere) {
    size = round_page(size);
    if ((seg = seg_alloc(&map->head, size)) == NULL)
      return -ENOMEM;
  } else {
    start = trunc_page((vaddr_t)*addr);
    vaddr_t end = round_page(start + size);
    size = (size_t)(end - start);

    if ((seg = seg_reserve(&map->head, start, size)) == NULL)
      return -ENOMEM;
  }

  seg->flags = SEG_READ|SEG_WRITE;

  // Allocate physical pages, and map them into the virtual address.
  paddr_t pa;
  if ((pa = page_alloc(size)) == 0)
    goto err1;

  DPRINTF(MEMDB_VM,
          ("vm_allocate: physical block allocated 0x%08lx (%zu bytes)\n",
           pa, size));
  seg->phys = pa;
  if (mmu_map(map->pgd, pa, seg->addr, size, PG_WRITE))
    goto err2;


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
  paddr_t old_pa, new_pa;
  vaddr_t va;

  va = trunc_page((vaddr_t)addr);
  if (va != (vaddr_t)addr) {
    // The address must be a multiple of the system page size.
    return -EINVAL;
  }

  // Find the target segment.
  seg = seg_lookup(&map->head, va, 1);
  if (seg == NULL || (seg->flags & SEG_FREE)) {
    return -EINVAL;        // Not allocated.
  }

  // The attribute of the mapped segment can not be changed.
  if (seg->flags & SEG_MAPPED)
    return -EINVAL;

  if (seg->addr != va) {
    // Not at the beginning, try to split up the segment.
    seg = seg_split(&map->head, seg, va, size, map);
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
  int map_type = (new_flags & SEG_WRITE) ? PG_WRITE : PG_READ;

  // If it is shared segment, duplicate it.
  if (seg->flags & SEG_SHARED) {
    old_pa = seg->phys;

    // Allocate new physical pages.
    if ((new_pa = page_alloc(seg->size)) == 0)
      return -ENOMEM;

    DPRINTF(MEMDB_VM,
            ("vm_attribute: physical block allocated 0x%08lx (%zu bytes)\n",
              new_pa, seg->size));

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
    if (mmu_map(map->pgd, seg->phys, seg->addr, seg->size, map_type))
      return -ENOMEM;
  }

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

#if RICH // MAXMEM defined in param.h as 4MB. Need to allocate more page tables.
  if (map->total + size >= MAXMEM)
    return -ENOMEM;
#endif

  vaddr_t start = trunc_page((vaddr_t)addr);
  vaddr_t end = round_page((vaddr_t)addr + size);
  size = (size_t)(end - start);
  size_t offset = (size_t)((vaddr_t)addr - start);

  // Find the segment that includes target address.
  struct seg *seg = seg_lookup(&map->head, start, size);
  if (seg == NULL || (seg->flags & SEG_FREE))
    return -EINVAL;             // Not allocated.

  struct seg *tgt = seg;

  // Find the free segment in current process.
  vm_map_t curmap = getcurmap();
  if ((seg = seg_alloc(&curmap->head, size)) == NULL)
    return -ENOMEM;

  struct seg *cur = seg;

  // Try to map into current memory.
  int map_type;
  if (tgt->flags & SEG_WRITE)
    map_type = PG_WRITE;
  else
    map_type = PG_READ;

  paddr_t pa = tgt->phys + (paddr_t)(start - tgt->addr);
  if (mmu_map(curmap->pgd, pa, cur->addr, size, map_type)) {
    seg_free(&curmap->head, seg);
    return -ENOMEM;
  }

  cur->flags = tgt->flags | SEG_MAPPED;
  cur->phys = pa;

  *alloc = (void *)(cur->addr + offset);

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

  // Allocate new page directory.
  if ((map->pgd = mmu_newmap()) == NO_PGD) {
    kmem_free(map);
    return NULL;
  }

  seg_init(&map->head, 0);
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
  pthread_mutex_unlock(&map->lock);
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
static vm_map_t int_dup(vm_map_t map)
{

  pthread_mutex_lock(&map->lock);
  vm_map_t new_map = do_dup(map);
  pthread_mutex_unlock(&map->lock);
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

  if (src == src->next)      // Blank memory?
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
        // Allocate new physical pages.
        dest->phys = page_alloc(src->size);
        if (dest->phys == 0)
          return NULL;

        DPRINTF(MEMDB_VM,
                ("vm_dup: physical block allocated 0x%08lx (%zu bytes)\n",
                  dest->phys, src->size));
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
static void int_switch(vm_map_t map)
{
  if (map != &kernel_map)
    mmu_switch(map->pgd);
}

/** Increment reference count of VM mapping.
 */
static int int_reference(vm_map_t map)
{
  map->refcnt++;
  return 0;
}

/** Map a physical page into a virtual page.
 * This is used for early access to I/O registers.
 */
static void int_premap(paddr_t paddr, vaddr_t vaddr)
{
  mmu_premap(paddr, vaddr);
}

/** Translate virtual address of current process to physical address.
 * Returns physical address on success, or NULL if no mapped memory.
 */
static paddr_t int_translate(vaddr_t addr, size_t size)
{
  return mmu_extract(getcurmap()->pgd, addr, size);
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

/* RICH: Temporary. Hacked for vexpress-a9
 * This table will move.
 */
/*
 * Virtual and physical address mapping
 *
 *      { virtual, physical, size, type }
 */
#if defined(__arm__)
#include "arm_sp804.h"
#include "arm_priv.h"
#include "pl011.h"
#endif
static void int_mmu_init(void)
{
  const struct mmumap mmumap_table[] =
  {
#if defined(__arm__)
    // RAM
    { 0x80000000, 0x48000000, 0x2000000, VMT_RAM },

    // Counter/Timers.
    { SP804_BASE, SP804_PHYSICAL_BASE, SP804_SIZE, VMT_IO },

    // Private memory.
    { ARM_PRIV_BASE, ARM_PRIV_PHYSICAL_BASE, ARM_PRIV_SIZE, VMT_IO },

    // UART 0.
    { PL011_BASE, PL011_PHYSICAL_BASE, PL011_SIZE, VMT_IO },

    // LAN9118. RICH: Better way.
    { 0xdb000000, 0x4e000000, 0x100000, VMT_IO },
#endif
    { 0,0,0,0 }
  };

  mmu_init(mmumap_table);
}

static vm_map_t int_init(size_t kmem)
{
  pgd_t pgd;

  kernel_map.refcnt = 1;

  // Setup vm mapping for the kernel process.
  if ((pgd = mmu_newmap()) == NO_PGD)
    panic("vm_init");
  mmu_switch(pgd);

  kernel_map.pgd = pgd;
  seg_init(&kernel_map.head, kmem);
  return &kernel_map;
}

/** Initialize segment.
 */
static void seg_init(struct seg *seg, size_t kmem)
{
  extern char __end[];            // The end of the kernel .bss area.
  seg->next = seg->prev = seg;
  seg->sh_next = seg->sh_prev = seg;
  // RICH: Leav room for kmem_alloc(). How much?
  seg->addr = kmem ? round_page((uintptr_t)__end + kmem): PAGE_SIZE;
  seg->phys = 0;
  seg->size = (kmem ? 0L - seg->addr : USERLIMIT) - PAGE_SIZE;
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
          sp->next->prev = seg;
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

  // If it is shared segment, unlink from shared list.
  if (seg->flags & SEG_SHARED) {
    seg->sh_prev->sh_next = seg->sh_next;
    seg->sh_next->sh_prev = seg->sh_prev;

    if (seg->sh_prev == seg->sh_next)
      seg->sh_prev->flags &= ~SEG_SHARED;
  }

  seg->flags = SEG_FREE;

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

/** Split the segment at the specified address/size.
 */
static struct seg *seg_split(struct seg *head, struct seg *seg,
                             vaddr_t addr, size_t size, vm_map_t map)
{
  struct seg *prev, *next;
  size_t diff;

  // Check previous segment to split segment.
  prev = NULL;
  int flags = seg->flags;       // Save the original flags.
  if (seg->addr != addr) {
    prev = seg;
    diff = (size_t)(addr - seg->addr);
    seg = seg_create(prev, addr, prev->size - diff);
    if (seg == NULL)
      return NULL;

    seg->flags = prev->flags;
    if (!(seg->flags & SEG_FREE)) {
      ASSERT(map != NULL);
      seg->phys = prev->phys + diff;
      int map_type;
      if (flags & SEG_WRITE)
        map_type = PG_WRITE;
      else if (flags & SEG_READ)
        map_type = PG_READ;
      else
        map_type = PG_UNMAP;

      if (mmu_map(map->pgd, seg->phys, seg->addr, size, map_type))
        return NULL;
    }

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

    next->flags = seg->flags;
    if (!(seg->flags & SEG_FREE)) {
      ASSERT(map != NULL);
      next->phys = seg->phys + size;
      if (mmu_map(map->pgd, seg->phys, seg->addr, size, PG_WRITE)) {
        // Undo previous seg_create() operation.
        seg_free(head, seg);
        return NULL;
      }
    }
    seg->size = size;
  }

  return seg;
}

/** Reserve the segment at the specified address/size.
 */
static struct seg *seg_reserve(struct seg *head, vaddr_t addr, size_t size)
{
  // Find the block which includes specified block.
  struct seg *seg = seg_lookup(head, addr, size);
  if (seg == NULL || !(seg->flags & SEG_FREE))
    return NULL;

  // Split the block from the previous and next.
  struct seg *new = seg_split(head, seg, addr, size, NULL);
  if (new == NULL)
    return NULL;

  new->flags = 0;                       // Clear the SEG_FREE flag.
  return new;
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
