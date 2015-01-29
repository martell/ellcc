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

#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "config.h"
#include "kernel.h"
#include "syscalls.h"
#include "crt1.h"
#include "thread.h"
#include "kmem.h"
#include "page.h"
#include "network.h"

// Make networking a select-able feature.
FEATURE(network)

domain_interface_t unix_interface;
domain_interface_t inet_interface;
domain_interface_t ipx_interface;
domain_interface_t netlink_interface;
domain_interface_t x25_interface;
domain_interface_t ax25_interface;
domain_interface_t atmpvc_interface;
domain_interface_t appletalk_interface;
domain_interface_t packet_interface;

/** Create and initialize a new socket.
 */
static socket_t new_socket(domain_interface_t interface,
                           int domain, int type, int protocol)
{
  socket_t sp = kmem_alloc(sizeof(struct socket));
  if (sp == NULL) {
    return NULL;
  }

  // Initialize default values.
  *sp = (struct socket){ .domain = domain, .type = type, .protocol = protocol,
                         .interface = interface,
                       };

  return sp;
}

/** Create and initialize a new buffer.
 */
int net_new_buffer(struct buffer **buf, int max, int total)
{
  if (max < 1 || total < 1 || max > total) {
    return -EINVAL;
  }

  // Allocate a buffer and room for the maximum buffer pointers.
  *buf = kmem_alloc(sizeof(struct buffer) + (total * sizeof(char *)));
  if (*buf == NULL) {
    return -ENOMEM;
  }

  // Initialize default values.
  (*buf)->refcnt = 1;
  (*buf)->max = max;
  (*buf)->total = total;
  (*buf)->used = 0;                     // Bytes currently buffered.
  (*buf)->available = max * PAGE_SIZE;  // Available buffer space.
  sem_init(&(*buf)->osem, 0, 0);
  sem_init(&(*buf)->isem, 0, 0);
  memset((*buf)->buf, 0, total * sizeof(char *));
  return 0;
}

/** Release a buffer.
 * There may be multiple threads using the buffer, so watch the
 * reference counts.
 */
void net_release_buffer(struct buffer *buf)
{
  if (buf == NULL) return;

  pthread_mutex_lock(&buf->mutex);
  ASSERT(buf->refcnt > 0);
  --buf->refcnt;
  pthread_mutex_unlock(&buf->mutex);
  if (buf->refcnt == 0) {
    // The buffer is unused. Delete it.
    pthread_mutex_destroy(&buf->mutex);
    sem_destroy(&buf->osem);
    sem_destroy(&buf->isem);
    for (int i = 0; i < buf->max; ++i) {
      if (buf->buf[i])
        // RICH: Use mummap() here?
        vm_free(getpid(), buf->buf[i], PAGE_SIZE);
    }
    kmem_free(buf);
  }
}

/** Send bytes to a buffer.
 */
ssize_t net_buffer_send(struct socket *sp, const char *buffer, size_t size,
        int flags, const struct sockaddr *to, socklen_t tolen)
{
  int s;
  struct buffer *buf = sp->snd;
  size_t total = 0;
  while (size) {
    pthread_mutex_lock(&buf->mutex);
    if (buf->used >= buf->available) {
      // The buffer is full.
      pthread_mutex_unlock(&buf->mutex);
      if (flags & MSG_DONTWAIT) {
        if (total > 0) {
          // We have sent some data.
          return total;
        }

        return -EWOULDBLOCK;
      } else {
        // Wait for space to become available.
        sem_wait(&buf->osem);
        continue;               // And try again.
      }
    }

    if (buf->buf[buf->out_index] == NULL) {
      // No buffer has been allocated here yet.
      void *addr;
      s = vm_allocate(getpid(), &addr, PAGE_SIZE, 1);
      if (s < 0) {
        pthread_mutex_unlock(&buf->mutex);
        return s;
      }
      buf->buf[buf->out_index] = addr;
      buf->out_offset = 0;
    }

    char *bp = buf->buf[buf->out_index];  // Get the current buffer.
    size_t rem = (PAGE_SIZE - buf->out_offset);
    if (rem >= size) {
      // There is enough room in this buffer.
      s = copyin(buffer, &bp[buf->out_offset], size);
      if (s < 0) {
        pthread_mutex_unlock(&buf->mutex);
        return s;
      }
      buf->out_offset += size;
      buf->used += size;
      total += size;
      size = 0;
    } else {
      // Do a partial write.
      s = copyin(buffer, &bp[buf->out_offset], rem);
      if (s < 0) {
        pthread_mutex_unlock(&buf->mutex);
        return s;
      }
      size -= rem;
      buffer += rem;
      buf->out_offset += rem;
      buf->used += rem;
      total += rem;
    }

    // Bump the output pointer.
    if (buf->out_offset >= PAGE_SIZE) {
      // Go to the next page.
      ++buf->out_index;
      if (buf->out_index >= buf->max) {
        buf->out_index = 0;
      }
      buf->out_offset = 0;
    }

    pthread_mutex_unlock(&buf->mutex);
  }

  if (total)
    sem_post(&buf->isem);               // Data is available.
  return total;
}

