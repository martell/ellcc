/*-
 * Copyright (c) 1989, 1991, 1993
 *  The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *  @(#)mount.h  8.21 (Berkeley) 5/20/95
 */

#ifndef _SYS_MOUNT_H_
#define _SYS_MOUNT_H_

#include "config.h"
#include "list.h"
#include "vnode.h"

#if ELK_NAMESPACE
#define vfssw __elk_vfssw
#define vfs_register __elk_vfs_register
#define vfsop_mount __elk_vfsop_mount
#define vfsop_umount __elk_vfsop_umount
#define vfs_nullop __elk_vfs_nullop
#define vfs_einval __elk_vfs_einval
#endif

typedef struct { int32_t val[2]; } fsid_t;  /* file system id type */


/*
 * file system statistics
 */
struct statfs
{
  short f_type;                 // Filesystem type number.
  short f_flags;                // Copy of mount flags.
  long f_bsize;                 // Fundamental file system block size.
  long f_blocks;                // Total data blocks in file system.
  long f_bfree;                 // Free blocks in fs.
  long f_bavail;                // Free blocks avail to non-superuser.
  long f_files;                 // Total file nodes in file system.
  long f_ffree;                 // Free file nodes in fs.
  fsid_t f_fsid;                // File system id.
  long f_namelen;               // Maximum filename length.
};

/*
 * Mount data
 */
typedef struct mount
{
  struct list m_link;           // Link to next mount point.
  struct vfsops *m_op;          // Pointer to vfs operation.
  int m_flags;                  // Mount flag.
  int m_count;                  // Reference count.
  dev_t m_dev;                  // Mounted device.
  vnode_t m_root;               // Root vnode.
  vnode_t m_covered;            // Vnode covered on parent fs.
  void *m_data;                 // Private data for fs.
  char m_path[];                // Mounted path.
} *mount_t;


/*
 * Mount flags.
 *
 * Unmount uses MNT_FORCE flag.
 */
#define MNT_RDONLY       0x00000001     // Read only filesystem.
#define MNT_SYNCHRONOUS  0x00000002     // File system written synchronously.
#define MNT_NOEXEC       0x00000004     // Can't exec from filesystem.
#define MNT_NOSUID       0x00000008     // Don't honor setuid bits on fs.
#define MNT_NODEV        0x00000010     // Don't interpret special files.
#define MNT_UNION        0x00000020     // Union with underlying filesystem.
#define MNT_ASYNC        0x00000040     // File system written asynchronously.

/*
 * exported mount flags.
 */
#define MNT_EXRDONLY     0x00000080     // Exported read only.
#define MNT_EXPORTED     0x00000100     // File system is exported.
#define MNT_DEFEXPORTED  0x00000200     // Exported to the world.
#define MNT_EXPORTANON   0x00000400     // Use anon uid mapping for everyone.
#define MNT_EXKERB       0x00000800     // Exported with Kerberos uid mapping.

/*
 * Flags set by internal operations.
 */
#define  MNT_LOCAL       0x00001000     // Filesystem is stored locally.
#define  MNT_QUOTA       0x00002000     // Quotas are enabled on filesystem.
#define  MNT_ROOTFS      0x00004000     // Identifies the root filesystem.

/*
 * Mask of flags that are visible to statfs()
 */
#define  MNT_VISFLAGMASK 0x0000ffff

/*
 * Filesystem type switch table.
 */
struct vfssw
{
  const char *vs_name;                  // Name of file system.
  int (*vs_init)(void);                 // Initialize routine.
  struct vfsops *vs_op;                 // Pointer to vfs operation.
};

/*
 * Operations supported on virtual file system.
 */
struct vfsops
{
  int (*vfs_mount)(mount_t, char *, int, void *);
  int (*vfs_unmount)(mount_t);
  int (*vfs_sync)(mount_t);
  int (*vfs_vget)(mount_t, vnode_t);
  int (*vfs_statfs)(mount_t, struct statfs *);
  struct vnops  *vfs_vnops;
};

typedef int (*vfsop_mount_t)(mount_t, char *, int, void *);
typedef int (*vfsop_umount_t)(mount_t);
typedef int (*vfsop_sync_t)(mount_t);
typedef int (*vfsop_vget_t)(mount_t, vnode_t);
typedef int (*vfsop_statfs_t)(mount_t, struct statfs *);

/*
 * VFS interface
 */
#define VFS_MOUNT(MP, DEV, FL, DAT) ((MP)->m_op->vfs_mount)(MP, DEV, FL, DAT)
#define VFS_UNMOUNT(MP)             ((MP)->m_op->vfs_unmount)(MP)
#define VFS_SYNC(MP)                ((MP)->m_op->vfs_sync)(MP)
#define VFS_VGET(MP, VP)            ((MP)->m_op->vfs_vget)(MP, VP)
#define VFS_STATFS(MP, SFP)         ((MP)->m_op->vfs_statfs)(MP, SFP)

#define VFS_NULL        ((void *)vfs_null)

int vfs_register(const char *name, int (*init)(void),
                       struct vfsops *vfsops);
int vfsop_mount(const char *, const char *, const char *, int, void *);
int vfsop_umount(const char *);
int vfs_nullop(void);
int vfs_einval(void);

#endif  /* !_SYS_MOUNT_H_ */
