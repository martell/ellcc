/*
 * Copyright (c) 2005-2008, Kohsuke Ohtani
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
 * vfs_vnode.c - vnode service
 */

#include <sys/param.h>
#include <limits.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "list.h"
#include "kmem.h"
#include "vnode.h"
#include "mount.h"
#include "vfs.h"
#include "command.h"

/*
 * Memo:
 *
 * Function   Ref count Lock
 * ---------- --------- ----------
 * vn_lock     *        Lock RO or RW
 * vn_lock     *        Lock R/W. Must have been RO or R/W locked previously.
 * vn_unlock   *        Unlock
 * vget        1        Lock RO
 * vput       -1        Unlock
 * vref       +1        *
 * vrele      -1        *
 */

#define VNODE_BUCKETS 32    /* size of vnode hash table */

/*
 * vnode table.
 * All active (opened) vnodes are stored on this hash table.
 * They can be accessed by its path name.
 */
static struct list vnode_table[VNODE_BUCKETS];

/** Anonymous vnode list.
 */
static struct list anon_vnodes;

/*
 * Global lock to access all vnodes and vnode table.
 * If a vnode is already locked, there is no need to
 * lock this global lock to access internal data.
 */
static pthread_mutex_t vnode_lock = PTHREAD_MUTEX_INITIALIZER;
#define VNODE_LOCK() pthread_mutex_lock(&vnode_lock)
#define VNODE_UNLOCK() pthread_mutex_unlock(&vnode_lock)

static int VP_LOCK_INIT(vnode_t vp_ro)
{
  vnode_rw_t vp = VN_RW_OVERRIDE(vp_ro);
  int s = pthread_mutex_init(&vp->v_interlock, NULL);
  if (s)
    return -s;

  s = sem_init(&vp->v_wait, 0, 0);
  if (s)
    return -errno;

  vp->v_nrlocks = 0;
  vp->v_flags &= ~VLOCKFLAGS;
  return 0;
}

static int VP_LOCK_DESTROY(vnode_t vp_ro)
{
  vnode_rw_t vp = VN_RW_OVERRIDE(vp_ro);
  int s = pthread_mutex_destroy(&vp->v_interlock);
  if (s)
    return -s;

  s = sem_destroy(&vp->v_wait);
  if (s)
    return -errno;

  return 0;
}

static int VP_LOCK(vnode_t vp_ro, int flags)
{
  vnode_rw_t vp = VN_RW_OVERRIDE(vp_ro);

  // Check flags for validity.
  if ((flags & (LK_SHARED|LK_EXCLUSIVE)) == (LK_SHARED|LK_EXCLUSIVE) ||
      (flags & (LK_SHARED|LK_EXCLUSIVE)) == 0) {
    // Need one and only one flag set.
    return -EINVAL;
  }

  int s;
  do {
    s  = pthread_mutex_lock(&vp->v_interlock);
    if (s)
      return -s;

    if ((vp->v_flags & (VSHARED|VEXCLUSIVE)) == 0 ||
        ((vp->v_flags & VSHARED) && (flags & LK_SHARED))) {
      // Not locked yet or currently sharable.
      if (flags & LK_SHARED) {
        vp->v_flags |= VSHARED;
      } else {
        // Exclusive use.
        vp->v_flags |= VEXCLUSIVE;
      }

      ++vp->v_nrlocks;
      pthread_mutex_unlock(&vp->v_interlock);
      return 0;
    }

    if (flags & LK_NOWAIT) {
      pthread_mutex_unlock(&vp->v_interlock);
      return -EAGAIN;           // Don't wait.
    }

    // The vnode is locked. Wait for it to be unlocked.
    vp->v_flags |= VWAITER;
    pthread_mutex_unlock(&vp->v_interlock);
    sem_wait(&vp->v_wait);
  } while (1);
}

int VP_UNLOCK(vnode_t vp_ro)
{
  vnode_rw_t vp = VN_RW_OVERRIDE(vp_ro);
  int s  = pthread_mutex_lock(&vp->v_interlock);
  if (s) {
    return -s;
  }

  if (vp->v_nrlocks == 0) {
    pthread_mutex_unlock(&(vp)->v_interlock);
    return -EINVAL;
  }

  --vp->v_nrlocks;
  if (vp->v_nrlocks == 0) {
    // The last lock is gone.
    vp->v_flags &= ~(VEXCLUSIVE|VSHARED);
    if (vp->v_flags & VWAITER) {
      vp->v_flags &= ~VWAITER;
      sem_post(&vp->v_wait);
    }
  }

  pthread_mutex_unlock(&(vp)->v_interlock);
  return 0;
}

/** Transition a vnode from RO to RW.
 * This function should be called on a vnode that is already locked.
 * If the vnode is locked VEXCLUSIVE, this is a NOOP.
 * If the vnode is locked VSHARED, it is transitioned to VEXCLUSIVE.
 */
