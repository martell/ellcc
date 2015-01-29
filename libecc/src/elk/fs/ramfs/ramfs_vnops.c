/*
 * Copyright (c) 2006-2007, Kohsuke Ohtani
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
 * ramfs_vnops.c - vnode operations for RAM file system.
 */

#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/param.h>

#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>

#include "kmem.h"
#include "vnode.h"
#include "file.h"
#include "mount.h"
#include "thread.h"
#include "page.h"
#include "vm.h"

FEATURE(ramfs)

/*
 * File/directory node for RAMFS
 */
struct ramfs_node
{
  struct ramfs_node *rn_next;   // Next node in the same directory.
  struct ramfs_node *rn_child;  // First child node.
  int rn_type;                  // File or directory.
  char *rn_name;                // Name (null-terminated).
  size_t rn_namelen;            // Length of name not including terminator.
  size_t rn_size;               // File size.
  char *rn_buf;                 // Buffer to the file data.
  size_t rn_bufsize;            // Allocated buffer size.
  mode_t rn_mode;               // File access mode.
};

#define ramfs_open ((vnop_open_t)vop_nullop)
#define ramfs_close ((vnop_close_t)vop_nullop)
static int ramfs_read(vnode_t, file_t, struct uio *, size_t *);
static int ramfs_write(vnode_t, file_t, struct uio *, size_t *);
#define ramfs_poll ((vnop_poll_t)vop_nullop)
#define ramfs_seek ((vnop_seek_t)vop_nullop)
#define ramfs_ioctl ((vnop_ioctl_t)vop_einval)
#define ramfs_fsync ((vnop_fsync_t)vop_nullop)
static int ramfs_readdir(vnode_t, file_t, struct dirent *);
static int ramfs_lookup(vnode_t, char *, vnode_t);
static int ramfs_create(vnode_t, char *, mode_t);
static int ramfs_remove(vnode_t, vnode_t, char *);
static int ramfs_rename(vnode_t, vnode_t, char *, vnode_t, vnode_t, char *);
static int ramfs_mkdir(vnode_t, char *, mode_t);
static int ramfs_rmdir(vnode_t, vnode_t, char *);
#define ramfs_getattr ((vnop_getattr_t)vop_nullop)
#define ramfs_setattr ((vnop_setattr_t)vop_nullop)
#define ramfs_inactive ((vnop_inactive_t)vop_nullop)
static int ramfs_truncate(vnode_t, off_t);

static pthread_mutex_t ramfs_lock = PTHREAD_MUTEX_INITIALIZER;
#define RAMFS_LOCK() pthread_mutex_lock(&ramfs_lock)
#define RAMFS_UNLOCK() pthread_mutex_unlock(&ramfs_lock)

/*
 * vnode operations
 */
static struct vnops ramfs_vnops = {
  ramfs_open,
  ramfs_close,
  ramfs_read,
  ramfs_write,
  ramfs_poll,
  ramfs_seek,
  ramfs_ioctl,
  ramfs_fsync,
  ramfs_readdir,
  ramfs_lookup,
  ramfs_create,
  ramfs_remove,
  ramfs_rename,
  ramfs_mkdir,
  ramfs_rmdir,
  ramfs_getattr,
  ramfs_setattr,
  ramfs_inactive,
  ramfs_truncate,
};

static int ramfs_mount(mount_t mp, char *dev, int flags, void *data);
static int ramfs_unmount(mount_t mp);
#define ramfs_sync  ((vfsop_sync_t)vfs_nullop)
#define ramfs_vget  ((vfsop_vget_t)vfs_nullop)
#define ramfs_statfs  ((vfsop_statfs_t)vfs_nullop)

/*
 * File system operations
 */
static struct vfsops ramfs_vfsops = {
  ramfs_mount,
  ramfs_unmount,
  ramfs_sync,
  ramfs_vget,
  ramfs_statfs,
  &ramfs_vnops,
};

static struct ramfs_node *ramfs_allocate_node(char *name, int type,
                                              mode_t mode);

/*
 * Mount a file system.
 */