/** Get bytes from a buffer.
 */
ssize_t net_buffer_recv(struct socket *sp, char *buffer, size_t size, int flags,
                        struct sockaddr *from, socklen_t *fromlen)
{
  int s;
  struct buffer *buf = sp->rcv;
  size_t total = 0;
  while (size) {
    pthread_mutex_lock(&buf->mutex);
    if (buf->used == 0) {
      // The buffer is empty.
      pthread_mutex_unlock(&buf->mutex);
      if (flags & MSG_DONTWAIT) {
        if (total > 0) {
          // We have received some data.
          return total;
        }

        return -EWOULDBLOCK;
      } else {
        // Wait for data to become available.
        sem_wait(&buf->isem);
        continue;               // And try again.
      }
    }

    char *bp = buf->buf[buf->in_index];  // Get the current buffer.
    ASSERT(bp != NULL);
    size_t rem = (PAGE_SIZE - buf->in_offset);
    if (rem >= size && buf->used >= size) {
      // There is enough data in this buffer.
      s = copyout(&bp[buf->in_offset], buffer, size);
      if (s < 0) {
        pthread_mutex_unlock(&buf->mutex);
        return s;
      }
      buf->in_offset += size;
      buf->used -= size;
      total += size;
      size = 0;
    } else {
      // Do a partial read. Read either the remainder of the buffer
      // or the amount of data that is available.
      if (buf->used < rem)
        rem = buf->used;
      s = copyout(&bp[buf->in_offset], buffer, rem);
      if (s < 0) {
        pthread_mutex_unlock(&buf->mutex);
        return s;
      }
      size -= rem;
      buffer += rem;
      buf->in_offset += rem;
      buf->used -= rem;
      total += rem;
    }

    // Bump the input pointer.
    if (buf->in_offset >= PAGE_SIZE) {
      // Go to the next page.
      ++buf->in_index;
      if (buf->in_index >= buf->max) {
        buf->in_index = 0;
      }
      buf->in_offset = 0;
    }

    pthread_mutex_unlock(&buf->mutex);
  }

  if (total)
    sem_post(&buf->osem);               // Space is available.
  return total;
}

/** Send data to a connection.
 */
static int net_out(struct socket *sp, struct uio *uio, size_t *size, int flags)
{
  ssize_t s = 0;
  size_t total = 0;
  for (int i = 0; i < uio->iovcnt; ++i) {
    char *buffer = uio->iov[i].iov_base;
    size_t nbyte = uio->iov[i].iov_len;
    // Write nbyte bytes from buffer to buf.
    while (nbyte > 0) {
      // Send bytes to a connection.
      s = sp->interface->sendto(sp, buffer, nbyte, flags, NULL, 0);
      if (s < 0) {
        return s;
      }
      nbyte -= s;
      buffer += s;
      total += s;
    }
  }

  return copyout(&total, size, sizeof(*size));
}