vnode_rw_t vn_lock_rw(vnode_t vp_ro)
{
  vnode_rw_t vp = VN_RW_OVERRIDE(vp_ro);
  ASSERT(vp->v_flags & (VEXCLUSIVE|VSHARED));
  ASSERT(vp->v_nrlocks >= 1);
  if (vp->v_flags & VEXCLUSIVE)
    return vp;

  int s = pthread_mutex_lock(&vp->v_interlock);
  ASSERT(s == 0);

  if (vp->v_nrlocks == 1) {
    // Shared with only one lock.
    vp->v_flags &= ~VSHARED;
    vp->v_flags |= VEXCLUSIVE;
    pthread_mutex_unlock(&(vp)->v_interlock);
    return vp;
  }

  --vp->v_nrlocks;      // Remove the old shared lock.
  pthread_mutex_unlock(&(vp)->v_interlock);
  vn_lock(vp, LK_EXCLUSIVE|LK_RETRY);
  return vp;
}

/*
 * Get the hash value from the mount point and path name.
 */
static u_int vn_hash(mount_t mp, char *path)
{
  u_int val = 0;

  if (path) {
    while (*path)
      val = ((val << 5) + val) + *path++;
  }
  return (val ^ (u_int)mp) & (VNODE_BUCKETS - 1);
}

/*
 * Returns locked vnode for specified mount point and path.
 * vn_lock() will increment the reference count of vnode.
 */
vnode_t vn_lookup(mount_t mp, char *path)
{
  list_t head, n;
  vnode_t vp;

  VNODE_LOCK();
  head = &vnode_table[vn_hash(mp, path)];
  for (n = list_first(head); n != head; n = list_next(n)) {
    vp = list_entry(n, struct vnode, v_link);
    if (vp->v_mount == mp &&
        !strncmp(vp->v_path, path, PATH_MAX)) {
      VN_RW_OVERRIDE(vp)->v_refcnt++;
      VNODE_UNLOCK();
      VP_LOCK(vp, LK_SHARED|LK_RETRY);
      return vp;
    }
  }
  VNODE_UNLOCK();
  return NULL;    /* not found */
}

/*
 * Lock vnode
 */
void vn_lock(vnode_t vp, int flags)
{
  ASSERT(vp);
  ASSERT(vp->v_refcnt > 0);

  VP_LOCK(vp, flags);
  DPRINTF(VFSDB_VNODE, ("vn_lock:   %s\n", vp->v_path));
}

/*
 * Unlock vnode
 */
void vn_unlock(vnode_t vp)
{
  ASSERT(vp);
  ASSERT(vp->v_refcnt > 0);
  ASSERT(vp->v_nrlocks > 0);

  DPRINTF(VFSDB_VNODE, ("vn_unlock: %s\n", vp->v_path));
  VP_UNLOCK(vp);
}

/*
 * Allocate new vnode for specified path.
 * Increment its reference count and lock it.
 */
vnode_t vget(mount_t mp, char *path)
{
  vnode_rw_t vp;
  int error;
  size_t len;

  DPRINTF(VFSDB_VNODE, ("vget: %s\n", path));

  if (!(vp = kmem_alloc(sizeof(struct vnode))))
    return NULL;
  *vp = (struct vnode){};

  if (path) {
    len = strlen(path) + 1;
    if (!(vp->v_path = kmem_alloc(len))) {
      kmem_free(vp);
      return NULL;
    }
    strlcpy(vp->v_path, path, len);
  }
  vp->v_refcnt = 1;
  VP_LOCK_INIT(vp);
  if (mp) {
    vp->v_mount = mp;
    vp->v_op = mp->m_op->vfs_vnops;

    /*
     * Request to allocate fs specific data for vnode.
     */
    if ((error = VFS_VGET(mp, vp)) != 0) {
      VP_LOCK_DESTROY(vp);
      kmem_free(vp->v_path);
      kmem_free(vp);
      return NULL;
    }
    vfs_busy(vp->v_mount);
  }

  VP_LOCK(vp, LK_SHARED|LK_RETRY);

  VNODE_LOCK();
  if (mp) {
    list_insert(&vnode_table[vn_hash(mp, path)], &vp->v_link);
  } else {
    list_insert(&anon_vnodes, &vp->v_link);
  }
  VNODE_UNLOCK();
  return vp;
}

/** Bind an anonymous vnode to a file system name.
 */
