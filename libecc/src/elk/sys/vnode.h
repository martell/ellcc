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

#ifndef _SYS_VNODE_H_
#define _SYS_VNODE_H_

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pthread.h>
#include "file.h"
#include "list.h"

struct vfsops;
struct vnops;
struct vnode;
struct file;

/*
 * Vnode types.
 */
enum
{
  VNON,                         // No type.
  VREG,                         // Regular file.
  VDIR,                         // Directory.
  VBLK,                         // Block device.
  VCHR,                         // Character device.
  VLNK,                         // Symbolic link.
  VSOCK,                        // Socks.
  VFIFO                         // FIFO.
};

/*
 * Reading or writing any of these items requires holding the
 * appropriate lock.
 */
struct vnode
{
  struct list v_link;           // Link for hash list.
  struct mount *v_mount;        // Mounted vfs pointer.
  struct vnops *v_op;           // Vnode operations.
  int v_refcnt;                 // Reference count.
  int v_type;                   // Vnode type.
  int v_flags;                  // Vnode flag.
  mode_t v_mode;                // File mode.
  size_t v_size;                // File size.
  pthread_mutex_t v_lock;       // Lock for this vnode.
  int v_nrlocks;                // Lock count (for debug).
  int v_blkno;                  // Block number.
  char *v_path;                 // Pointer to path in fs.
  void *v_data;                 // Private data for fs.
};
typedef struct vnode *vnode_t;

/* flags for vnode */
#define VROOT     0x0001        // Root of its file system.
#define VISTTY    0x0002        // Device is tty.
#define VPROTDEV  0x0004        // Protected device.

/*
 * Vnode attribute
 */
struct vattr {
  int va_type;                  // Vnode type.
  mode_t va_mode;               // File access mode.
};

/*
 *  Modes.
 */
#define  VREAD  0x00004         // Read, write, execute permissions.
#define  VWRITE 0x00002
#define  VEXEC  0x00001

/*
 * vnode operations
 */
struct vnops {
  int (*vop_open)(vnode_t, int);
  int (*vop_close)(vnode_t, file_t);
  int (*vop_read)(vnode_t, file_t, void *, size_t, size_t *);
  int (*vop_write)(vnode_t, file_t, void *, size_t, size_t *);
  int (*vop_seek)(vnode_t, file_t, off_t, off_t);
  int (*vop_ioctl)(vnode_t, file_t, u_long, void *);
  int (*vop_fsync)(vnode_t, file_t);
  int (*vop_readdir)(vnode_t, file_t, struct dirent *);
  int (*vop_lookup)(vnode_t, char *, vnode_t);
  int (*vop_create)(vnode_t, char *, mode_t);
  int (*vop_remove)(vnode_t, vnode_t, char *);
  int (*vop_rename)(vnode_t, vnode_t, char *, vnode_t, vnode_t, char *);
  int (*vop_mkdir)(vnode_t, char *, mode_t);
  int (*vop_rmdir)(vnode_t, vnode_t, char *);
  int (*vop_getattr)(vnode_t, struct vattr *);
  int (*vop_setattr)(vnode_t, struct vattr *);
  int (*vop_inactive)(vnode_t);
  int (*vop_truncate)(vnode_t, off_t);
};

typedef  int (*vnop_open_t)(vnode_t, int);
typedef  int (*vnop_close_t)(vnode_t, file_t);
typedef  int (*vnop_read_t)(vnode_t, file_t, void *, size_t, size_t *);
typedef  int (*vnop_write_t)(vnode_t, file_t, void *, size_t, size_t *);
typedef  int (*vnop_seek_t)(vnode_t, file_t, off_t, off_t);
typedef  int (*vnop_ioctl_t)(vnode_t, file_t, u_long, void *);
typedef  int (*vnop_fsync_t)(vnode_t, file_t);
typedef  int (*vnop_readdir_t)(vnode_t, file_t, struct dirent *);
typedef  int (*vnop_lookup_t)(vnode_t, char *, vnode_t);
typedef  int (*vnop_create_t)(vnode_t, char *, mode_t);
typedef  int (*vnop_remove_t)(vnode_t, vnode_t, char *);
typedef  int (*vnop_rename_t)(vnode_t, vnode_t, char *, vnode_t, vnode_t,
                              char *);