static int ramfs_mount(mount_t mp, char *dev, int flags, void *data)
{
  struct ramfs_node *np;

  DPRINTF(AFSDB_CORE, ("ramfs_mount: dev=%s\n", dev));

  /* Create a root node */
  np = ramfs_allocate_node("/", VDIR, ACCESSPERMS);
  if (np == NULL)
    return -ENOMEM;
  vnode_rw_t vp = vn_lock_rw(mp->m_root);
  vp->v_data = np;
  return 0;
}

/*
 * Unmount a file system.
 *
 * NOTE: Currently, we don't support unmounting of the RAMFS. This is
 *       because we have to deallocate all nodes included in all sub
 *       directories, and it requires more work...
 */
static int ramfs_unmount(mount_t mp)
{
  return -EBUSY;
}

static struct ramfs_node *ramfs_allocate_node(char *name, int type, mode_t mode)
{
  struct ramfs_node *np;

  np = kmem_alloc(sizeof(struct ramfs_node));
  if (np == NULL)
    return NULL;
  memset(np, 0, sizeof(struct ramfs_node));

  np->rn_namelen = strlen(name);
  np->rn_name = kmem_alloc(np->rn_namelen + 1);
  if (np->rn_name == NULL) {
    kmem_free(np);
    return NULL;
  }
  strlcpy(np->rn_name, name, np->rn_namelen + 1);
  np->rn_type = type;
  np->rn_mode = mode;
  return np;
}

static void ramfs_free_node(struct ramfs_node *np)
{
  kmem_free(np->rn_name);
  kmem_free(np);
}

static struct ramfs_node *ramfs_add_node(struct ramfs_node *dnp, char *name,
                                         int type, mode_t mode)
{
  struct ramfs_node *np, *prev;

  np = ramfs_allocate_node(name, type, mode);
  if (np == NULL)
    return NULL;

  RAMFS_LOCK();

  /* Link to the directory list */
  if (dnp->rn_child == NULL) {
    dnp->rn_child = np;
  } else {
    prev = dnp->rn_child;
    while (prev->rn_next != NULL)
      prev = prev->rn_next;
    prev->rn_next = np;
  }
  RAMFS_UNLOCK();
  return np;
}

static int ramfs_remove_node(struct ramfs_node *dnp, struct ramfs_node *np)
{
  struct ramfs_node *prev;

  if (dnp->rn_child == NULL)
    return -EBUSY;

  RAMFS_LOCK();

  /* Unlink from the directory list */
  if (dnp->rn_child == np) {
    dnp->rn_child = np->rn_next;
  } else {
    for (prev = dnp->rn_child; prev->rn_next != np;
         prev = prev->rn_next) {
      if (prev->rn_next == NULL) {
        RAMFS_UNLOCK();
        return -ENOENT;
      }
    }
    prev->rn_next = np->rn_next;
  }
  ramfs_free_node(np);

  RAMFS_UNLOCK();
  return 0;
}

static int ramfs_rename_node(struct ramfs_node *np, char *name)
{
  size_t len;
  char *tmp;

  len = strlen(name);
  if (len <= np->rn_namelen) {
    /* Reuse current name buffer */
    strlcpy(np->rn_name, name, sizeof(np->rn_name));
  } else {
    /* Expand name buffer */
    tmp = kmem_alloc(len + 1);
    if (tmp == NULL)
      return -ENOMEM;
    strlcpy(tmp, name, len + 1);
    kmem_free(np->rn_name);
    np->rn_name = tmp;
  }
  np->rn_namelen = len;
  return 0;
}

static int ramfs_lookup(vnode_t dvp, char *name, vnode_t vp_ro)
{
  struct ramfs_node *np, *dnp;
  size_t len;
  int found;

  if (*name == '\0')
    return -ENOENT;

  RAMFS_LOCK();

  len = strlen(name);
  dnp = dvp->v_data;
  found = 0;
  for (np = dnp->rn_child; np != NULL; np = np->rn_next) {
    if (np->rn_namelen == len &&
        memcmp(name, np->rn_name, len) == 0) {
      found = 1;
      break;
    }
  }
  if (found == 0) {
    RAMFS_UNLOCK();
    return -ENOENT;
  }

  vnode_rw_t vp = vn_lock_rw(vp_ro);
  vp->v_data = np;
  vp->v_mode = np->rn_mode;
  vp->v_type = np->rn_type;
  vp->v_size = np->rn_size;

  RAMFS_UNLOCK();
  return 0;
}

