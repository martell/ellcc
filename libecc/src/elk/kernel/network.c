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

#define _GNU_SOURCE
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

#include "config.h"
#include "kernel.h"
#include "syscalls.h"
#include "crt1.h"
#include "thread.h"
#include "network.h"

// Make networking a select-able feature.
FEATURE(network)

const struct domain_interface *unix_interface;
const struct domain_interface *inet_interface;
const struct domain_interface *ipx_interface;
const struct domain_interface *netlink_interface;
const struct domain_interface *x25_interface;
const struct domain_interface *ax25_interface;
const struct domain_interface *atmpvc_interface;
const struct domain_interface *appletalk_interface;
const struct domain_interface *packet_interface;

/** Default vnode operations for sockets.
 */
int net_open(vnode_t vp, int flags)
{
  return -EINVAL;
}

int net_close(vnode_t vp, file_t fp)
{
  return -EINVAL;
}

int net_read(vnode_t vp, file_t fp, struct uio *uio, size_t *count)
{
  return -EINVAL;
}

int net_write(vnode_t vp, file_t fp, struct uio *uio, size_t *count)
{
  return -EINVAL;
}

int net_poll(vnode_t vp, file_t fp, int what)
{
  return -EINVAL;
}

int net_seek(vnode_t vp, file_t fp, off_t foffset, off_t offset)
{
  return -EINVAL;
}

int net_ioctl(vnode_t vp, file_t fp, u_long request, void *buf)
{
  return -EINVAL;
}

int net_fsync(vnode_t vp, file_t fp)
{
  return -EINVAL;
}

int net_readdir(vnode_t vp, file_t fp, struct dirent *dir)
{
  return -EINVAL;
}

int net_lookup(vnode_t dvp, char *name, vnode_t vp)
{
  return -EINVAL;
}

int net_create(vnode_t vp, char *name, mode_t mode)
{
  return -EINVAL;
}

int net_remove(vnode_t dvp, vnode_t vp, char *name)
{
  return -EINVAL;
}

int net_rename(vnode_t dvp1, vnode_t vp1, char *sname,
               vnode_t dvp2, vnode_t vp2, char *dname)
{
  return -EINVAL;
}

int net_mkdir(vnode_t vp, char *name, mode_t mode)
{
  return -EINVAL;
}

int net_rmdir(vnode_t dvp, vnode_t vp, char *name)
{
  return -EINVAL;
}

int net_getattr(vnode_t vp, struct vattr *vattr)
{
  return -EINVAL;
}

int net_setattr(vnode_t vp, struct vattr *vattr)
{
  return -EINVAL;
}

int net_inactive(vnode_t vp)
{
  return -EINVAL;
}

int net_truncate(vnode_t vp, off_t offset)
{
  return -EINVAL;
}

/** Socket related system calls.
 */
static int sys_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
  return -ENOSYS;
}

static int sys_accept4(int sockfd, struct sockaddr *addr, socklen_t *addrlen,
                       int flags)
{
  return -ENOSYS;
}

static int sys_bind(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
  return -ENOSYS;
}

static int sys_connect(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
  return -ENOSYS;
}

static int sys_getpeername(int sockfd, struct sockaddr *addr,
                           socklen_t *addrlen)
{
  return -ENOSYS;
}

static int sys_getsockname(int sockfd, struct sockaddr *addr,
                           socklen_t *addrlen)
{
  return -ENOSYS;
}

/** Generic socket system call entry.
 * on exit, fp is the file pointer, sp is the socket pointer, s is zero.
 */
#define SOCKET_ENTER()                                          \
  int s;                                                        \
  file_t fp;                                                    \
  s = getfile(sockfd, &fp);                                     \
  if (s < 0) {                                                  \
    return s;                                                   \
  }                                                             \
  vnode_t vp = fp->f_vnode;                                     \
  vn_lock(vp, LK_SHARED|LK_RETRY);                              \
  if (vp->v_type != VSOCK) {                                    \
    vn_unlock(vp);                                              \
    return -ENOTSOCK;                                           \
  }                                                             \
  struct socket *sp = vp->v_data;