typedef  int (*vnop_mkdir_t)(vnode_t, char *, mode_t);
typedef  int (*vnop_rmdir_t)(vnode_t, vnode_t, char *);
typedef  int (*vnop_getattr_t)(vnode_t, struct vattr *);
typedef  int (*vnop_setattr_t)(vnode_t, struct vattr *);
typedef  int (*vnop_inactive_t)(vnode_t);
typedef  int (*vnop_truncate_t)(vnode_t, off_t);

/*
 * vnode interface
 */
#define VOP_OPEN(VP, F) ((VP)->v_op->vop_open)(VP, F)
#define VOP_CLOSE(VP, FP) ((VP)->v_op->vop_close)(VP, FP)
#define VOP_READ(VP, FP, B, S, C) ((VP)->v_op->vop_read)(VP, FP, B, S, C)
#define VOP_WRITE(VP, FP, B, S, C) ((VP)->v_op->vop_write)(VP, FP, B, S, C)
#define VOP_SEEK(VP, FP, OLD, NEW) ((VP)->v_op->vop_seek)(VP, FP, OLD, NEW)
#define VOP_IOCTL(VP, FP, C, A) ((VP)->v_op->vop_ioctl)(VP, FP, C, A)
#define VOP_FSYNC(VP, FP) ((VP)->v_op->vop_fsync)(VP, FP)
#define VOP_READDIR(VP, FP, DIR) ((VP)->v_op->vop_readdir)(VP, FP, DIR)
#define VOP_LOOKUP(DVP, N, VP) ((DVP)->v_op->vop_lookup)(DVP, N, VP)
#define VOP_CREATE(DVP, N, M) ((DVP)->v_op->vop_create)(DVP, N, M)
#define VOP_REMOVE(DVP, VP, N) ((DVP)->v_op->vop_remove)(DVP, VP, N)
#define VOP_RENAME(DVP1, VP1, N1, DVP2, VP2, N2) \
         ((DVP1)->v_op->vop_rename)(DVP1, VP1, N1, DVP2, VP2, N2)
#define VOP_MKDIR(DVP, N, M) ((DVP)->v_op->vop_mkdir)(DVP, N, M)
#define VOP_RMDIR(DVP, VP, N) ((DVP)->v_op->vop_rmdir)(DVP, VP, N)
#define VOP_GETATTR(VP, VAP) ((VP)->v_op->vop_getattr)(VP, VAP)
#define VOP_SETATTR(VP, VAP) ((VP)->v_op->vop_setattr)(VP, VAP)
#define VOP_INACTIVE(VP) ((VP)->v_op->vop_inactive)(VP)
#define VOP_TRUNCATE(VP, N) ((VP)->v_op->vop_truncate)(VP, N)

int __elk_vop_nullop(void);
int __elk_vop_einval(void);
vnode_t __elk_vn_lookup(struct mount *, char *);
void __elk_vn_lock(vnode_t);
void __elk_vn_unlock(vnode_t);
int __elk_vn_stat(vnode_t, struct stat *);
int __elk_vn_access(vnode_t, int);
vnode_t __elk_vget(struct mount *, char *);
void __elk_vput(vnode_t);
void __elk_vgone(vnode_t);
void __elk_vref(vnode_t);
void __elk_vrele(vnode_t);
int __elk_vcount(vnode_t);
void __elk_vflush(struct mount *);

#define vop_nullop __elk_vop_nullop
#define vop_einval __elk_vop_einval
#define vn_lookup __elk_vn_lookup
#define vn_lock __elk_vn_lock
#define vn_unlock __elk_vn_unlock
#define vn_stat __elk_vn_stat
#define vn_access __elk_vn_access
#define vget __elk_vget
#define vput __elk_vput
#define vgone __elk_vgone
#define vref __elk_vref
#define vrele __elk_vrele
#define vcount __elk_vcount
#define vflush __elk_vflush

#endif /* !_SYS_VNODE_H_ */
