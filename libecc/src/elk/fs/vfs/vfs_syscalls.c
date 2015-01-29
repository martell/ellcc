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
 * vfs_syscalls.c - everything in this file is a routine implementing
 *                  a VFS system call.
 */

#include <sys/stat.h>
#include <sys/uio.h>
#include <dirent.h>

#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>

#include "syscalls.h"
#include "kmem.h"
#include "vnode.h"
#include "file.h"
#include "mount.h"
#include "list.h"
#include "buf.h"
#include "thread.h"
#include "crt1.h"
#include "vfs.h"

FEATURE(vfs_syscalls)

/** The vnode is locked after this call.
 * fp->f_vnode must be vn_unlocked or vput after this call.
 */
static int vfs_open(char *name, int flags, mode_t mode, file_t *fpp)
{
  file_t fp;
  vnode_t vp, dvp;
  char *filename;
  int error;

  // Find the full path name (may be relative to cwd).
  char path[PATH_MAX];
  if ((error = getpath(name, path, 1)) != 0)
    return error;

  DPRINTF(VFSDB_SYSCALL, ("sys_open: path=%s flags=%o mode=%x\n",
                          path, flags, mode));

  // Check to see whether the file as accessible.
  int acc = 0;
  switch (flags & O_ACCMODE) {
  case O_RDONLY:
    acc = VREAD;
    break;
  case O_WRONLY:
    acc = VWRITE;
    break;
  case O_RDWR:
    acc = VREAD | VWRITE;
    break;
  }

  if ((error = sec_file_permission(path, acc))) {
    return error;
  }

  flags = FFLAGS(flags);
  if  ((flags & (FREAD | FWRITE)) == 0)
    return -EINVAL;
  if (flags & O_CREAT) {
    error = namei(path, &vp);
    if (error == -ENOENT) {
      /* Create new file. */
      if ((error = lookup(path, &dvp, &filename)) != 0)
        return error;
      if ((error = vn_access(dvp, VWRITE)) != 0) {
        vput(dvp);
        return error;
      }
      mode &= ~S_IFMT;
      mode |= S_IFREG;
      error = VOP_CREATE(dvp, filename, mode);
      vput(dvp);
      if (error)
        return error;
      if ((error = namei(path, &vp)) != 0)
        return error;
      flags &= ~O_TRUNC;
    } else if (error) {
      return error;
    } else {
      /* File already exits */
      if (flags & O_EXCL) {
        vput(vp);
        return -EEXIST;
      }
      flags &= ~O_CREAT;
    }
  } else {
    /* Open */
    if ((error = namei(path, &vp)) != 0)
      return error;
  }
  if ((flags & O_CREAT) == 0) {
    if (flags & FWRITE || flags & O_TRUNC) {
      if ((error = vn_access(vp, VWRITE)) != 0) {
        vput(vp);
        return error;
      }
      if (vp->v_type == VDIR) {
        /* Openning directory with writable. */
        vput(vp);
        return -EISDIR;
      }
    }
  }
  /* Process truncate request */
  if (flags & O_TRUNC) {
    if (!(flags & FWRITE) || (vp->v_type == VDIR)) {
      vput(vp);
      return -EINVAL;
    }
    if ((error = VOP_TRUNCATE(vp, 0)) != 0) {
      vput(vp);
      return error;
    }
  }

  /* Setup file structure */
  if (!(fp = kmem_alloc(sizeof(struct file)))) {
    vput(vp);
    return -ENOMEM;
  }

  /* Request to file system */
  if ((error = VOP_OPEN(vp, flags)) != 0) {
    kmem_free(fp);
    vput(vp);
    return error;
  }

  memset(fp, 0, sizeof(struct file));
  fp->f_vnode = vp;
  fp->f_flags = flags;
  fp->f_offset = 0;
  fp->f_count = 1;
  *fpp = fp;
  return 0;
}

static int sys_open(char *name, int flags, mode_t mode)
{
  file_t fp;
  int error;

  if ((error = vfs_open(name, flags, mode, &fp)) != 0) {
    return error;
  }

  int fd = allocfd(fp);         // Get a file descriptor.
  if (fd < 0) {
    vput(fp->f_vnode);
    kmem_free(fp);
    return fd;
  }

  vn_unlock(fp->f_vnode);
  return fd;
}