/** Get data from a connection.
 */
static int net_in(struct socket *sp, struct uio *uio, size_t *size, int flags)
{
  ssize_t s = 0;
  size_t total = 0;
  for (int i = 0; i < uio->iovcnt; ++i) {
    char *buffer = uio->iov[i].iov_base;
    size_t nbyte = uio->iov[i].iov_len;
    // Read nbyte bytes from the buffer.
    while (nbyte > 0) {
      // Get bytes from the connection.
      s = sp->interface->recvfrom(sp, buffer, nbyte, flags, NULL, 0);
      if (s < 0 || s < nbyte) {
        // Either an error occured or we would have blocked.
        return s;
      }
      nbyte -= s;
      buffer += s;
      total += s;
    }
  }

  return copyout(&total, size, sizeof(*size));
}

/** Default vnode operations for sockets.
 */

/** Open a socket file.
 * RICH: This is questionable.
 */
int net_open(vnode_t vp, int flags)
{
  if (unix_interface == NULL) {
    return -EAFNOSUPPORT;
  }

  socket_t sp = new_socket(unix_interface, AF_UNIX, SOCK_STREAM, 0);
  if (sp == NULL) {
    return -ENOMEM;
  }

  // The vnode will now own the socket structure.
  VN_RW_OVERRIDE(vp)->v_data = sp;

  /* Check whether the interface supports the protocol and type
   * and set it up.
   */
  int s = unix_interface->setup(vp, 0);
  if (s != 0) {
    kmem_free(sp);
    vput(vp);
    return s;
  }

  return 0;
}

/** Close a socket.
 */
int net_close(vnode_t vp, file_t fp)
{
  socket_t sp = fp->f_vnode->v_data;

  sp->interface->close(fp);
  return 0;
}

int net_read(vnode_t vp, file_t fp, struct uio *uio, size_t *count)
{
  struct socket *sp = vp->v_data;       // Get the socket.
  return net_in(sp, uio, count, (fp->f_flags & O_NONBLOCK) ? MSG_DONTWAIT : 0);
}

/** Write to a socket connection.
 */
int net_write(vnode_t vp, file_t fp, struct uio *uio, size_t *count)
{
  struct socket *sp = vp->v_data;       // Get the socket.
  return net_out(sp, uio, count, (fp->f_flags & O_NONBLOCK) ? MSG_DONTWAIT : 0);
}

int net_poll(vnode_t vp, file_t fp, int what)
{
  return -EPROTONOSUPPORT;
}

int net_seek(vnode_t vp, file_t fp, off_t foffset, off_t offset)
{
  return -EINVAL;
}

int net_ioctl(vnode_t vp, file_t fp, u_long request, void *buf)
{
  struct socket *sp = vp->v_data;

  switch (request) {
  // Socket ioctls.
  case SIOCGSTAMP:
  case SIOCSPGRP:
  case FIOASYNC:
  case SIOCGPGRP:
  default:
    return sp->interface->ioctl(fp, request, buf);
  }
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
  // Free the socket specific data.
  kmem_free(vp->v_data);
  return 0;
}

int net_truncate(vnode_t vp, off_t offset)
{
  return -EINVAL;
}

/** Common network vnode operations.
 */