static int ramfs_mkdir(vnode_t dvp, char *name, mode_t mode)
{
  struct ramfs_node *np;

  DPRINTF(AFSDB_CORE, ("mkdir %s\n", name));
  if (!S_ISDIR(mode))
    return -EINVAL;

  np = ramfs_add_node(dvp->v_data, name, VDIR, mode);
  if (np == NULL)
    return -ENOMEM;
  np->rn_size = 0;
  return 0;
}

/* Remove a directory */
static int ramfs_rmdir(vnode_t dvp, vnode_t vp, char *name)
{
  return ramfs_remove_node(dvp->v_data, vp->v_data);
}

/* Remove a file */
static int ramfs_remove(vnode_t dvp, vnode_t vp, char *name)
{
  struct ramfs_node *np;
  int error;

  DPRINTF(AFSDB_CORE, ("remove %s in %s\n", name, dvp->v_path));
  error = ramfs_remove_node(dvp->v_data, vp->v_data);
  if (error)
    return error;

  np = vp->v_data;
  if (np->rn_buf != NULL)
    vm_free(getpid(), np->rn_buf, np->rn_bufsize);
  return 0;
}

/* Truncate file */
static int ramfs_truncate(vnode_t vp, off_t length)
{
  struct ramfs_node *np;
  void *new_buf;
  size_t new_size;

  DPRINTF(AFSDB_CORE, ("truncate %s length=%lld\n", vp->v_path, (long long)length));
  np = vp->v_data;

  if (length == 0) {
    if (np->rn_buf != NULL) {
      vm_free(getpid(), np->rn_buf, np->rn_bufsize);
      np->rn_buf = NULL;
      np->rn_bufsize = 0;
    }
  } else if (length > np->rn_bufsize) {
    new_size = round_page(length);
    if (vm_allocate(getpid(), &new_buf, new_size, 1))
      return -EIO;
    if (np->rn_size != 0) {
      memcpy(new_buf, np->rn_buf, vp->v_size);
      vm_free(getpid(), np->rn_buf, np->rn_bufsize);
    }
    np->rn_buf = new_buf;
    np->rn_bufsize = new_size;
  }
  np->rn_size = length;
  vn_lock_rw(vp)->v_size = length;
  return 0;
}

/*
 * Create empty file.
 */
static int ramfs_create(vnode_t dvp, char *name, mode_t mode)
{
  struct ramfs_node *np;

  DPRINTF(AFSDB_CORE, ("create %s in %s\n", name, dvp->v_path));
  int type;

  if (S_ISREG(mode)) {
    type = VREG;
  } else if (S_ISBLK(mode)) {
    type = VBLK;
  } else if (S_ISCHR(mode)) {
    type = VCHR;
  } else if (S_ISSOCK(mode)) {
    type = VSOCK;
  } else if (S_ISFIFO(mode)) {
    type = VFIFO;
  } else {
    return -EINVAL;
  }

  np = ramfs_add_node(dvp->v_data, name, type, mode);
  if (np == NULL)
    return -ENOMEM;
  return 0;
}

static int ramfs_read(vnode_t vp, file_t fp, struct uio *uio, size_t *result)
{
  struct ramfs_node *np;
  off_t off;

  *result = 0;
  if (vp->v_type == VDIR)
    return -EISDIR;
  if (vp->v_type != VREG)
    return -EINVAL;

  off = fp->f_offset;
  size_t total = 0;
  const struct iovec *iov = uio->iov;
  for (int i = 0; i < uio->iovcnt; ++i) {
    if (off >= (off_t)vp->v_size)
      return 0;

    size_t size = iov->iov_len;
    if (vp->v_size - off < size)
      size = vp->v_size - off;

    np = vp->v_data;
    memcpy(iov->iov_base, np->rn_buf + off, size);
    off += size;
    total += size;
    ++iov;
  }

  fp->f_offset = off;
  *result = total;
  return 0;
}