static int sys_creat(char *name, mode_t mode)
{
  return sys_open(name, O_CREAT|O_WRONLY|O_TRUNC, mode);
}

int vfs_close(file_t fp)
{
  vnode_t vp;

  ASSERT(fp->f_count > 0);
  if (fp->f_count <= 0)
    panic("vfs_close");

  vp = fp->f_vnode;
  if (--fp->f_count > 0) {
    vrele(vp);
    return 0;
  }

  vn_lock(vp, LK_SHARED|LK_RETRY);
  int error;
  if ((error = VOP_CLOSE(vp, fp)) != 0) {
    vn_unlock(vp);
    return error;
  }

  vput(vp);
  kmem_free(fp);
  return 0;
}

static int sys_close(int fd)
{
  int error;
  file_t fp;

  error = getfile(fd, &fp);
  if (error < 0) {
    return error;
  }

  DPRINTF(VFSDB_SYSCALL, ("sys_close: fd=%d fp=%p count=%d\n",
                          fd, fp, fp->f_count));

  error = vfs_close(fp);
  setfile(fd, NULL);
  return error;
}

static ssize_t sys_read(int fd, void *buf, size_t size)
{
  vnode_t vp;
  int error;
  file_t fp;

  error = getfile(fd, &fp);
  if (error < 0) {
    return error;
  }

#if RICH
  DPRINTF(VFSDB_SYSCALL, ("sys_read: fd=%d fp=%p buf=%p size=%zd\n",
                          fd, fp, buf, size));
#endif

  if ((fp->f_flags & FREAD) == 0)
    return -EBADF;

  if (size == 0) {
    return 0;
  }

  vp = fp->f_vnode;
  vn_lock(vp, LK_SHARED|LK_RETRY);
  size_t count;
  struct iovec iovec = { .iov_base = buf, .iov_len = size };
  struct uio uio = { .iovcnt = 1, .iov = &iovec };
  error = VOP_READ(vp, fp, &uio, &count);
  vn_unlock(vp);
  if (error) {
    return error;
  }

  return count;
}

static ssize_t sys_readv(int fd, struct iovec *iov, int iovcnt)
{
  vnode_t vp;
  int error;
  file_t fp;

  error = getfile(fd, &fp);
  if (error < 0) {
    return error;
  }

#if RICH
  DPRINTF(VFSDB_SYSCALL, ("sys_read: fd=%d fp=%p iov->iov_base=%p "
                          "iov->iov_len=%zd\n",
                          fd, fp, iov->iov_base, iov->iov_len));
#endif

  if ((fp->f_flags & FREAD) == 0)
    return -EBADF;

  vp = fp->f_vnode;
  vn_lock(vp, LK_SHARED|LK_RETRY);
  size_t count;
  struct uio uio = { .iovcnt = iovcnt, .iov = iov };
  error = VOP_READ(vp, fp, &uio, &count);
  vn_unlock(vp);
  if (error) {
    return error;
  }

  return count;
}

static ssize_t sys_write(int fd, void *buf, size_t size)
{
  vnode_t vp;
  int error;
  file_t fp;

  error = getfile(fd, &fp);
  if (error < 0) {
    return error;
  }

#if RICH
  DPRINTF(VFSDB_SYSCALL, ("sys_write: fd=%d fp=%p buf=%p size=%zd\n",
                          fd, fp, buf, size));
#endif

  if ((fp->f_flags & FWRITE) == 0)
    return -EBADF;
  if (size == 0) {
    return 0;
  }
  vp = fp->f_vnode;
  vn_lock(vp, LK_SHARED|LK_RETRY);
  size_t count;
  struct iovec iovec = { .iov_base = buf, .iov_len = size };
  struct uio uio = { .iovcnt = 1, .iov = &iovec };
  error = VOP_WRITE(vp, fp, &uio, &count);
  vn_unlock(vp);
  if (error) {
    return error;
  }

  return count;
}