static int sys_getsockopt(int sockfd, int level, int optname, void *optval,
                          socklen_t *optlen)
{
  SOCKET_ENTER()

  if (level != SOL_SOCKET) {
    // Try a lower level get.
    s = sp->interface->getopt(fp, level, optname, optval, optlen);
  } else {
    // Handle SOL_SOCKET level requests.
    switch (optname) {
#define COPYOUTINT(expr)                                        \
      {                                                         \
      socklen_t len;                                            \
      s = copyin(optlen, &len, sizeof(socklen_t));              \
      if (s != 0) {                                             \
        vn_unlock(vp);                                          \
        return s;                                               \
      }                                                         \
      if (len < sizeof(int)) {                                  \
        vn_unlock(vp);                                          \
        return -EINVAL;                                         \
      }                                                         \
      len = sizeof(int);                                        \
      int value = (expr);                                       \
      s = copyout(&value, optval, sizeof(int));                 \
      if (s != 0) {                                             \
        vn_unlock(vp);                                          \
        return s;                                               \
      }                                                         \
      s = copyout(&len, optlen, sizeof(socklen_t));             \
      if (s != 0) {                                             \
        vn_unlock(vp);                                          \
        return s;                                               \
      }                                                         \
      }

#define COPYOUTTYPE(type, addr)                                 \
      {                                                         \
      socklen_t len;                                            \
      s = copyin(optlen, &len, sizeof(socklen_t));              \
      if (s != 0) {                                             \
        vn_unlock(vp);                                          \
        return s;                                               \
      }                                                         \
      if (len < sizeof(type)) {                                 \
        vn_unlock(vp);                                          \
        return -EINVAL;                                         \
      }                                                         \
      len = sizeof(type);                                       \
      s = copyout(addr, optval, sizeof(type));                  \
      if (s != 0) {                                             \
        vn_unlock(vp);                                          \
        return s;                                               \
      }                                                         \
      s = copyout(&len, optlen, sizeof(socklen_t));             \
      if (s != 0) {                                             \
        vn_unlock(vp);                                          \
        return s;                                               \
      }                                                         \
      }

    case SO_ACCEPTCONN:
      COPYOUTINT((sp->flags & SF_ACCEPTCONN) != 0);
      break;

    case SO_BINDTODEVICE:
      s = -ENOPROTOOPT;
      break;

    case SO_BROADCAST:
      COPYOUTINT((sp->flags & SF_BROADCAST) != 0);
      break;

    case SO_BSDCOMPAT:
      COPYOUTINT((sp->flags & SF_BSDCOMPAT) != 0);
      break;

    case SO_DEBUG:
      COPYOUTINT((sp->flags & SF_DEBUG) != 0);
      break;

    case SO_DOMAIN:
      COPYOUTINT(sp->domain);
      break;

    case SO_ERROR:
      COPYOUTINT(sp->error);
      break;

    case SO_DONTROUTE:
      COPYOUTINT((sp->flags & SF_DONTROUTE) != 0);
      break;

    case SO_KEEPALIVE:
      COPYOUTINT((sp->flags & SF_KEEPALIVE) != 0);
      break;

    case SO_LINGER: {
      socklen_t len;
      s = copyin(optlen, &len, sizeof(socklen_t));
      if (s != 0) {
        vn_unlock(vp);
        return s;
      }
      if (len < sizeof(struct linger)) {
        vn_unlock(vp);
        return -EINVAL;
      }
      len = sizeof(struct linger);
      struct linger linger = { .l_onoff = (sp->flags & SF_LINGER) != 0,
                               .l_linger = sp->linger };
      s = copyout(&linger, optval, sizeof(struct linger));
      if (s != 0) {
        vn_unlock(vp);
        return s;
      }
      s = copyout(&len, optlen, sizeof(socklen_t));
      if (s != 0) {
        vn_unlock(vp);
        return s;
      }
      break;
    }

    case SO_MARK:
      COPYOUTINT(sp->mark);
      break;

    case SO_OOBINLINE:
      COPYOUTINT((sp->flags & SF_OOBINLINE) != 0);
      break;

    case SO_PASSCRED:
      COPYOUTINT((sp->flags & SF_PASSCRED) != 0);
      break;

    case SO_PEEK_OFF:
      COPYOUTINT(sp->peek_off);
      break;

    case SO_PEERCRED:
      COPYOUTTYPE(struct timeval, &sp->ucred);
      break;

    case SO_PRIORITY:
      COPYOUTINT(sp->priority);
      break;

    case SO_PROTOCOL:
      COPYOUTINT(sp->protocol);
      break;

    case SO_RCVBUF:
    case SO_RCVBUFFORCE:
      COPYOUTINT(sp->rcvbuf);
      break;

    case SO_RCVLOWAT:
      COPYOUTINT(sp->rcvlowait);
      break;

    case SO_SNDLOWAT:
      COPYOUTINT(sp->sndlowait);
      break;

    case SO_RCVTIMEO:
      COPYOUTTYPE(struct timeval, &sp->rcvtimeo);
      break;

    case SO_SNDTIMEO:
      COPYOUTTYPE(struct timeval, &sp->sndtimeo);
      break;

    case SO_REUSEADDR:
      COPYOUTINT((sp->flags & SF_REUSEADDR) != 0);
      break;

    case SO_RXQ_OVFL:
      COPYOUTINT((sp->flags & SF_RXQ_OVFL) != 0);
      break;

    case SO_SNDBUF:
    case SO_SNDBUFFORCE:
      COPYOUTINT(sp->sndbuf);
      break;

    case SO_TIMESTAMP:
      COPYOUTINT((sp->flags & SF_TIMESTAMP) != 0);
      break;

    case SO_TYPE:
      COPYOUTINT(sp->type);
      break;

    case SO_BUSY_POLL:
      COPYOUTINT(sp->busy_poll);
      break;

    default:
      s = -ENOPROTOOPT;
      break;
    }

#undef COPYOUTINT
#undef COPYOUTTYPE
  }

  vn_unlock(vp);
  return s;
}