const struct vnops net_vnops = {
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

static int getsockfd(struct socket *sp, int flags, int setup)
{
  vnode_t vp = vget(NULL, NULL);        // Get an anonymous vnode.
  if (vp == NULL) {
    kmem_free(sp);
    return -ENOMEM;
  }
  VN_RW_OVERRIDE(vp)->v_op = sp->interface->vnops;
  VN_RW_OVERRIDE(vp)->v_type = VSOCK;
  // The vnode will now own the socket structure.
  VN_RW_OVERRIDE(vp)->v_data = sp;

  // Setup the file structure.
  file_t fp;
  if (!(fp = kmem_alloc(sizeof(struct file)))) {
    vput(vp);
    return -ENOMEM;
  }

  *fp = (struct file){ .f_vnode = vp, .f_flags = FREAD|FWRITE,
                       .f_offset = 0, .f_count = 1 };

  if (flags & SOCK_NONBLOCK)
    fp->f_flags |= O_NONBLOCK;
  if (flags & SOCK_CLOEXEC)
    fp->f_flags |= O_CLOEXEC;

  if (setup) {
    // Set up the new socket.
    int s = sp->interface->setup(vp, flags);
    if (s != 0) {
      vput(vp);
      return s;
    }
  }

  /* Check whether the interface supports the protocol and type
   * and set it up.
   */
  // Finally, allocate a file descriptor.
  int fd = allocfd(fp);
  if (fd < 0) {
    sp->interface->close(fp);
    kmem_free(fp);
    vput(vp);
    return fd;
  }

  vn_unlock(vp);
  return fd;
}
/** Socket related system calls.
 */

/** Generic socket system call entry.
 * on exit, fp is the file pointer, vp is the vnode pointer,
 * sp is the socket pointer, s is zero indicating no error.
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
  socket_t sp = vp->v_data;

#define IS_LISTEN()                                             \
  if (!(sp->flags & SF_ACCEPTCONN)) {                           \
    return -EINVAL;                                             \
  }

#define NOT_LISTEN()                                            \
  if (sp->flags & SF_ACCEPTCONN) {                              \
    return -EINVAL;                                             \
  }

#define IS_BOUND()                                              \
  if (!(sp->flags & SF_BOUND)) {                                \
    return -EINVAL;                                             \
  }

#define NOT_BOUND()                                             \
  if (sp->flags & SF_BOUND) {                                   \
    return -EINVAL;                                             \
  }

#define IS_CONNECTED()                                          \
  if (!(sp->flags & SF_CONNECTED)) {                            \
    return -ENOTCONN;                                           \
  }

#define NOT_CONNECTED()                                         \
  if (sp->flags & SF_CONNECTED) {                               \
    return -EINVAL;                                             \
  }

static int sys_accept4(int sockfd, struct sockaddr *addr, socklen_t *addrlen,
                       int flags)
{
  SOCKET_ENTER()
  IS_LISTEN()
  socket_t newsp = new_socket(sp->interface, sp->domain, sp->type,
                              sp->protocol);
  if (newsp == NULL) {
    return -ENOMEM;
  }

  s = sp->interface->accept4(sp, newsp, addr, addrlen, flags);
  if (s < 0) {
    kmem_free(newsp);
    // The accept failed.
    return s;
  }

  // Create a file for the socket and return the socket descriptor.
  return getsockfd(newsp, 0, 0);
}

static int sys_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
  return sys_accept4(sockfd, addr, addrlen, 0);
}

static int sys_bind(int sockfd, struct sockaddr *addr, socklen_t addrlen)
{
  SOCKET_ENTER()
  NOT_BOUND()
  return sp->interface->bind(fp, addr, addrlen);
}

static int sys_connect(int sockfd, struct sockaddr *addr, socklen_t addrlen)
{
  SOCKET_ENTER()
  NOT_LISTEN()
  return sp->interface->connect(sp, addr, addrlen);
}

static int sys_getpeername(int sockfd, struct sockaddr *addr,
                           socklen_t *addrlen)
{
  SOCKET_ENTER()
  IS_CONNECTED()
  return sp->interface->getpeername(sp, addr, addrlen);
}

static int sys_getsockname(int sockfd, struct sockaddr *addr,
                           socklen_t *addrlen)
{
  SOCKET_ENTER()
  IS_BOUND()
  return sp->interface->getsockname(sp, addr, addrlen);
}

/** Check for the existance of a buffer.
 */
#define CHECK_BUFFER(buf)                                       \
      if (buf == NULL) {                                        \
        s = -EAGAIN;                                            \
        break;                                                  \
      }

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
      COPYOUTTYPE(struct ucred, &sp->ucred);
      break;

    case SO_PRIORITY:
      COPYOUTINT(sp->priority);
      break;

    case SO_PROTOCOL:
      COPYOUTINT(sp->protocol);
      break;

    case SO_RCVBUF:
    case SO_RCVBUFFORCE: {
      CHECK_BUFFER(sp->rcv);
      int rcvmax = sp->rcv->max * PAGE_SIZE;
      COPYOUTINT(rcvmax);
      break;
    }

    case SO_RCVLOWAT:
      COPYOUTINT(sp->rcvlowait);
      break;

    case SO_SNDLOWAT:
      COPYOUTINT(sp->sndlowait);
      break;

    case SO_RCVTIMEO:
      CHECK_BUFFER(sp->rcv);
      COPYOUTTYPE(struct timeval, &sp->rcv->timeo);
      break;

    case SO_SNDTIMEO:
      CHECK_BUFFER(sp->snd);
      COPYOUTTYPE(struct timeval, &sp->snd->timeo);
      break;

    case SO_REUSEADDR:
      COPYOUTINT((sp->flags & SF_REUSEADDR) != 0);
      break;

    case SO_RXQ_OVFL:
      COPYOUTINT((sp->flags & SF_RXQ_OVFL) != 0);
      break;

    case SO_SNDBUF:
    case SO_SNDBUFFORCE: {
      CHECK_BUFFER(sp->snd);
      int sndmax = sp->snd->max * PAGE_SIZE;
      COPYOUTINT(sndmax);
      break;
    }

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
  NOT_LISTEN()
  NOT_CONNECTED()
  s = sp->interface->listen(sp, backlog);
  if (s < 0) {
    return s;
  }
  sp->flags |= SF_ACCEPTCONN;
  return 0;
}

