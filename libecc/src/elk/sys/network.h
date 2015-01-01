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

#ifndef _network_h_
#define _network_h_

#include <sys/socket.h>

#include "config.h"
#include "vnode.h"

typedef struct socket *socket_t;

/* Network domain interface functions.
 */
typedef const struct domain_interface
{
  // Check arguments and allocate provate data.
  int (*setup)(vnode_t vp);
  int (*getopt)(file_t fp, int level, int optname, void *optval,
                socklen_t *optlen);
  int (*setopt)(file_t fp, int level, int optname, const void *optval,
                socklen_t optlen);
  int (*option_update)(file_t fp);
  int (*bind)(file_t fp, struct sockaddr *addr, socklen_t addrlen);
  const struct vnops *vnops;            // Vnode operations.
} *domain_interface_t;

#if ELK_NAMESPACE
#define unix_interface __elk_unix_interface
#define inet_interface __elk_inet_interface
#define ipx_interface __elk_ipx_interface
#define netlink_interface __elk_netlink_interface
#define x25_interface __elk_x25_interface
#define ax25_interface __elk_ax25_interface
#define atmpvc_interface __elk_atmpvc_interface
#define appletalk_interface __elk_appletalk_interface
#define packet_interface __elk_packet_interface
#endif

/* Interfaces to different domains. These pointers are filled in
 * by the interface modes that implement them. This file assumes
 * a NULL interface pointer is an unsupported domain.
 */
extern domain_interface_t unix_interface;
extern domain_interface_t inet_interface;
extern domain_interface_t ipx_interface;
extern domain_interface_t netlink_interface;
extern domain_interface_t x25_interface;
extern domain_interface_t ax25_interface;
extern domain_interface_t atmpvc_interface;
extern domain_interface_t appletalk_interface;
extern domain_interface_t packet_interface;

#if ELK_NAMESPACE
#define net_open __elk_net_open
#define net_close __elk_net_close
#define net_read __elk_net_read
#define net_write __elk_net_write
#define net_poll __elk_net_poll
#define net_seek __elk_net_seek
#define net_ioctl __elk_net_ioctl
#define net_fsync __elk_net_fsync
#define net_readdir __elk_net_readdir
#define net_lookup __elk_net_lookup
#define net_create __elk_net_create
#define net_remove __elk_net_remove
#define net_rename __elk_net_rename
#define net_mkdir __elk_net_mkdir
#define net_rmdir __elk_net_rmdir
#define net_getattr __elk_net_getattr
#define net_setattr __elk_net_setattr
#define net_inactive __elk_net_inactive
#define net_truncate __elk_net_truncate
#endif

/* Default socket vnode operations.
 */
int net_open(vnode_t vp, int flags);
int net_close(vnode_t vp, file_t fp);
int net_read(vnode_t vp, file_t fp, struct uio *uio, size_t *count);
int net_write(vnode_t vp, file_t fp, struct uio *uio, size_t *count);
int net_poll(vnode_t vp, file_t fp, int what);
int net_seek(vnode_t vp, file_t fp, off_t foffset, off_t offset);
int net_ioctl(vnode_t vp, file_t fp, u_long request, void *buf);
int net_fsync(vnode_t vp, file_t fp);
int net_readdir(vnode_t vp, file_t fp, struct dirent *dir);
int net_lookup(vnode_t dvp, char *name, vnode_t vp);
int net_create(vnode_t vp, char *name, mode_t mode);
int net_remove(vnode_t dvp, vnode_t vp, char *name);
int net_rename(vnode_t dvp1, vnode_t vp1, char *sname,
               vnode_t dvp2, vnode_t vp2, char *dname);
int net_mkdir(vnode_t vp, char *name, mode_t mode);
int net_rmdir(vnode_t dvp, vnode_t vp, char *name);
int net_getattr(vnode_t vp, struct vattr *vattr);
int net_setattr(vnode_t vp, struct vattr *vattr);
int net_inactive(vnode_t vp);
int net_truncate(vnode_t vp, off_t offset);

/** A socket buffer.
 */
struct buffer
{
  pthread_mutex_t mutex;                // The buffer mutex.
  int refcnt;                           // The buffer reference count.
  sem_t isem;                           // The data available semaphore.
  sem_t osem;                           // The space available semaphore.
  struct timeval timeo;                 // The timeout value.
  int max;                              // The max buffer page count.
  int total;                            // The total buffer page count.
  size_t used;                          // The number of bytes used.
  size_t available;                     // The total number of bytes available.
  int in_index;                         // Buffer input index.
  int in_offset;                        // Input offset in buffer.
  int out_index;                        // Buffer output index.
  int out_offset;                       // Output offset in buffer.
  char *buf[];                          // The buffer pointers.
};

/** The state of a socket.
 */
struct socket
{
  int domain;                           // The socket's domain.
  int type;                             // The socket's type.
  int protocol;                         // The socket's protocol.
  unsigned flags;                       // Socket flags. See below.
  int error;                            // The pending socket error.
  int linger;                           // The linger time in seconds.
  int mark;                             // The mark.
  int peek_off;                         // The peek offset.
  int priority;                         // Packet priority.
  int rcvlowait;                        // Minimum bytes to receive.
  int sndlowait;                        // Minimum bytes to send.
  int busy_poll;                        // The busy poll time in microseconds.
  const domain_interface_t interface;   // The domain interface.
  void *priv;                           // Domain private data.
  struct ucred ucred;                   // Connect credentials.
  struct buffer *snd;                   // The send buffer.
  struct buffer *rcv;                   // The receive buffer.
};

/** Socket flag values.
 */
#define SF_ACCEPTCONN   0x00000001      // Will accept connections.
#define SF_BROADCAST    0x00000002      // Can broadcast.
#define SF_BSDCOMPAT    0x00000004      // BSD bug compatibility.
#define SF_DEBUG        0x00000008      // Enable debugging.
#define SF_DONTROUTE    0x00000010      // Don't send via a gateway.
#define SF_KEEPALIVE    0x00000020      // Enable keep-alive messages.
#define SF_LINGER       0x00000040      // Linger has been set.
#define SF_MARK         0x00000080      // The mark has been set.
#define SF_OOBINLINE    0x00000100      // Send out-of-band data in line.
#define SF_PASSCRED     0x00000200      // Enable SCM_CREDENTIALS.
#define SF_REUSEADDR    0x00000400      // Reuse local addresses.
#define SF_RXQ_OVFL     0x00000800      // Enable dropped packet count.
#define SF_TIMESTAMP    0x00001000      // Enable SO_TIMESTAMP control message.
#define SF_BOUND        0x00002000      // The socket has been bound.
#define SF_CONNECTED    0x00004000      // The socket is connected.

#if ELK_NAMESPACE
#define net_new_buffer __elk_net_new_buffer
#endif

/** Network support functions.
 */

/** Create and initialize a new buffer.
 */
int net_new_buffer(struct buffer **buf, int max, int total);

#endif // _network_h_