static int ramfs_write(vnode_t vp, file_t fp, struct uio *uio, size_t *result)
{
  struct ramfs_node *np;
  off_t file_pos, end_pos;
  void *new_buf;
  size_t new_size;

  *result = 0;
  if (vp->v_type == VDIR)
    return -EISDIR;
  if (vp->v_type != VREG)
    return -EINVAL;

  np = vp->v_data;
  /* Check if the file position exceeds the end of file. */
  end_pos = vp->v_size;
  file_pos = (fp->f_flags & O_APPEND) ? end_pos : fp->f_offset;

  size_t total = 0;
  const struct iovec *iov = uio->iov;
  for (int i = 0; i < uio->iovcnt; ++i) {
    size_t size = iov->iov_len;
    if (file_pos + size > (size_t)end_pos) {
      /* Expand the file size before writing to it */
      end_pos = file_pos + size;
      if (end_pos > (off_t)np->rn_bufsize) {
        /*
         * We allocate the data buffer in page boundary.
         * So that we can reduce the memory allocation unless
         * the file size exceeds next page boundary.
         * This will prevent the memory fragmentation by
         * many malloc/free calls.
         */
        new_size = round_page(end_pos);
        if (vm_allocate(getpid(), &new_buf, new_size, 1))
          return -EIO;
        if (np->rn_size != 0) {
          memcpy(new_buf, np->rn_buf, vp->v_size);
          vm_free(getpid(), np->rn_buf, np->rn_bufsize);
        }
        np->rn_buf = new_buf;
        np->rn_bufsize = new_size;
      }
      np->rn_size = end_pos;
      vn_lock_rw(vp)->v_size = end_pos;
    }
    memcpy(np->rn_buf + file_pos, iov->iov_base, size);
    file_pos += size;
    total += size;
    ++iov;
  }

  fp->f_offset = file_pos;
  *result = total;
  return 0;
}

static int ramfs_rename(vnode_t dvp1, vnode_t vp1, char *name1,
                        vnode_t dvp2, vnode_t vp2, char *name2)
{
  struct ramfs_node *np, *old_np;
  int error;

  if (vp2) {
    /* Remove destination file, first */
    error = ramfs_remove_node(dvp2->v_data, vp2->v_data);
    if (error)
      return error;
  }
  /* Same directory ? */
  if (dvp1 == dvp2) {
    /* Change the name of existing file */
    error = ramfs_rename_node(vp1->v_data, name2);
    if (error)
      return error;
  } else {
    /* Create new file or directory */
    old_np = vp1->v_data;
    np = ramfs_add_node(dvp2->v_data, name2, vp1->v_type, old_np->rn_mode);
    if (np == NULL)
      return -ENOMEM;

    if (vp1->v_type == VREG) {
      /* Copy file data */
      np->rn_buf = old_np->rn_buf;
      np->rn_size = old_np->rn_size;
      np->rn_bufsize = old_np->rn_bufsize;
    }
    /* Remove source file */
    ramfs_remove_node(dvp1->v_data, vp1->v_data);
  }
  return 0;
}

/*
 * @vp: vnode of the directory.
 */
static int ramfs_readdir(vnode_t vp, file_t fp, struct dirent *dir)
{
  struct ramfs_node *np, *dnp;
  int i;

  RAMFS_LOCK();

  if (fp->f_offset == 0) {
    dir->d_type = DT_DIR;
    strlcpy((char *)&dir->d_name, ".", sizeof(dir->d_name));
  } else if (fp->f_offset == 1) {
    dir->d_type = DT_DIR;
    strlcpy((char *)&dir->d_name, "..", sizeof(dir->d_name));
  } else {
    dnp = vp->v_data;
    np = dnp->rn_child;
    if (np == NULL) {
      RAMFS_UNLOCK();
      return -ENOENT;
    }

    for (i = 0; i != (fp->f_offset - 2); i++) {
      np = np->rn_next;
      if (np == NULL) {
        RAMFS_UNLOCK();
        return -ENOENT;
      }
    }
    if (np->rn_type == VDIR)
      dir->d_type = DT_DIR;
    else
      dir->d_type = DT_REG;
    strlcpy((char *)&dir->d_name, np->rn_name,
      sizeof(dir->d_name));
  }
  dir->d_fileno = (uint32_t)fp->f_offset;
  dir->d_namlen = (uint16_t)strlen(dir->d_name);
  dir->d_off = fp->f_offset;

  fp->f_offset++;

  RAMFS_UNLOCK();
  return 0;
}

static int ramfs_init(void)
{
  return 0;
}

ELK_PRECONSTRUCTOR()
{
  vfs_register("ramfs", ramfs_init, &ramfs_vfsops);
}
