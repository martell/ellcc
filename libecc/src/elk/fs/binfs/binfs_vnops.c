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
 * binfs - executable binary file system.
 */

#include <sys/stat.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <fcntl.h>

#include "mount.h"
#include "vnode.h"
#include "file.h"

FEATURE(binfs)

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static struct binary *binary_list = NULL;
typedef struct binary {
  struct binary *next;
  char *name;
  int (*main)(int argc, char **argv);
} binary_t;

#define binfs_mount ((vfsop_mount_t)vfs_nullop)
#define binfs_unmount ((vfsop_umount_t)vfs_nullop)
#define binfs_sync ((vfsop_sync_t)vfs_nullop)
#define binfs_vget ((vfsop_vget_t)vfs_nullop)
#define binfs_statfs ((vfsop_statfs_t)vfs_nullop)

static int binfs_open(vnode_t, int);
static int binfs_close(vnode_t, file_t);
#define binfs_read  ((vnop_read_t)vop_nullop)
#define binfs_write  ((vnop_write_t)vop_nullop)
#define binfs_poll  ((vnop_poll_t)vop_nullop)
#define binfs_seek  ((vnop_seek_t)vop_nullop)
static int binfs_ioctl  (vnode_t, file_t, u_long, void *);
#define binfs_fsync  ((vnop_fsync_t)vop_nullop)
#define binfs_fsync  ((vnop_fsync_t)vop_nullop)
static int binfs_readdir(vnode_t, file_t, struct dirent *);
static int binfs_lookup(vnode_t, char *, vnode_t);
#define binfs_create  ((vnop_create_t)vop_einval)
#define binfs_remove  ((vnop_remove_t)vop_einval)
#define binfs_rename  ((vnop_rename_t)vop_einval)
#define binfs_mkdir  ((vnop_mkdir_t)vop_einval)
#define binfs_rmdir  ((vnop_rmdir_t)vop_einval)
#define binfs_getattr  ((vnop_getattr_t)vop_nullop)
#define binfs_setattr  ((vnop_setattr_t)vop_nullop)
#define binfs_inactive  ((vnop_inactive_t)vop_nullop)
#define binfs_truncate  ((vnop_truncate_t)vop_nullop)

/*
 * vnode operations
 */
static struct vnops binfs_vnops = {
  binfs_open,
  binfs_close,
  binfs_read,
  binfs_write,
  binfs_poll,
  binfs_seek,
  binfs_ioctl,
  binfs_fsync,
  binfs_readdir,
  binfs_lookup,
  binfs_create,
  binfs_remove,
  binfs_rename,
  binfs_mkdir,
  binfs_rmdir,
  binfs_getattr,
  binfs_setattr,
  binfs_inactive,
  binfs_truncate,
};

/*
 * File system operations
 */
static struct vfsops binfs_vfsops = {
  binfs_mount,
  binfs_unmount,
  binfs_sync,
  binfs_vget,
  binfs_statfs,
  &binfs_vnops,
};

static int binfs_open(vnode_t vp, int flags)
{
  char *path;
  device_t dev;
  int error;

  DPRINTF(AFSDB_CORE, ("binfs_open: path=%s\n", vp->v_path));

  path = vp->v_path;
  if (strcmp(path, "/") == 0)   /* root ? */
    return 0;

  if (vp->v_flags & VPROTDEV) {
    DPRINTF(AFSDB_CORE, ("binfs_open: failed to open protected device.\n"));
    return -EPERM;
  }

  if (*path == '/')
    path++;
  error = device_open(path, flags & DO_RWMASK, &dev);
  if (error) {
    DPRINTF(AFSDB_CORE, ("binfs_open: can not open device = %s error=%d\n",
       path, error));
    return error;
  }
  vp->v_data = (void *)dev;  /* Store private data */
  return 0;
}

static int binfs_close(vnode_t vp, file_t fp)
{

  DPRINTF(AFSDB_CORE, ("binfs_close: fp=%p\n", fp));

  if (!strcmp(vp->v_path, "/"))  /* root ? */
    return 0;

  return device_close((device_t)vp->v_data);
}

static int binfs_ioctl(vnode_t vp, file_t fp, u_long cmd, void *arg)
{
  int error;

  error = device_ioctl((device_t)vp->v_data, cmd, arg);
  DPRINTF(AFSDB_CORE, ("binfs_ioctl: cmd=%lx\n", cmd));
  return error;
}

static int binfs_lookup(vnode_t dvp, char *name, vnode_t vp)
{
  struct devinfo info;
  int error, i;

  DPRINTF(AFSDB_CORE, ("binfs_lookup:%s\n", name));

  if (*name == '\0')
    return -ENOENT;

  i = 0;
  error = 0;
  info.cookie = 0;
  for (;;) {
    error = device_info(&info);
    if (error)
      return -ENOENT;
    if (!strncmp(info.name, name, CONFIG_MAXDEVNAME))
      break;
    i++;
  }
  vp->v_type = (info.flags & D_CHR) ? VCHR : VBLK;
  if (info.flags & D_TTY)
    vp->v_flags |= VISTTY;

  if (info.flags & D_PROT)
    vp->v_flags |= VPROTDEV;
  else
    vp->v_mode = (mode_t)(S_IRUSR | S_IWUSR);
  return 0;
}

/*
 * @vp: vnode of the directory.
 */
static int binfs_readdir(vnode_t vp, file_t fp, struct dirent *dir)
{
  struct devinfo info;
  int error, i;

  DPRINTF(AFSDB_CORE, ("binfs_readdir offset=%lld\n", (long long)fp->f_offset));

  i = 0;
  error = 0;
  info.cookie = 0;
  do {
    error = device_info(&info);
    if (error)
      return -ENOENT;
  } while (i++ != fp->f_offset);

  dir->d_type = 0;
  if (info.flags & D_CHR)
    dir->d_type = DT_CHR;
  else if (info.flags & D_BLK)
    dir->d_type = DT_BLK;
  strlcpy((char *)&dir->d_name, info.name, sizeof(dir->d_name));
  dir->d_fileno = (uint32_t)fp->f_offset;
  dir->d_namlen = (uint16_t)strlen(dir->d_name);

  DPRINTF(AFSDB_CORE, ("binfs_readdir: %s\n", dir->d_name));
  fp->f_offset++;
  return 0;
}

static int binfs_init(void)
{
  return 0;
}

ELK_PRECONSTRUCTOR()
{
  vfs_register("binfs", binfs_init, &binfs_vfsops);
}
