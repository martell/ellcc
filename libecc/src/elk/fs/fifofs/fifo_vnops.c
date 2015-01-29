/*
 * Copyright (c) 2008, Kohsuke Ohtani
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
 * fifofs - FIFO/pipe file system.
 */

#include <sys/stat.h>
#include <sys/syslog.h>

#include <dirent.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <fcntl.h>

#include "uio.h"
#include "list.h"
#include "kmem.h"
#include "vnode.h"
#include "file.h"
#include "mount.h"

FEATURE(fifofs)

struct fifo_node {
  struct list fn_link;
  int fn_type;                  // Type: VFIFO or VPIPE.
  char *fn_name;                // Name (null-terminated).
  pthread_cond_t fn_rcond;      // cv for read.
  pthread_cond_t fn_wcond;      // cv for write.
  pthread_mutex_t fn_rmtx;      // Mutex for read.
  pthread_mutex_t fn_wmtx;      // Mutex for write.
  int fn_readers;               // Reader count.
  int fn_writers;               // Writer count.
  int fn_start;                 // Start offset of buffer data.
  int fn_size;                  // Size of buffer data.
  char *fn_buf;                 // Pointer to buffer.
};

#define fifo_mount  ((vfsop_mount_t)vfs_nullop)
#define fifo_unmount  ((vfsop_umount_t)vfs_nullop)
#define fifo_sync  ((vfsop_sync_t)vfs_nullop)
#define fifo_vget  ((vfsop_vget_t)vfs_nullop)
#define fifo_statfs  ((vfsop_statfs_t)vfs_nullop)

static int fifo_open  (vnode_t, int);
static int fifo_close  (vnode_t, file_t);
static int fifo_read  (vnode_t, file_t, struct uio *, size_t *);
static int fifo_write  (vnode_t, file_t, struct uio *, size_t *);
#define fifo_poll ((vnop_poll_t)vop_nullop)
#define fifo_seek  ((vnop_seek_t)vop_nullop)
static int fifo_ioctl  (vnode_t, file_t, u_long, void *);
#define fifo_fsync  ((vnop_fsync_t)vop_nullop)
static int fifo_readdir  (vnode_t, file_t, struct dirent *);
static int fifo_lookup  (vnode_t, char *, vnode_t);
static int fifo_create  (vnode_t, char *, mode_t);
static int fifo_remove  (vnode_t, vnode_t, char *);
#define fifo_rename  ((vnop_rename_t)vop_einval)
#define fifo_mkdir  ((vnop_mkdir_t)vop_einval)
#define fifo_rmdir  ((vnop_rmdir_t)vop_einval)
#define fifo_getattr  ((vnop_getattr_t)vop_nullop)
#define fifo_setattr  ((vnop_setattr_t)vop_nullop)
#define fifo_inactive  ((vnop_inactive_t)vop_nullop)
#define fifo_truncate  ((vnop_truncate_t)vop_nullop)

static void cleanup_fifo(vnode_t);
static void wait_reader(vnode_t);
static void wakeup_reader(vnode_t);
static void wait_writer(vnode_t);
static void wakeup_writer(vnode_t);

static pthread_mutex_t fifo_lock = PTHREAD_MUTEX_INITIALIZER;

static struct list fifo_head;

/*
 * vnode operations
 */
static struct vnops fifofs_vnops =
{
  fifo_open,
  fifo_close,
  fifo_read,
  fifo_write,
  fifo_poll,
  fifo_seek,
  fifo_ioctl,
  fifo_fsync,
  fifo_readdir,
  fifo_lookup,
  fifo_create,
  fifo_remove,
  fifo_rename,
  fifo_mkdir,
  fifo_rmdir,
  fifo_getattr,
  fifo_setattr,
  fifo_inactive,
  fifo_truncate,
};

/*
 * File system operations
 */
static struct vfsops fifofs_vfsops =
{
  fifo_mount,    /* mount */
  fifo_unmount,    /* unmount */
  fifo_sync,    /* sync */
  fifo_vget,    /* vget */
  fifo_statfs,    /* statfs */
  &fifofs_vnops,    /* vnops */
};

static int fifo_open(vnode_t vp, int flags)
{
  struct fifo_node *np = vp->v_data;

  DPRINTF(AFSDB_CORE, ("fifo_open: path=%s\n", vp->v_path));

  if (!strcmp(vp->v_path, "/"))  /* root ? */
    return 0;

  /*
   * Unblock all threads who are waiting in open().
   */
  if (flags & FREAD) {
    if (np->fn_readers == 0 && np->fn_writers > 0)
      wakeup_writer(vp);
    np->fn_readers++;
  }

  if (flags & FWRITE) {
    if (np->fn_writers == 0 && np->fn_readers > 0)
      wakeup_reader(vp);
    np->fn_writers++;
  }

  /*
   * If no-one opens FIFO at the other side, wait for open().
   */
  if (flags & FREAD) {
    if (flags & O_NONBLOCK) {
    } else {
      while (np->fn_writers == 0)
        wait_writer(vp);
    }
  }

  if (flags & FWRITE) {
    if (flags & O_NONBLOCK) {
      if (np->fn_readers == 0)
        return -ENXIO;
    } else {
      while (np->fn_readers == 0)
        wait_reader(vp);
    }
  }

  return 0;
}

