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

#ifndef _VFS_H
#define _VFS_H

#include <dirent.h>
#include "vnode.h"
#include "file.h"
#include "mount.h"

#include <assert.h>

// #define DEBUG_VFS 1

/*
 * Tunable parameters
 */
#define FSMAXNAMES	16		/* max length of 'file system' name */

#undef DPRINTF
#ifdef DEBUG_VFS

#include <stdio.h>

#define vfs_debug __elk_vfs_debug
extern int vfs_debug;

#define	VFSDB_CORE	0x00000001      // Unused.
#define	VFSDB_SYSCALL	0x00000002
#define	VFSDB_VNODE	0x00000004
#define	VFSDB_BIO	0x00000008
#define	VFSDB_CAP	0x00000010

#define VFSDB_FLAGS	0x00000013

#define	DPRINTF(_m,X)	if (vfs_debug & (_m)) printf X
#else
#define	DPRINTF(_m, X)
#endif

#define vfssw __elk_vfssw
extern struct vfssw vfssw[];

#define sec_vnode_permission __elk_sec_vnode_permission
#define sec_file_permission __elk_sec_file_permission
#define namei __elk_namei
#define lookup __elk_lookup
#define vnode_init __elk_vnode_init
#define vfs_findroot __elk_vfs_findroot
#define vfs_busy __elk_vfs_busy
#define vfs_unbusy __elk_vfs_unbusy
#define fs_noop __elk_fs_noop

int sec_vnode_permission(char *path);
int sec_file_permission(char *path, int mode);
int namei(char *path, vnode_t *vpp);
int lookup(char *path, vnode_t *vpp, char **name);
void vnode_init(void);
int vfs_findroot(char *path, mount_t *mp, char **root);
void vfs_busy(mount_t mp);
void vfs_unbusy(mount_t mp);
int fs_noop(void);

#endif /* !_VFS_H */