int  vbind(mount_t mp, vnode_t vp_ro, char *path)
{
  vnode_rw_t vp = vn_lock_rw(vp_ro);
  vp->v_mount = mp;
  int len = strlen(path) + 1;
  if (!(vp->v_path = kmem_alloc(len))) {
    kmem_free(vp);
    return -ENOMEM;
  }
  strlcpy(vp->v_path, path, len);
  VNODE_LOCK();
  list_remove(&vp->v_link);     // Remove from the anonymous list.
  list_insert(&vnode_table[vn_hash(mp, path)], &vp->v_link);
  VNODE_UNLOCK();
  vn_unlock(vp);
  return 0;
}

/*
 * Unlock vnode and decrement its reference count.
 */
void vput(vnode_t vp_ro)
{
  vnode_rw_t vp = vn_lock_rw(vp_ro);
  ASSERT(vp);
  ASSERT(vp->v_nrlocks > 0);
  ASSERT(vp->v_refcnt > 0);
  DPRINTF(VFSDB_VNODE, ("vput: ref=%d %s\n", vp->v_refcnt,
            vp->v_path));

  vp->v_refcnt--;
  if (vp->v_refcnt > 0) {
    vn_unlock(vp);
    return;
  }

  VNODE_LOCK();
  list_remove(&vp->v_link);
  VNODE_UNLOCK();

  /*
   * Deallocate fs specific vnode data
   */
  VOP_INACTIVE(vp);
  if (vp->v_mount) vfs_unbusy(vp->v_mount);
  VP_UNLOCK(vp);
  ASSERT(vp->v_nrlocks == 0);
  VP_LOCK_DESTROY(vp);
  kmem_free(vp->v_path);
  kmem_free(vp);
}

/*
 * Increment the reference count on an active vnode.
 */
void vref(vnode_t vp_ro)
{
  vnode_rw_t vp = VN_RW_OVERRIDE(vp_ro);
  ASSERT(vp);
  ASSERT(vp->v_refcnt > 0);  /* Need vget */

  VNODE_LOCK();
  DPRINTF(VFSDB_VNODE, ("vref: ref=%d %s\n", vp->v_refcnt,
            vp->v_path));
  vp->v_refcnt++;
  VNODE_UNLOCK();
}

/*
 * Decrement the reference count of the vnode.
 * Any code in the system which is using vnode should call vrele()
 * when it is finished with the vnode.
 * If count drops to zero, call inactive routine and return to freelist.
 */
void vrele(vnode_t vp_ro)
{
  vnode_rw_t vp = VN_RW_OVERRIDE(vp_ro);
  ASSERT(vp);
  ASSERT(vp->v_refcnt > 0);

  VNODE_LOCK();
  DPRINTF(VFSDB_VNODE, ("vrele: ref=%d %s\n", vp->v_refcnt,
            vp->v_path));
  vp->v_refcnt--;
  if (vp->v_refcnt > 0) {
    VNODE_UNLOCK();
    return;
  }
  list_remove(&vp->v_link);
  VNODE_UNLOCK();

  /*
   * Deallocate fs specific vnode data
   */
  VOP_INACTIVE(vp);
  if (vp->v_mount) vfs_unbusy(vp->v_mount);
  VP_LOCK_DESTROY(vp);
  kmem_free(vp->v_path);
  kmem_free(vp);
}

/*
 * vgone() is called when unlocked vnode is no longer valid.
 */
void vgone(vnode_t vp_ro)
{
  vnode_rw_t vp = VN_RW_OVERRIDE(vp_ro);
  ASSERT(vp->v_nrlocks == 0);

  VNODE_LOCK();
  DPRINTF(VFSDB_VNODE, ("vgone: %s\n", vp->v_path));
  list_remove(&vp->v_link);
  if (vp->v_mount) vfs_unbusy(vp->v_mount);
  VP_LOCK_DESTROY(vp);
  kmem_free(vp->v_path);
  kmem_free(vp);
  VNODE_UNLOCK();
}

/*
 * Return reference count.
 */
int vcount(vnode_t vp)
{
  int count;

  vn_lock(vp, LK_SHARED|LK_RETRY);
  count = vp->v_refcnt;
  vn_unlock(vp);
  return count;
}

/*
 * Remove all vnode in the vnode table for unmount.
 */
void vflush(mount_t mp)
{
  int i;
  list_t head, n;
  vnode_t vp;

  VNODE_LOCK();
  for (i = 0; i < VNODE_BUCKETS; i++) {
    head = &vnode_table[i];
    for (n = list_first(head); n != head; n = list_next(n)) {
      vp = list_entry(n, struct vnode, v_link);
      if (vp->v_mount == mp) {
        /* XXX: */
      }
    }
  }
  VNODE_UNLOCK();
}