static int sys_listen(int sockfd, int backlog)
{
  SOCKET_ENTER()
  return -ENOSYS;
}

#if defined(SYS_socketcall) || defined(SYS_recv)
static int sys_recv(int sockfd, void *buf, size_t len, int flags)
{
  SOCKET_ENTER()
  return -ENOSYS;
}
#endif

static int sys_recvfrom(int sockfd, void *buf, size_t len, int flags,
                        struct sockaddr *src_addr, socklen_t *addrlen)
{
  SOCKET_ENTER()
  return -ENOSYS;
}

static int sys_recvmmsg(int sockfd, struct msghdr *msgvec, unsigned int vlen,
                        unsigned int flags, struct timespec *timeout)
{
  SOCKET_ENTER()
  return -ENOSYS;
}

static int sys_recvmsg(int sockfd, struct msghdr *msgvec, unsigned int flags)
{
  SOCKET_ENTER()
  return -ENOSYS;
}

#if defined(SYS_socketcall) || defined(SYS_send)
static int sys_send(int sockfd, const void *buf, size_t len, int flags)
{
  SOCKET_ENTER()
  return -ENOSYS;
}
#endif

static int sys_sendto(int sockfd, const void *buf, size_t len, int flags,
                        struct sockaddr *src_addr, socklen_t *addrlen)
{
  SOCKET_ENTER()
  return -ENOSYS;
}

static int sys_sendmmsg(int sockfd, const struct msghdr *msgvec,
                        unsigned int vlen, unsigned int flags)
{
  SOCKET_ENTER()
  return -ENOSYS;
}

