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

#define _GNU_SOURCE
#include <sys/socket.h>

#include "config.h"
#include "vnode.h"

typedef struct socket *socket_t;

/* Network domain interface functions.
 */
typedef const struct domain_interface
{
  // Check arguments and allocate private data.
  int (*setup)(vnode_t vp, int flags);
  int (*getopt)(file_t fp, int level, int optname, void *optval,
                socklen_t *optlen);
  int (*setopt)(file_t fp, int level, int optname, const void *optval,
                socklen_t optlen);
  int (*option_update)(file_t fp);
  int (*ioctl)(file_t fp, u_long request, void *buf);
  int (*bind)(file_t fp, struct sockaddr *addr, socklen_t addrlen);
  int (*listen)(struct socket *sp, int backlog);
  int (*connect)(struct socket *sp, const struct sockaddr *addr,
                 socklen_t addrlenlen);
  int (*accept4)(struct socket *sp, struct socket *newsp,
                struct sockaddr *addr, socklen_t *addrlen, int flags);
  ssize_t (*sendto)(struct socket *sp, const char *buffer, size_t size,
                    int flags, const struct sockaddr *to, socklen_t tolen);
  ssize_t (*recvfrom)(struct socket *sp, char *buffer, size_t size, int flags,
                      struct sockaddr *from, socklen_t *fromlen);
  int (*getpeername)(struct socket *sp, struct sockaddr *addr,
                            socklen_t *addrlen);
  int (*getsockname)(struct socket *sp, struct sockaddr *addr,
                            socklen_t *addrlen);
  int (*shutdown)(struct socket *sp, int how);
  int (*close)(file_t fp);
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
#define net_vnops __elk_net_vnops
#endif

const struct vnops net_vnops;

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
#define net_release_buffer __elk_net_release_buffer
#endif

/** Network support functions.
 */

/** Create and initialize a new buffer.
 */
int net_new_buffer(struct buffer **buf, int max, int total);
/** Release a buffer.
 * There may be multiple threads using the buffer, so watch the
 * reference counts.
 */
void net_release_buffer(struct buffer *buf);
/** Send bytes to a buffer.
 */
ssize_t net_buffer_send(struct socket *sp, const char *buffer, size_t size,
                        int flags, const struct sockaddr *to, socklen_t tolen);
/** Get bytes from a buffer.
 */
ssize_t net_buffer_recv(struct socket *sp, char *buffer, size_t size, int flags,
                        struct sockaddr *from, socklen_t *fromlen);


#endif // _network_h_