static int fifo_close(vnode_t vp, file_t fp)
{
  struct fifo_node *np = vp->v_data;

  DPRINTF(AFSDB_CORE, ("fifo_close: fp=%p\n", fp));

  if (np == NULL)
    return 0;

  if (fp->f_flags & FREAD) {
    np->fn_readers--;
    if (np->fn_readers == 0)
      wakeup_writer(vp);
  }

  if (fp->f_flags & FWRITE) {
    np->fn_writers--;
    if (np->fn_writers == 0)
      wakeup_reader(vp);
  }

  if (vp->v_refcnt > 1)
    return 0;

  /* Clearn up pipe */
  if (!strncmp(np->fn_name, "pipe", 4)) {
    DPRINTF(AFSDB_CORE, ("fifo_close: remove pipe\n"));
    cleanup_fifo(vp);
  }

  return 0;
}

static int fifo_read(vnode_t vp, file_t fp, struct uio *uio, size_t *result)
{
  struct fifo_node *np = vp->v_data;

  DPRINTF(AFSDB_CORE, ("fifo_read\n"));

  /*
   * If nothing in the pipe, wait.
   */
  while (np->fn_size == 0) {
    // No data and no writer, then EOF
    if (np->fn_writers == 0) {
      *result = 0;
      return 0;
    }

    // wait for data
    wait_writer(vp);
  }

  // Read
  size_t total = 0;
  size_t pos = np->fn_start;
  const struct iovec *iov = uio->iov;
  for (int i = 0; i < uio->iovcnt; ++i) {
    size_t size = iov->iov_len;
    size_t nbytes;
    nbytes = (np->fn_size < size) ? np->fn_size : size;
    char *p = iov->iov_base;
    while (nbytes > 0) {
      *p++ = np->fn_buf[pos];
      if (++pos > PIPE_BUF)
        pos = 0;
      nbytes--;
    }

    total += size;
    ++iov;
  }

  np->fn_start = pos;
  *result = total;
  wakeup_writer(vp);
  return 0;
}

static int fifo_write(vnode_t vp, file_t fp, struct uio *uio, size_t *result)
{
  struct fifo_node *np = vp->v_data;

  DPRINTF(AFSDB_CORE, ("fifo_write\n"));

  size_t total = 0;
  const struct iovec *iov = uio->iov;
  for (int i = 0; i < uio->iovcnt; ++i) {
    size_t size = iov->iov_len;
    char *p = iov->iov_base;

  again:
    /*
     * If the pipe is full,
     * wait for reads to deplete
     * and truncate it.
     */
    while (np->fn_size >= PIPE_BUF)
      wait_reader(vp);

    // Write
    size_t nfree = PIPE_BUF - np->fn_size;
    size_t nbytes = (nfree < size) ? nfree : size;

    size_t pos = np->fn_start + np->fn_size;
    if (pos >= PIPE_BUF)
      pos -= PIPE_BUF;

    np->fn_size += nbytes;
    size -= nbytes;
    while (nbytes > 0) {
      np->fn_buf[pos] = *p++;
      if (++pos > PIPE_BUF)
        pos = 0;
      nbytes--;
      total++;
    }

    wakeup_reader(vp);

    if (size > 0)
      goto again;       // More to do.
  }

  *result = total;
  return 0;
}

static int fifo_ioctl(vnode_t vp, file_t fp, u_long cmd, void *arg)
{
  DPRINTF(AFSDB_CORE, ("fifo_ioctl\n"));
  return -EINVAL;
}

static int fifo_lookup(vnode_t dvp, char *name, vnode_t vp_ro)
{
  list_t head, n;
  struct fifo_node *np = NULL;
  int found;

  DPRINTF(AFSDB_CORE, ("fifo_lookup: %s\n", name));

  if (*name == '\0')
    return -ENOENT;

  pthread_mutex_lock(&fifo_lock);

  found = 0;
  head = &fifo_head;
  for (n = list_first(head); n != head; n = list_next(n)) {
    np = list_entry(n, struct fifo_node, fn_link);
    if (strcmp(name, np->fn_name) == 0) {
      found = 1;
      break;
    }
  }
  if (found == 0) {
    pthread_mutex_unlock(&fifo_lock);
    return -ENOENT;
  }

  vnode_rw_t vp = vn_lock_rw(vp_ro);
  vp->v_data = np;
  vp->v_mode = ALLPERMS;
  vp->v_size = 0;
  vp->v_type = VFIFO;
  pthread_mutex_unlock(&fifo_lock);
  return 0;
}