static int sys_sendmsg(int sockfd, const struct msghdr *msgvec,
                       unsigned int flags)
{
  SOCKET_ENTER()
  return -ENOSYS;
}

static int sys_setsockopt(int sockfd, int level, int optname,
                          const void *optval, socklen_t optlen)
{
  SOCKET_ENTER()
  if (level != SOL_SOCKET) {
    // Try a lower level get.
    s = sp->interface->setopt(fp, level, optname, optval, optlen);
  } else {
    // Handle SOL_SOCKET level requests.
    switch (optname) {
#define COPYININT(where)                                        \
      {                                                         \
      if (optlen < sizeof(int)) {                               \
        vn_unlock(vp);                                          \
        return -EINVAL;                                         \
      }                                                         \
      s = copyin(optval, &where, sizeof(int));                  \
      if (s != 0) {                                             \
        vn_unlock(vp);                                          \
        return s;                                               \
      }                                                         \
      }

#define COPYINFLAG(flag)                                        \
      {                                                         \
      if (optlen < sizeof(int)) {                               \
        vn_unlock(vp);                                          \
        return -EINVAL;                                         \
      }                                                         \
      int value;                                                \
      s = copyin(optval, &value, sizeof(int));                  \
      if (s != 0) {                                             \
        vn_unlock(vp);                                          \
        return s;                                               \
      }                                                         \
      if (value)                                                \
        sp->flags |= (flag);                                    \
      else                                                      \
        sp->flags &= ~(flag);                                   \
      }

#define COPYINTYPE(type, where)                                 \
      {                                                         \
      if (optlen < sizeof(type)) {                              \
        vn_unlock(vp);                                          \
        return -EINVAL;                                         \
      }                                                         \
      s = copyin(optval, &where, sizeof(type));                 \
      if (s != 0) {                                             \
        vn_unlock(vp);                                          \
        return s;                                               \
      }                                                         \
      }

    case SO_ACCEPTCONN:
    case SO_DOMAIN:
    case SO_ERROR:
    case SO_PEERCRED:
    case SO_PROTOCOL:
    case SO_TYPE:
      s = -EINVAL;                      // Read only.
      break;

    case SO_BINDTODEVICE:
      s = -ENOPROTOOPT;
      break;

    case SO_BROADCAST:
      COPYINFLAG(SF_BROADCAST);
      break;

    case SO_BSDCOMPAT:
      COPYINFLAG(SF_BSDCOMPAT);
      break;

    case SO_DEBUG:
      if (capable(CAP_NET_ADMIN)) {
        COPYINFLAG(SF_DEBUG);
      } else {
        s = -EPERM;
      }
      break;

    case SO_DONTROUTE:
      COPYINFLAG(SF_DONTROUTE);
      break;

    case SO_KEEPALIVE:
      COPYINFLAG(SF_KEEPALIVE);
      break;

    case SO_LINGER: {
      if (optlen < sizeof(struct linger)) {
        vn_unlock(vp);
        return -EINVAL;
      }
      struct linger linger;
      s = copyin(optval, &linger, sizeof(struct linger));
      if (s != 0) {
        vn_unlock(vp);
        return s;
      }
      if (linger.l_onoff) {
        sp->flags |= SF_LINGER;
      } else {
        sp->flags &= ~SF_LINGER;
      }
      sp->linger = linger.l_linger;
      break;
    }

    case SO_MARK:
      if (capable(CAP_NET_ADMIN)) {
        COPYININT(sp->mark);
      } else {
        s = -EPERM;
      }
      break;

    case SO_OOBINLINE:
      COPYINFLAG(SF_OOBINLINE);
      break;

    case SO_PASSCRED:
      COPYINFLAG(SF_PASSCRED);
      break;

    case SO_PEEK_OFF:
      COPYININT(sp->peek_off);
      break;

    case SO_PRIORITY: {
      int temp;
      COPYININT(temp);
      if ((temp < 0 || temp > 6) && !capable(CAP_NET_ADMIN)) {
        s = -EPERM;
      } else {
        sp->priority = temp;
      }
      break;
    }

    case SO_RCVBUF:
    case SO_RCVBUFFORCE: {
      int rcvbuf;
      COPYININT(rcvbuf);
      rcvbuf *= 2;      // RICH: May not need this.
      if (rcvbuf < CONFIG_SO_RCVBUF_MIN ||
          ((optname == SO_RCVBUF || !capable(CAP_NET_ADMIN)) &&
           rcvbuf > CONFIG_SO_RCVBUF_MAX)) {
        s = -EINVAL;
      } else {
        sp->rcvbuf = rcvbuf;
      }
      break;
    }

    case SO_RCVLOWAT:
      COPYININT(sp->rcvlowait);
      break;

    case SO_SNDLOWAT:
      COPYININT(sp->sndlowait);
      break;

    case SO_RCVTIMEO:
      COPYINTYPE(struct timeval, sp->rcvtimeo);
      break;

    case SO_SNDTIMEO:
      COPYINTYPE(struct timeval, sp->sndtimeo);
      break;

    case SO_REUSEADDR:
      COPYINFLAG(SF_REUSEADDR);
      break;

    case SO_RXQ_OVFL:
      COPYINFLAG(SF_RXQ_OVFL);
      break;

    case SO_SNDBUF:
    case SO_SNDBUFFORCE: {
      int sndbuf;
      COPYININT(sndbuf);
      sndbuf *= 2;      // RICH: May not need this.
      if (sndbuf < CONFIG_SO_SNDBUF_MIN ||
          ((optname == SO_SNDBUF || !capable(CAP_NET_ADMIN)) &&
           sndbuf > CONFIG_SO_SNDBUF_MAX)) {
        s = -EINVAL;
      } else {
        sp->sndbuf = sndbuf;
      }
      break;
    }

    case SO_TIMESTAMP:
      COPYINFLAG(SF_TIMESTAMP);
      break;

    case SO_BUSY_POLL:
      COPYININT(sp->busy_poll);
      break;

    default:
      s = -ENOPROTOOPT;
    }
#undef COPYININT
#undef COPYINFLAG
#undef COPYINTYPE
    if (s == 0) {
      // Tell the interface level that some option has changed.
      s = sp->interface->option_update(fp);
    }
  }

  vn_unlock(vp);
  return s;
}