static ssize_t sys_writev(int fd, struct iovec *iov, int iovcnt)
{
  vnode_t vp;
  int error;
  file_t fp;

  error = getfile(fd, &fp);
  if (error < 0) {
    return error;
  }

#if RICH
  DPRINTF(VFSDB_SYSCALL, ("sys_write: fd=%d fp=%p iov->iov_base=%p "
                          "iov->iov_len=%zd\n",
                          fd, fp, iov->iov_base, iov->iov_len));
#endif

  if ((fp->f_flags & FWRITE) == 0)
    return -EBADF;
  vp = fp->f_vnode;
  vn_lock(vp, LK_SHARED|LK_RETRY);
  size_t count;
  struct uio uio = { .iovcnt = iovcnt, .iov = iov };
  error = VOP_WRITE(vp, fp, &uio, &count);
  vn_unlock(vp);
  if (error) {
    return error;
  }

  return count;
}

static off_t sys_lseek(int fd, off_t off, int type)
{
  vnode_t vp;
  int error;
  file_t fp;

  error = getfile(fd, &fp);
  if (error < 0) {
    return error;
  }

  DPRINTF(VFSDB_SYSCALL, ("sys_seek: fd=%d fp=%p off=%lld type=%d\n",
                          fd, fp, (long long)off, type));

  vp = fp->f_vnode;
  vn_lock(vp, LK_SHARED|LK_RETRY);
  switch (type) {
  case SEEK_SET:
    if (off < 0)
      off = 0;
    if (off > (off_t)vp->v_size)
      off = vp->v_size;
    break;
  case SEEK_CUR:
    if (fp->f_offset + off > (off_t)vp->v_size)
      off = vp->v_size;
    else if (fp->f_offset + off < 0)
      off = 0;
    else
      off = fp->f_offset + off;
    break;
  case SEEK_END:
    if (off > 0)
      off = vp->v_size;
    else if ((int)vp->v_size + off < 0)
      off = 0;
    else
      off = vp->v_size + off;
    break;
  default:
    vn_unlock(vp);
    return -EINVAL;
  }

  /* Request to check the file offset */
  error = VOP_SEEK(vp, fp, fp->f_offset, off);
  if (error != 0) {
    vn_unlock(vp);
    return error;
  }

  fp->f_offset = off;
  vn_unlock(vp);
  return off;
}

static int sys_ioctl(int fd, u_long request, void *buf)
{
  vnode_t vp;
  int error;
  file_t fp;

  error = getfile(fd, &fp);
  if (error < 0) {
    return error;
  }

  DPRINTF(VFSDB_SYSCALL, ("sys_ioctl: fd=%d fp=%p request=%lx\n",
                          fd, fp, request));

  if ((fp->f_flags & (FREAD | FWRITE)) == 0)
    return -EBADF;

  vp = fp->f_vnode;
  vn_lock(vp, LK_SHARED|LK_RETRY);
  error = VOP_IOCTL(vp, fp, request, buf);
  vn_unlock(vp);
  DPRINTF(VFSDB_SYSCALL, ("sys_ioctl: comp error=%d\n", error));
  return error;
}

static int sys_fsync(int fd)
{
  vnode_t vp;
  int error;
  file_t fp;

  error = getfile(fd, &fp);
  if (error < 0) {
    return error;
  }

  DPRINTF(VFSDB_SYSCALL, ("sys_fsync: fd=%d fp=%p\n", fd, fp));

  if ((fp->f_flags & FWRITE) == 0)
    return -EBADF;

  vp = fp->f_vnode;
  vn_lock(vp, LK_SHARED|LK_RETRY);
  error = VOP_FSYNC(vp, fp);
  vn_unlock(vp);
  return error;
}

static int sys_fstat(int fd, struct stat *st)
{
  vnode_t vp;
  int error = 0;
  file_t fp;

  error = getfile(fd, &fp);
  if (error < 0) {
    return error;
  }

  DPRINTF(VFSDB_SYSCALL, ("sys_fstat: fd=%d fp=%p\n", fd, fp));

  vp = fp->f_vnode;
  vn_lock(vp, LK_SHARED|LK_RETRY);
  error = vn_stat(vp, st);
  vn_unlock(vp);
  return error;
}