#if defined(SYS_socketcall) || defined(SYS_recv)
static int sys_recv(int sockfd, void *buf, size_t len, int flags)
{
  SOCKET_ENTER()
  NOT_LISTEN()
  return sp->interface->recvfrom(sp, buf, len, flags, NULL, NULL);
}
#endif

static int sys_recvfrom(int sockfd, void *buf, size_t len, int flags,
                        struct sockaddr *src_addr, socklen_t *addrlen)
{
  SOCKET_ENTER()
  NOT_LISTEN()
  return sp->interface->recvfrom(sp, buf, len, flags, src_addr, addrlen);
}

static int sys_recvmmsg(int sockfd, struct msghdr *msgvec, unsigned int vlen,
                        unsigned int flags, struct timespec *timeout)
{
  SOCKET_ENTER()
  NOT_LISTEN()
  return -ENOSYS;
}

static int sys_recvmsg(int sockfd, struct msghdr *msgvec, unsigned int flags)
{
  SOCKET_ENTER()
  NOT_LISTEN()
  return -ENOSYS;
}

#if defined(SYS_socketcall) || defined(SYS_send)
static int sys_send(int sockfd, const void *buf, size_t len, int flags)
{
  SOCKET_ENTER()
  NOT_LISTEN()
  return sp->interface->sendto(sp, buf, len, flags, NULL, 0);
}
#endif

static int sys_sendto(int sockfd, const void *buf, size_t len, int flags,
                        struct sockaddr *src_addr, socklen_t addrlen)
{
  SOCKET_ENTER()
  NOT_LISTEN()
  return sp->interface->sendto(sp, buf, len, flags, src_addr, addrlen);
}

static int sys_sendmmsg(int sockfd, const struct msghdr *msgvec,
                        unsigned int vlen, unsigned int flags)
{
  SOCKET_ENTER()
  NOT_LISTEN()
  return -ENOSYS;
}

