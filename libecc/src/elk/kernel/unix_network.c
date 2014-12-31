/*
 * Copyright (c) 2014 Richard Pennington.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */


#include <sys/un.h>
#include <errno.h>

#include "config.h"
#include "kernel.h"
#include "vnode.h"
#include "network.h"

// Make AF_UNIX (AF_LOCAL) networking a select-able feature.
FEATURE_CLASS(unix_network, unix_network)

static const struct vnops vnops = {
  net_open,
  net_close,
  net_read,
  net_write,
  net_poll,
  net_seek,
  net_ioctl,
  net_fsync,
  net_readdir,
  net_lookup,
  net_create,
  net_remove,
  net_rename,
  net_mkdir,
  net_rmdir,
  net_getattr,
  net_setattr,
  net_inactive,
  net_truncate,
};

static int setup(vnode_t vp)
{
  struct socket *sp = vp->v_data;
  if (sp->domain != AF_UNIX) {
    // Only AF_UNIX is supported.
    return -EAFNOSUPPORT;
  }
  if (sp->type != SOCK_STREAM) { // RICH: need this && sp->type != SOCK_DGRAM) {
    // RICH: Check this error value.
    return -EOPNOTSUPP;
  }
  if (sp->protocol != 0) {
    return -EPROTONOSUPPORT;
  }

  // An AF_UNIX socket always has a send buffer.
  int s = net_new_buffer(&sp->snd, CONFIG_SO_BUFFER_DEFAULT_PAGES,
                         CONFIG_SO_BUFFER_PAGES);
  return s;
}

static int getopt(file_t fp, int level, int optname, void *optval,
                      socklen_t *optlen)
{
  return -ENOPROTOOPT;
}

static int setopt(file_t fp, int level, int optname, const void *optval,
                      socklen_t optlen)
{
  return -ENOPROTOOPT;
}

static int option_update(file_t fp)
{
  return 0;
}

static int bindaddr(file_t fp, struct sockaddr *addr, socklen_t addrlen)
{
  struct sockaddr_un uaddr;
  if (addrlen > sizeof(uaddr) || addrlen < sizeof(sa_family_t)) {
    return -EINVAL;
  }

  int s = copyin(addr, &uaddr, addrlen);
  if (s != 0)
    return s;

  if (uaddr.sun_family != AF_UNIX) {
    return -EINVAL;
  }

  if (addrlen == sizeof(sa_family_t)) {
    // Unnamed address doesn't make sense here.
    return -EINVAL;
  }

  if (uaddr.sun_path[0]) {
#if RICH
    // This is a file system path.
    // Find the full path name (may be relative to cwd).
    vnode_t vp;
    char path[PATH_MAX];

    s = getpath(uaddr.sun_path, path, 1);
    if (s != 0)
      return s;

    if ((s = namei(path, &vp)) == 0) {
      vput(vp);
      return -EEXIST;
    }

    if ((s = lookup(path, &vp, &name)) != 0)
      return s;

    if ((s = vn_access(vp, VWRITE)) != 0) {
      vput(vp);
      return s;
    }

    if (s = VOP_CREATE(vp, name, mode) != 0) {
      vput(vp);
      return s;
    }
#endif
  } else {
    // This is an abstract address.
    return -EINVAL;     // RICH: For now.
  }

  return -ENOPROTOOPT;
}

static const struct domain_interface interface = {
  .setup = setup,
  .getopt = getopt,
  .setopt = setopt,
  .option_update = option_update,
  .bind = bindaddr,
  .vnops = &vnops,
};

// Register the interface.
C_CONSTRUCTOR()
{
  unix_interface = &interface;
}