static int i_readdir(file_t fp, struct dirent *dir)
{
  vnode_t dvp;
  int error;

  DPRINTF(VFSDB_SYSCALL, ("i_readdir: fp=%p dirent=%p\n", fp, dir));

  dvp = fp->f_vnode;
  vn_lock(dvp, LK_SHARED|LK_RETRY);
  if (dvp->v_type != VDIR) {
    vn_unlock(dvp);
    return -EBADF;
  }

  error = VOP_READDIR(dvp, fp, dir);
  DPRINTF(VFSDB_SYSCALL, ("i_readdir: error=%d path=%s\n",
                          error, dir->d_name));
  vn_unlock(dvp);
  return error;
}

static int sys_getdents(int fd, char *buf, size_t len)
{
  file_t fp;
  int error;

  if (len < sizeof(struct dirent))
    return -EINVAL;

  if ((error = getfile(fd, &fp)) < 0) {
    return error;
  }

  size_t size = 0;
  while (size + sizeof(struct dirent) < len) {
    struct dirent dir;
    error = i_readdir(fp, &dir);
    if (error) {
      if (error == -ENOENT && size) {
        return size;
      }
      return error;
    }

    // dir.d_reclen is the length of the unterminated string.
    // Translate it to the total size.
    dir.d_reclen = offsetof(struct dirent, d_name)
                   + dir.d_reclen + 1;  // The string.

    DPRINTF(VFSDB_SYSCALL,
            ("dir: d_ino=%llu d_off=%llu d_reclen=%d"
             " d_type=%d d_name=%s\n",
             (long long)dir.d_ino, (long long)dir.d_off,
             dir.d_reclen,
             dir.d_type,
             dir.d_name));

    size_t reclen = dir.d_reclen;       // Total record length.
    // Round up to the next 32 byte boundry. RICH: ?
    dir.d_reclen += (32 - (dir.d_reclen & 0x1F));
    memcpy(buf, &dir, reclen);
    ASSERT((dir.d_reclen & 0x1F) == 0);
    size += dir.d_reclen;
    buf += dir.d_reclen;
  }

  return size;
}

/*
 * Return 0 if directory is empty
 */
static int check_dir_empty(char *path)
{
  int fd;
  file_t fp;
  struct dirent dir;

  DPRINTF(VFSDB_SYSCALL, ("check_dir_empty\n"));

  if ((fd = sys_open(path, O_RDONLY, 0)) != 0)
    return fd;

  int error = getfile(fd, &fp);
  if (error < 0) {
    sys_close(fd);
    return error;
  }

  vnode_t dvp = fp->f_vnode;
  vn_lock(dvp, LK_SHARED|LK_RETRY);
  if (dvp->v_type != VDIR) {
    vn_unlock(dvp);
    sys_close(fd);
    return -ENOTDIR;
  }

  vn_unlock(dvp);

  do {
    error = i_readdir(fp, &dir);
    if (error != 0 && error != -EACCES)
      break;
  } while (!strcmp(dir.d_name, ".") || !strcmp(dir.d_name, ".."));

  sys_close(fd);

  if (error == -ENOENT)
    return 0;
  else if (error == 0)
    return -EEXIST;

  return error;
}

static int sys_mkdir(char *name, mode_t mode)
{
  vnode_t vp, dvp;
  int error;

  // Find the full path name (may be relative to cwd).
  char path[PATH_MAX];
  if ((error = getpath(name, path, 1)) != 0)
    return error;

  DPRINTF(VFSDB_SYSCALL, ("sys_mkdir: path=%s mode=%d\n", path, mode));

  if ((error = namei(path, &vp)) == 0) {
    /* File already exists */
    vput(vp);
    return -EEXIST;
  }

  /* Notice: vp is invalid here! */

  if ((error = lookup(path, &dvp, &name)) != 0) {
    /* Directory already exists */
    return error;
  }
  if ((error = vn_access(dvp, VWRITE)) != 0)
    goto out;
  mode &= ~S_IFMT;
  mode |= S_IFDIR;

  error = VOP_MKDIR(dvp, name, mode);
 out:
  vput(dvp);
  return error;
}