static int sys_shutdown(int sockfd, int how)
{
  SOCKET_ENTER()
  return -ENOSYS;
}

static int sys_socket(int domain, int type, int protocol)
{
  const struct domain_interface *interface = NULL;

  // Choose the domain interface.
  switch (domain) {
  case AF_UNIX:
    interface  = unix_interface;
    break;
  case AF_INET:
  case AF_INET6:
    interface = inet_interface;
    break;
  case AF_IPX:
    interface = ipx_interface;
    break;
  case AF_NETLINK:
    interface = netlink_interface;
    break;
  case AF_X25:
    interface = x25_interface;
    break;
  case AF_AX25:
    interface = ax25_interface;
    break;
  case AF_ATMPVC:
    interface = atmpvc_interface;
    break;
  case AF_APPLETALK:
    interface = appletalk_interface;
    break;
  case AF_PACKET:
    interface = packet_interface;
    break;
  default:
    return -EINVAL;
  }

  if (interface == NULL) {
    return -EAFNOSUPPORT;
  }

  // Isolate the flags.
  unsigned flags = type & (SOCK_NONBLOCK|SOCK_CLOEXEC);
  type &= ~(SOCK_NONBLOCK|SOCK_CLOEXEC);

  /* Check whether the interface supports the protocol and type
   * and get its private data.
   */
  void *priv;
  int s = interface->setup(&priv, domain, protocol, type);
  if (s != 0)
    return s;

  return -ENOSYS;
}

static int sys_socketpair(int domain, int type, int protocol, int sv[2])
{
  return -ENOSYS;
}

