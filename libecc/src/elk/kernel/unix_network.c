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
#include <sys/ioctl.h>
#include <errno.h>

#include "config.h"
#include "kernel.h"
#include "vnode.h"
#include "thread.h"
#include "vm.h"
#include "network.h"

// Make AF_UNIX (AF_LOCAL) networking a select-able feature.
FEATURE_CLASS(unix_network, unix_network)

static int setup(vnode_t vp, int flags)
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

  return 0;
}

static int getopt(file_t fp, int level, int optname, void *optval,
                      socklen_t *optlen)
{
  if (level != SOL_SOCKET) {
    return -EINVAL;
  }

  // There ar no AF_UNIX socket options that aren't already handled.
  return -ENOPROTOOPT;
}

static int setopt(file_t fp, int level, int optname, const void *optval,
                      socklen_t optlen)
{
  if (level != SOL_SOCKET) {
    return -EINVAL;
  }

  // There ar no AF_UNIX socket options that aren't already handled.
  return -ENOPROTOOPT;
}

static int option_update(file_t fp)
{
  return 0;
}

static int unix_ioctl(file_t fp, u_long request, void *buf)
{
  struct socket *sp = fp->f_vnode->v_data;
  switch (request) {
  case FIONREAD: {      // Return the number of bytes in the read buffer.
    if (sp->flags & SF_ACCEPTCONN) {
      return -EINVAL;
    }

    int size;
    if (sp->rcv == NULL) {
      size = 0;
    } else {
      size = sp->rcv->used;
    }

    return copyout(&size, buf, sizeof(int));
  }
  default:
    return -EINVAL;
  }
}

static int unix_bind(file_t fp, struct sockaddr *addr, socklen_t addrlen)
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
    // This is a file system path.
    // Find the full path name (may be relative to cwd).
    vnode_t dvp;
    char path[PATH_MAX];

    s = getpath(uaddr.sun_path, path, 1);
    if (s != 0)
      return s;

    if ((s = namei(path, &dvp)) == 0) {
      vput(dvp);
      return -EEXIST;
    }

    char *name;
    if ((s = lookup(path, &dvp, &name)) != 0)
      return s;

    if ((s = vn_access(dvp, VWRITE)) != 0) {
      vput(dvp);
      return s;
    }

    // RICH: Creation mode? umask?
    int mode = S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH;
    if ((s = VOP_CREATE(dvp, name, S_IFSOCK|mode)) != 0) {
      vput(dvp);
      return s;
    }

    // Bind the socket to the path.
    s = vbind(dvp->v_mount, fp->f_vnode, path);
    vput(dvp);
    return s;
  } else {
    // This is an abstract address.
    return -EINVAL;     // RICH: For now.
  }

  return -ENOPROTOOPT;
}

static int unix_listen(struct socket *sp, int backlog)
{
  // RICH: TODO
  return -EINVAL;
}

static int unix_connect(struct socket *sp, const struct sockaddr *addr,
                        socklen_t addrlenlen)
{
  // RICH: TODO
  return -EINVAL;
}

static int unix_accept4(struct socket *sp, struct socket *newsp,
                        struct sockaddr *addr, socklen_t *addrlen, int flags)
{
  // RICH: TODO
  return -EINVAL;
}

static int unix_getpeername(struct socket *sp, struct sockaddr *addr,
                            socklen_t *addrlen)
{
  // RICH: TODO
  return -EINVAL;
}

static int unix_getsockname(struct socket *sp, struct sockaddr *addr, 
                            socklen_t *addrlen)
{
  // RICH: TODO
  return -EINVAL;
}

static int unix_shutdown(struct socket *sp, int how)
{
  // RICH: TODO
  return -EINVAL;
}

static int unix_close(file_t fp)
{
  struct socket *sp = fp->f_vnode->v_data;
  net_release_buffer(sp->snd);
  net_release_buffer(sp->rcv);
  return 0;
}

static const struct domain_interface interface = {
  .setup = setup,
  .getopt = getopt,
  .setopt = setopt,
  .option_update = option_update,
  .ioctl = unix_ioctl,
  .bind = unix_bind,
  .listen = unix_listen,
  .connect = unix_connect,
  .accept4 = unix_accept4,
  .sendto = net_buffer_send,
  .recvfrom = net_buffer_recv,
  .getpeername = unix_getpeername,
  .getsockname = unix_getsockname,
  .shutdown = unix_shutdown,
  .close = unix_close,
  .vnops = &net_vnops,
};

// Register the interface.
ELK_PRECONSTRUCTOR()
{
  unix_interface = &interface;
}