static int sys_rmdir(char *name)
{
  vnode_t vp, dvp;
  int error;

  // Find the full path name (may be relative to cwd).
  char path[PATH_MAX];
  if ((error = getpath(name, path, 1)) != 0)
    return error;

  DPRINTF(VFSDB_SYSCALL, ("sys_rmdir: path=%s\n", path));

  if ((error = check_dir_empty(path)) != 0)
    return error;
  if ((error = namei(path, &vp)) != 0)
    return error;
  if ((error = vn_access(vp, VWRITE)) != 0)
    goto out;
  if (vp->v_type != VDIR) {
    error = -ENOTDIR;
    goto out;
  }

  if (vp->v_flags & VROOT || vcount(vp) >= 2) {
    error = -EBUSY;
    goto out;
  }

  if ((error = lookup(path, &dvp, &name)) != 0)
    goto out;

  error = VOP_RMDIR(dvp, vp, name);
  vn_unlock(vp);
  vgone(vp);
  vput(dvp);
  return error;

 out:
  vput(vp);
  return error;
}

static int sys_mknod(char *name, mode_t mode)
{
  vnode_t vp, dvp;
  int error;

  // Find the full path name (may be relative to cwd).
  char path[PATH_MAX];
  if ((error = getpath(name, path, 1)) != 0)
    return error;

  DPRINTF(VFSDB_SYSCALL, ("sys_mknod: path=%s mode=%d\n",  path, mode));

  switch (mode & S_IFMT) {
  case S_IFREG:
  case S_IFDIR:
  case S_IFIFO:
  case S_IFSOCK:
    /* OK */
    break;
  default:
    return -EINVAL;
  }

  if ((error = namei(path, &vp)) == 0) {
    vput(vp);
    return -EEXIST;
  }

  if ((error = lookup(path, &dvp, &name)) != 0)
    return error;

  if ((error = vn_access(dvp, VWRITE)) != 0)
    goto out;

  if (S_ISDIR(mode))
    error = VOP_MKDIR(dvp, name, mode);
  else
    error = VOP_CREATE(dvp, name, mode);

 out:
  vput(dvp);
  return error;
}

static int sys_rename(char *srcname, char *destname)
{
  vnode_t vp1, vp2 = 0, dvp1, dvp2;
  char *sname, *dname;
  int error;
  size_t len;

  // Find the full path names (may be relative to cwd).
  char src[PATH_MAX];
  char dest[PATH_MAX];
  if ((error = getpath(srcname, src, 1)) != 0)
    return error;
  if ((error = getpath(destname, dest, 1)) != 0)
    return error;

  DPRINTF(VFSDB_SYSCALL, ("sys_rename: src=%s dest=%s\n", src, dest));

  if ((error = namei(src, &vp1)) != 0)
    return error;
  if ((error = vn_access(vp1, VWRITE)) != 0)
    goto err1;

  /* If source and dest are the same, do nothing */
  if (!strncmp(src, dest, PATH_MAX))
    goto err1;

  /* Check if target is directory of source */
  len = strlen(dest);
  if (!strncmp(src, dest, len)) {
    error = -EINVAL;
    goto err1;
  }
  /* Is the source busy ? */
  if (vcount(vp1) >= 2) {
    error = -EBUSY;
    goto err1;
  }
  /* Check type of source & target */
  error = namei(dest, &vp2);
  if (error == 0) {
    /* target exists */
    if (vp1->v_type == VDIR && vp2->v_type != VDIR) {
      error = -ENOTDIR;
      goto err2;
    } else if (vp1->v_type != VDIR && vp2->v_type == VDIR) {
      error = -EISDIR;
      goto err2;
    }
    if (vp2->v_type == VDIR && check_dir_empty(dest)) {
      error = -EEXIST;
      goto err2;
    }

    if (vcount(vp2) >= 2) {
      error = -EBUSY;
      goto err2;
    }
  }

  dname = strrchr(dest, '/');
  if (dname == NULL) {
    error = -ENOTDIR;
    goto err2;
  }

  if (dname == dest)
    strcpy(dest, "/");

  *dname = 0;
  dname++;

  if ((error = lookup(src, &dvp1, &sname)) != 0)
    goto err2;

  if ((error = namei(dest, &dvp2)) != 0)
    goto err3;

  /* The source and dest must be same file system */
  if (dvp1->v_mount != dvp2->v_mount) {
    error = -EXDEV;
    goto err4;
  }
  error = VOP_RENAME(dvp1, vp1, sname, dvp2, vp2, dname);
 err4:
  vput(dvp2);
 err3:
  vput(dvp1);
 err2:
  if (vp2)
    vput(vp2);
 err1:
  vput(vp1);
  return error;
}