static int sys_sendmsg(int sockfd, const struct msghdr *msgvec,
                       unsigned int flags)
{
  SOCKET_ENTER()
  NOT_LISTEN()
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
      int rcvmax;
      COPYININT(rcvmax);
      rcvmax /= PAGE_SIZE;
      if (rcvmax < CONFIG_SO_RCVBUF_MIN || rcvmax > CONFIG_SO_RCVBUF_MAX) {
        s = -EINVAL;
      } else {
        CHECK_BUFFER(sp->rcv);
        sp->rcv->max = rcvmax;
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
      CHECK_BUFFER(sp->rcv);
      COPYINTYPE(struct timeval, sp->rcv->timeo);
      break;

    case SO_SNDTIMEO:
      CHECK_BUFFER(sp->snd);
      COPYINTYPE(struct timeval, sp->snd->timeo);
      break;

    case SO_REUSEADDR:
      COPYINFLAG(SF_REUSEADDR);
      break;

    case SO_RXQ_OVFL:
      COPYINFLAG(SF_RXQ_OVFL);
      break;

    case SO_SNDBUF:
    case SO_SNDBUFFORCE: {
      int sndmax;
      COPYININT(sndmax);
      sndmax /= PAGE_SIZE;
      if (sndmax < CONFIG_SO_SNDBUF_MIN || sndmax > CONFIG_SO_SNDBUF_MAX) {
        s = -EINVAL;
      } else {
        CHECK_BUFFER(sp->snd);
        sp->snd->max = sndmax;
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
  NOT_LISTEN()
  IS_CONNECTED()
  return sp->interface->shutdown(sp, how);
}

static int sys_socket(int domain, int type, int protocol)
{
  domain_interface_t interface = NULL;

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

  // Isolate the flags. RICH: Use them.
  unsigned flags = type & (SOCK_NONBLOCK|SOCK_CLOEXEC);
  type &= ~(SOCK_NONBLOCK|SOCK_CLOEXEC);

  socket_t sp = new_socket(interface, domain, type, protocol);
  if (sp == NULL) {
    return -ENOMEM;
  }

  // Create a file for the socket and return the socket descriptor.
  return getsockfd(sp, flags, 1);
}

static int sys_socketpair(int domain, int type, int protocol, int sv[2])
{
  int lsv[2];
  lsv[0] = sys_socket(domain, type, protocol);
  if (lsv[0] < 0) {
    return lsv[0];
  }

  lsv[1] = sys_socket(domain, type, protocol);
  if (lsv[1] < 0) {
    close(lsv[0]);
    return lsv[1];
  }

  // Connect the two sockets.
  int s;
  file_t fp1, fp2;
  s = getfile(lsv[0], &fp1);
  if (s < 0) {
    close(lsv[0]);
    close(lsv[1]);
    return s;
  }

  s = getfile(lsv[1], &fp2);
  if (s < 0) {
    close(lsv[0]);
    close(lsv[1]);
    return s;
  }

  socket_t sp1 = fp1->f_vnode->v_data;
  socket_t sp2 = fp2->f_vnode->v_data;
  // Create the buffers.
  s = net_new_buffer(&sp1->snd, CONFIG_SO_BUFFER_DEFAULT_PAGES,
                     CONFIG_SO_BUFFER_PAGES);
  if (s < 0) {
    close(lsv[0]);
    close(lsv[1]);
    return s;
  }
  s = net_new_buffer(&sp2->snd, CONFIG_SO_BUFFER_DEFAULT_PAGES,
                     CONFIG_SO_BUFFER_PAGES);
  if (s < 0) {
    close(lsv[0]);
    close(lsv[1]);
    return s;
  }

  // Connect the buffers.
  sp1->rcv = sp2->snd;
  ++sp1->rcv->refcnt;
  sp2->rcv = sp1->snd;
  ++sp2->rcv->refcnt;
  // Mark the sockets as bound and connected.
  sp1->flags |= SF_BOUND|SF_CONNECTED;
  sp2->flags |= SF_BOUND|SF_CONNECTED;
  return copyout(lsv, sv, sizeof(lsv));
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
    return sys_bind(args[0], (struct sockaddr *)arg[1], (socklen_t)arg[2]);
  case __SC_connect:
    return sys_connect(args[0], (struct sockaddr *)arg[1], (socklen_t)arg[2]);
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
                      (struct sockaddr *)arg[4], (socklen_t)arg[5]);
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
ELK_PRECONSTRUCTOR()
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