static int fifo_create(vnode_t dvp, char *name, mode_t mode)
{
  struct fifo_node *np;
  size_t len;

  DPRINTF(AFSDB_CORE, ("create %s in %s\n", name, dvp->v_path));

#if 0
  if (!S_ISFIFO(mode))
    return -EINVAL;
#endif

  if ((np = kmem_alloc(sizeof(struct fifo_node))) == NULL)
    return -ENOMEM;

  if ((np->fn_buf = kmem_alloc(PIPE_BUF)) == NULL) {
    kmem_free(np);
    return -ENOMEM;
  }
  len = strlen(name) + 1;
  np->fn_name = kmem_alloc(len);
  if (np->fn_name == NULL) {
    kmem_free(np->fn_buf);
    kmem_free(np);
    return -ENOMEM;
  }

  strlcpy(np->fn_name, name, len);
  pthread_mutex_init(&np->fn_rmtx, NULL);
  pthread_mutex_init(&np->fn_wmtx, NULL);
  pthread_cond_init(&np->fn_rcond, NULL);
  pthread_cond_init(&np->fn_wcond, NULL);
  np->fn_readers = 0;
  np->fn_writers = 0;
  np->fn_start = 0;
  np->fn_size = 0;

  pthread_mutex_lock(&fifo_lock);
  list_insert(&fifo_head, &np->fn_link);
  pthread_mutex_unlock(&fifo_lock);
  return 0;
}

static void cleanup_fifo(vnode_t vp)
{
  struct fifo_node *np = vp->v_data;

  pthread_mutex_lock(&fifo_lock);
  list_remove(&np->fn_link);
  pthread_mutex_unlock(&fifo_lock);

  kmem_free(np->fn_name);
  kmem_free(np->fn_buf);
  kmem_free(np);

  vn_lock_rw(vp)->v_data = NULL;
}

static int fifo_remove(vnode_t dvp, vnode_t vp, char *name)
{
  DPRINTF(AFSDB_CORE, ("remove %s in %s\n", name, dvp->v_path));

  cleanup_fifo(vp);
  return 0;
}

/*
 * @vp: vnode of the directory.
 */
static int fifo_readdir(vnode_t vp, file_t fp, struct dirent *dir)
{
  struct fifo_node *np;
  list_t head, n;
  int i;

  pthread_mutex_lock(&fifo_lock);

  if (fp->f_offset == 0) {
    dir->d_type = DT_DIR;
    strlcpy((char *)&dir->d_name, ".", sizeof(dir->d_name));
  } else if (fp->f_offset == 1) {
    dir->d_type = DT_DIR;
    strlcpy((char *)&dir->d_name, "..", sizeof(dir->d_name));
  } else {
    i = 0;
    np = NULL;
    head = &fifo_head;
    for (n = list_first(head); n != head; n = list_next(n)) {
      if (i == (fp->f_offset - 2)) {
        np = list_entry(n, struct fifo_node, fn_link);
        break;
      }
    }
    if (np == NULL) {
      pthread_mutex_unlock(&fifo_lock);
      return -ENOENT;
    }
    dir->d_type = DT_FIFO;
    strlcpy((char *)&dir->d_name, np->fn_name,
      sizeof(dir->d_name));
  }
  dir->d_fileno = fp->f_offset;
  dir->d_namlen = (uint16_t)strlen(dir->d_name);

  fp->f_offset++;

  pthread_mutex_unlock(&fifo_lock);
  return 0;
}

static int fifofs_init(void)
{
  list_init(&fifo_head);
  return 0;
}


static void wait_reader(vnode_t vp)
{
  struct fifo_node *np = vp->v_data;

  DPRINTF(AFSDB_CORE, ("wait_reader: %p\n", np));
  vn_unlock(vp);
  pthread_mutex_lock(&np->fn_rmtx);
  pthread_cond_wait(&np->fn_rcond, &np->fn_rmtx);
  pthread_mutex_unlock(&np->fn_rmtx);
  vn_lock(vp, LK_SHARED|LK_RETRY);
}

static void wakeup_writer(vnode_t vp)
{
  struct fifo_node *np = vp->v_data;

  DPRINTF(AFSDB_CORE, ("wakeup_writer: %p\n", np));
  pthread_cond_broadcast(&np->fn_rcond);
}

static void wait_writer(vnode_t vp)
{
  struct fifo_node *np = vp->v_data;

  DPRINTF(AFSDB_CORE, ("wait_writer: %p\n", np));
  vn_unlock(vp);
  pthread_mutex_lock(&np->fn_wmtx);
  pthread_cond_wait(&np->fn_wcond, &np->fn_wmtx);
  pthread_mutex_unlock(&np->fn_wmtx);
  vn_lock(vp, LK_SHARED|LK_RETRY);
}

static void wakeup_reader(vnode_t vp)
{
  struct fifo_node *np = vp->v_data;

  DPRINTF(AFSDB_CORE, ("wakeup_reader: %p\n", np));
  pthread_cond_broadcast(&np->fn_wcond);
}

ELK_PRECONSTRUCTOR()
{
  vfs_register("fifofs", fifofs_init, &fifofs_vfsops);
}