static int sys_getcwd(char *buf, unsigned long size)
{
  int error;
  char path[PATH_MAX];
  if ((error = getpath("", path, 0)) != 0)
    return error;

  size_t len = strlen(path) + 1;
  if (len >= size) {
    return -ERANGE;
  }

  return copyout(path, buf, len);
}

static int sys_chdir(char *name)
{
  file_t fp;
  int error;

  if ((error = vfs_open(name, O_RDONLY, 0, &fp)) != 0) {
    return error;
  }

  vnode_t dvp;
  dvp = fp->f_vnode;
  if (dvp->v_type != VDIR) {
    vn_unlock(dvp);
    vfs_close(fp);
    return -ENOTDIR;
  }

  replacecwd(dvp);                      // Replace the current directory.
  vn_unlock(dvp);
  vfs_close(fp);

  return 0;
}

static int sys_fchdir(int fd)
{
  file_t fp;
  int error;

  if (!capable(CAP_SYS_CHROOT)) {
    return -EPERM;
  }

  if ((error = getfile(fd, &fp)) < 0) {
    return error;
  }

  vnode_t dvp;
  dvp = fp->f_vnode;
  vn_lock(dvp, LK_SHARED|LK_RETRY);
  if (dvp->v_type != VDIR) {
    vn_unlock(dvp);
    return -ENOTDIR;
  }

  replacecwd(dvp);                      // Replace the current directory.
  vn_unlock(dvp);

  return 0;
}

static int sys_chroot(char *name)
{
  file_t fp;
  int error;

  if ((error = vfs_open(name, O_RDONLY, 0, &fp)) != 0) {
    return error;
  }

  vnode_t dvp;
  dvp = fp->f_vnode;
  if (dvp->v_type != VDIR) {
    vn_unlock(dvp);
    vfs_close(fp);
    return -ENOTDIR;
  }

  replaceroot(dvp);                     // Replace the root directory.
  vn_unlock(dvp);
  vfs_close(fp);

  return 0;
}

static int sys_link(char *oldname, char *newname)
{
  // RICH: Implement links.
  return -EPERM;
}

static int sys_unlink(char *name)
{
  vnode_t vp, dvp;
  int error;
  char path[PATH_MAX];
  if ((error = getpath(name, path, 1)) != 0)
    return error;

  DPRINTF(VFSDB_SYSCALL, ("sys_unlink: path=%s\n", path));

  if ((error = namei(path, &vp)) != 0)
    return error;
  if ((error = vn_access(vp, VWRITE)) != 0)
    goto out;
  if (vp->v_type == VDIR) {
    error = -EPERM;
    goto out;
  }
  /* XXX: Need to allow unlink for opened file. */
  if (vp->v_flags & VROOT || vcount(vp) >= 2) {
    error = -EBUSY;
    goto out;
  }
  if ((error = lookup(path, &dvp, &name)) != 0)
    goto out;

  error = VOP_REMOVE(dvp, vp, name);

  vn_unlock(vp);
  vgone(vp);
  vput(dvp);
  return 0;
 out:
  vput(vp);
  return error;
}

static int sys_access(char *path, int mode)
{
  vnode_t vp;
  int error, flags;

  DPRINTF(VFSDB_SYSCALL, ("sys_access: path=%s mode=%x\n", path, mode));

  /* If F_OK is set, we return here if file is not found. */
  if ((error = namei(path, &vp)) != 0)
    return error;

  flags = 0;
  if (mode & R_OK)
    flags |= VREAD;
  if (mode & W_OK)
    flags |= VWRITE;
  if (mode & X_OK)
    flags |= VEXEC;

  error = vn_access(vp, flags);

  vput(vp);
  return error;
}