#ifdef SYS_socketcall
static int sys_socketcall(int call, unsigned long *args)
{
  long arg[6];

  // Get the call arguments.
  copyin(arg, args, sizeof(arg));

  switch (call) {
  case __SC_accept:
    return sys_accept(args[0], (struct sockaddr *)arg[1], (socklen_t *)arg[2]);
  case __SC_accept4:
    return sys_accept4(args[0], (struct sockaddr *)arg[1], (socklen_t *)arg[2],
                       arg[3]);
  case __SC_bind:
    return sys_bind(args[0], (struct sockaddr *)arg[1], (socklen_t *)arg[2]);
  case __SC_connect:
    return sys_connect(args[0], (struct sockaddr *)arg[1], (socklen_t *)arg[2]);
  case __SC_getpeername:
    return sys_getpeername(args[0], (struct sockaddr *)arg[1],
                           (socklen_t *)arg[2]);
  case __SC_getsockname:
    return sys_getsockname(args[0], (struct sockaddr *)arg[1],
                           (socklen_t *)arg[2]);
  case __SC_getsockopt:
    return sys_getsockopt(args[0], arg[1], arg[2], (void *)arg[3],
                          (socklen_t *)arg[4]);
  case __SC_listen:
    return sys_listen(args[0], arg[1]);
  case __SC_recv:
    return sys_recv(args[0], (void *)arg[1], (size_t)arg[2], arg[3]);
  case __SC_recvfrom:
    return sys_recvfrom(args[0], (void *)arg[1], (size_t)arg[2], arg[3],
                        (struct sockaddr *)arg[4], (socklen_t *)arg[5]);
  case __SC_recvmmsg:
    return sys_recvmmsg(args[0], (struct msghdr *)arg[1], arg[2], arg[3],
                        (struct timespec *)arg[4]);
  case __SC_recvmsg:
    return sys_recvmsg(args[0], (struct msghdr *)arg[1], arg[2]);
  case __SC_send:
    return sys_send(args[0], (void *)arg[1], (size_t)arg[2], arg[3]);
  case __SC_sendto:
    return sys_sendto(args[0], (void *)arg[1], (size_t)arg[2], arg[3],
                      (struct sockaddr *)arg[4], (socklen_t *)arg[5]);
  case __SC_sendmmsg:
    return sys_sendmmsg(args[0], (struct msghdr *)arg[1], arg[2], arg[3]);
  case __SC_sendmsg:
    return sys_sendmsg(args[0], (struct msghdr *)arg[1], arg[2]);
  case __SC_setsockopt:
    return sys_setsockopt(args[0], arg[1], arg[2], (const void *)arg[3],
                          (socklen_t)arg[4]);
  case __SC_shutdown:
    return sys_shutdown(args[0], arg[1]);
  case __SC_socket:
    return sys_socket(args[0], arg[1], arg[2]);
  case __SC_socketpair:
    return sys_socketpair(args[0], arg[1], arg[2], (int *)arg[3]);

  default:
    return -EINVAL;
  }
}
#endif

// Define socket related system calls.
ELK_CONSTRUCTOR()
{
#ifndef SYS_socketcall

  SYSCALL(accept);
  SYSCALL(accept4);
  SYSCALL(bind);
  SYSCALL(connect);
  SYSCALL(getpeername);
  SYSCALL(getsockname);
  SYSCALL(getsockopt);
  SYSCALL(listen);
#ifdef SYS_recv
  SYSCALL(recv);
#endif
  SYSCALL(recvfrom);
  SYSCALL(recvmmsg);
  SYSCALL(recvmsg);
#ifdef SYS_send
  SYSCALL(send);
#endif
  SYSCALL(sendto);
  SYSCALL(sendmmsg);
  SYSCALL(sendmsg);
  SYSCALL(setsockopt);
  SYSCALL(shutdown);
  SYSCALL(socket);
  SYSCALL(socketpair);

#else

  SYSCALL(socketcall);

#endif
}