int vn_stat(vnode_t vp, struct stat *st)
{
  mode_t mode;

  memset(st, 0, sizeof(struct stat));

  st->st_ino = (ino_t)vp;
  st->st_size = vp->v_size;
  mode = vp->v_mode;
  switch (vp->v_type) {
  case VREG:
    mode |= S_IFREG;
    break;
  case VDIR:
    mode |= S_IFDIR;
    break;
  case VBLK:
    mode |= S_IFBLK;
    break;
  case VCHR:
    mode |= S_IFCHR;
    break;
  case VLNK:
    mode |= S_IFLNK;
    break;
  case VSOCK:
    mode |= S_IFSOCK;
    break;
  case VFIFO:
    mode |= S_IFIFO;
    break;
  default:
    return -EBADF;
  };
  st->st_mode = mode;
  st->st_blksize = DEV_BSIZE;
  st->st_blocks = vp->v_size / S_BLKSIZE;
  st->st_uid = 0;
  st->st_gid = 0;
  if (vp->v_type == VCHR || vp->v_type == VBLK)
    st->st_rdev = (dev_t)vp->v_data;

  return 0;
}

/*
 * Chceck permission on vnode pointer.
 */
int vn_access(vnode_t vp, int flags)
{
  int error = 0;

  if ((flags & VEXEC) && (vp->v_mode & 0111) == 0) {
    error = -EACCES;
    goto out;
  }
  if ((flags & VREAD) && (vp->v_mode & 0444) == 0) {
    error = -EACCES;
    goto out;
  }
  if (flags & VWRITE) {
    if (vp->v_mount && vp->v_mount->m_flags & MNT_RDONLY) {
      error = -EROFS;
      goto out;
    }
    if ((vp->v_mode & 0222) == 0) {
      error = -EACCES;
      goto out;
    }
  }
 out:
  return error;
}

int vop_nullop(void)
{
  return 0;
}

int vop_einval(void)
{
  return -EINVAL;
}

#if CONFIG_VFS_COMMANDS
/*
 * List all vnodes.
 */
static int vsCommand(int argc, char **argv)
{
  if (argc <= 0) {
    printf("list all vnodes.\n");
    return COMMAND_OK;
  }

  int i;
  list_t head, n;
  vnode_t vp;
  mount_t mp;
  static const char type[][6] = { "VNON ", "VREG ", "VDIR ", "VBLK ", "VCHR ",
         "VLNK ", "VSOCK", "VFIFO" };

  VNODE_LOCK();
  printf("vnode    mount    type  refcnt blkno    path\n");
  printf("-------- -------- ----- ------ -------- ------------------------------\n");

  for (i = 0; i < VNODE_BUCKETS; i++) {
    head = &vnode_table[i];
    for (n = list_first(head); n != head; n = list_next(n)) {
      vp = list_entry(n, struct vnode, v_link);
      mp = vp->v_mount;

      printf("%08x %08x %s %6d %8d %s%s\n", (u_int)vp,
        (u_int)mp, type[vp->v_type], vp->v_refcnt,
        (u_int)vp->v_blkno,
        (strlen(mp->m_path) == 1) ? "\0" : mp->m_path,
        vp->v_path);
    }
  }
  head = &anon_vnodes;
  for (n = list_first(head); n != head; n = list_next(n)) {
    vp = list_entry(n, struct vnode, v_link);
    printf("%08x %08x %s %6d %8d anonymous\n", (u_int)vp,
      (u_int)mp, type[vp->v_type], vp->v_refcnt,
      (u_int)vp->v_blkno);
  }
  VNODE_UNLOCK();

  return COMMAND_OK;
}

/* Change directory.
 */
static int cdCommand(int argc, char **argv)
{
  if (argc <= 0) {
    printf("change the current working directory\n");
    return COMMAND_OK;
  }

  if (argc != 2) {
    printf("usage: cd <directory>\n");
    return COMMAND_ERROR;
  }

  int s;
  if ((s = chdir(argv[1])) != 0) {
    printf("cd failed: %s\n", strerror(errno));
    return COMMAND_ERROR;
  }

  return COMMAND_OK;
}

/* Change the root directory.
 */
static int chrootCommand(int argc, char **argv)
{
  if (argc <= 0) {
    printf("change the root directory\n");
    return COMMAND_OK;
  }

  if (argc != 2) {
    printf("usage: chroot <directory>\n");
    return COMMAND_ERROR;
  }

  int s;
  if ((s = chroot(argv[1])) != 0) {
    printf("chroot failed: %s\n", strerror(errno));
    return COMMAND_ERROR;
  }

  return COMMAND_OK;
}

#endif // CONFIG_VFS_COMMANDS

ELK_PRECONSTRUCTOR()
{
  for (int i = 0; i < VNODE_BUCKETS; i++)
    list_init(&vnode_table[i]);

  list_init(&anon_vnodes);

#if CONFIG_VFS_COMMANDS
  command_insert("vs", vsCommand);
  command_insert("cd", cdCommand);
  command_insert("chroot", chrootCommand);
#endif
}