static int sys_stat(char *name, struct stat *st)
{
  vnode_t vp;
  int error;
  char path[PATH_MAX];
  if ((error = getpath(name, path, 1)) != 0)
    return error;

  DPRINTF(VFSDB_SYSCALL, ("sys_stat: path=%s\n", path));

  if ((error = namei(path, &vp)) != 0)
    return error;
  error = vn_stat(vp, st);
  vput(vp);
  return error;
}

// RICH: This should handle symbolic links.
static int sys_lstat(char *name, struct stat *st)
{
  vnode_t vp;
  int error;
  char path[PATH_MAX];
  if ((error = getpath(name, path, 1)) != 0)
    return error;

  DPRINTF(VFSDB_SYSCALL, ("sys_lstat: path=%s\n", path));

  if ((error = namei(path, &vp)) != 0)
    return error;
  error = vn_stat(vp, st);
  vput(vp);
  return error;
}

static int sys_truncate(char *path, off_t length)
{
  return 0;
}

static int sys_ftruncate(int fd, off_t length)
{
  return 0;
}

static int sys_dup(int oldfd)
{
  file_t fp;
  int newfd;
  int error;

  if ((error = getfile(oldfd, &fp)) < 0) {
    return error;
  }

  if ((newfd = allocfd(fp)) < 0) {
    return newfd;
  }

  // Increment file references.
  vref(fp->f_vnode);
  ++fp->f_count;

  return newfd;
}

static int sys_dup2(int oldfd, int newfd)
{
  file_t fp, ofp;
  int error;

  if ((error = getfile(oldfd, &fp)) < 0) {
    return error;
  }

  if (oldfd == newfd) {
    return newfd;
  }

  if ((error = getdup(newfd, &ofp, 0)) < 0) {
    return error;
  }

  // Increment file references.
  vref(fp->f_vnode);
  ++fp->f_count;

  if (ofp) {
    // Close the old file, if any.
    vfs_close(ofp);
  }

  setfile(newfd, fp);

  return newfd;
}

static int sys_fcntl(int fd, int cmd, int arg)
{
  file_t fp;
  int s;

  if ((s = getfile(fd, &fp)) < 0) {
    return s;
  }

  switch (cmd) {
  case F_DUPFD: {
    file_t ofp;
    if ((s = getdup(arg, &ofp, 1)) < 0) {
      return s;
    }

    // Increment file references.
    vref(fp->f_vnode);
    ++fp->f_count;

    if (ofp) {
      // Close the old file, if any.
      vfs_close(ofp);
    }

    setfile(arg, fp);

    s = arg;
    break;
  }
  case F_GETFD:
    s = fp->f_flags & FD_CLOEXEC;
    break;
  case F_SETFD:
    fp->f_flags = (fp->f_flags & ~FD_CLOEXEC) | (arg & FD_CLOEXEC);
    s = 0;
    break;
  case F_GETFL:
  case F_SETFL:
    s = -EINVAL;
    break;
  default:
    s = -EINVAL;
    break;
  }

  return s;
}

ELK_PRECONSTRUCTOR()
{
  SYSCALL(access);
  SYSCALL(chdir);
  SYSCALL(chroot);
  SYSCALL(close);
  SYSCALL(creat);
  SYSCALL(dup);
  SYSCALL(dup2);
  SYSCALL(fchdir);
  SYSCALL(fcntl);
  SYSCALL(fstat);
  SYSCALL(fsync);
  SYSCALL(ftruncate);
  SYSCALL(getcwd);
  SYSCALL(getdents);
  SYSCALL(ioctl);
  SYSCALL(link);
  SYSCALL(lseek);
  SYSCALL(lstat);
  SYSCALL(mkdir);
  SYSCALL(mknod);
  SYSCALL(open);
  SYSCALL(read);
  SYSCALL(readv);
  SYSCALL(rmdir);
  SYSCALL(rename);
  SYSCALL(stat);
  SYSCALL(truncate);
  SYSCALL(unlink);
  SYSCALL(write);
  SYSCALL(writev);
}
