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
 *
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define _GNU_SOURCE
#include <netinet/in.h>

#include "config.h"
#include "kernel.h"
#include "kmem.h"
#include "network.h"
#include "lwip/tcpip.h"

// Make LwIP AF_INET(6) networking a select-able feature.
FEATURE_CLASS(lwip_network, inet_network)

/** All internal pointers and states used for a socket.
 */
struct lwip_sock
 {
  // Sockets currently are built on netconns, each socket has one netconn.
  struct netconn *conn;
  // data that was left from the previous read.
  void *lastdata;
  // Offset in the data that was left from the previous read.
  int lastoffset;
  /** Number of times data was received, set by event_callback(),
   * tested by the receive and select functions
   */
  int rcvevent;
  /** Number of times data was ACKed (free send buffer), set by
   *  event_callback(), tested by select.
   */
  int sendevent;
  /** Error happened for this socket, set by event_callback(),
   * tested by select */
  int errevent;
  // The last error that occurred on this socket.
  int err;
  // Counter of how many threads are waiting for this socket using select.
  int select_waiting;
};

/** Table to quickly map an lwIP error (err_t) to a socket error
  * by using -err as an index */
static const int err_to_errno_table[] = {
  0,             /* ERR_OK          0      No error, everything OK. */
  ENOMEM,        /* ERR_MEM        -1      Out of memory error.     */
  ENOBUFS,       /* ERR_BUF        -2      Buffer error.            */
  EWOULDBLOCK,   /* ERR_TIMEOUT    -3      Timeout                  */
  EHOSTUNREACH,  /* ERR_RTE        -4      Routing problem.         */
  EINPROGRESS,   /* ERR_INPROGRESS -5      Operation in progress    */
  EINVAL,        /* ERR_VAL        -6      Illegal value.           */
  EWOULDBLOCK,   /* ERR_WOULDBLOCK -7      Operation would block.   */
  EADDRINUSE,    /* ERR_USE        -8      Address in use.          */
  EALREADY,      /* ERR_ISCONN     -9      Already connected.       */
  ENOTCONN,      /* ERR_CONN       -10     Not connected.           */
  ECONNABORTED,  /* ERR_ABRT       -11     Connection aborted.      */
  ECONNRESET,    /* ERR_RST        -12     Connection reset.        */
  ENOTCONN,      /* ERR_CLSD       -13     Connection closed.       */
  EIO,           /* ERR_ARG        -14     Illegal argument.        */
  -1,            /* ERR_IF         -15     Low-level netif error    */
};

#define ERR_TO_ERRNO_TABLE_SIZE \
  (sizeof(err_to_errno_table)/sizeof(err_to_errno_table[0]))

#define err_to_errno(err) \
  ((unsigned)(-(err)) < ERR_TO_ERRNO_TABLE_SIZE ? \
    err_to_errno_table[-(err)] : EIO)

#if LWIP_IPV6

#define IS_SOCK_ADDR_LEN_VALID(namelen) \
  (((namelen) == sizeof(struct sockaddr_in)) || \
  ((namelen) == sizeof(struct sockaddr_in6)))
#define IS_SOCK_ADDR_TYPE_VALID(name) (((name)->sa_family == AF_INET) || \
                                      ((name)->sa_family == AF_INET6))

#define SOCK_ADDR_TYPE_MATCH(name, sock) \
  ((((name)->sa_family == AF_INET) && \
    !(NETCONNTYPE_ISIPV6((sock)->conn->type))) || \
   (((name)->sa_family == AF_INET6) && \
    (NETCONNTYPE_ISIPV6((sock)->conn->type))))

#define IP6ADDR_PORT_TO_SOCKADDR(sin6, ipXaddr, port) \
  do { \
    (sin6)->sin6_len = sizeof(struct sockaddr_in6); \
    (sin6)->sin6_family = AF_INET6; \
    (sin6)->sin6_port = htons((port)); \
    (sin6)->sin6_flowinfo = 0; \
    inet6_addr_from_ip6addr(&(sin6)->sin6_addr, ipX_2_ip6(ipXaddr)); \
  }while(0)

#define IPXADDR_PORT_TO_SOCKADDR(isipv6, sockaddr, ipXaddr, port) \
  do { \
    if (isipv6) { \
      IP6ADDR_PORT_TO_SOCKADDR((struct sockaddr_in6*)(void*)(sockaddr), \
                                ipXaddr, port); \
    } else { \
      IP4ADDR_PORT_TO_SOCKADDR((struct sockaddr_in*)(void*)(sockaddr), \
                               ipXaddr, port); \
    } \
  } while(0)

#define SOCKADDR6_TO_IP6ADDR_PORT(sin6, ipXaddr, port) \
  do { \
    inet6_addr_to_ip6addr(ipX_2_ip6(ipXaddr), &((sin6)->sin6_addr)); \
    (port) = ntohs((sin6)->sin6_port); \
  } while(0)

#define SOCKADDR_TO_IPXADDR_PORT(isipv6, sockaddr, ipXaddr, port) \
  do { \
    if (isipv6) { \
      SOCKADDR6_TO_IP6ADDR_PORT((struct sockaddr_in6*)(void*)(sockaddr), \
                                ipXaddr, port); \
    } else { \
      SOCKADDR4_TO_IP4ADDR_PORT((struct sockaddr_in*)(void*)(sockaddr), \
                                ipXaddr, port); \
    } \
  } while(0)

#define DOMAIN_TO_NETCONN_TYPE(domain, type) (((domain) == AF_INET) ? \
  (type) : (enum netconn_type)((type) | NETCONN_TYPE_IPV6))

#else // LWIP_IPV6

#define IS_SOCK_ADDR_LEN_VALID(namelen) ((namelen) == sizeof(struct sockaddr_in))
#define IS_SOCK_ADDR_TYPE_VALID(name) ((name)->sa_family == AF_INET)
#define SOCK_ADDR_TYPE_MATCH(name, sock) 1
#define IPXADDR_PORT_TO_SOCKADDR(isipv6, sockaddr, ipXaddr, port) \
  IP4ADDR_PORT_TO_SOCKADDR((struct sockaddr_in*)(void*)(sockaddr), \
                            ipXaddr, port)
#define SOCKADDR_TO_IPXADDR_PORT(isipv6, sockaddr, ipXaddr, port) \
  SOCKADDR4_TO_IP4ADDR_PORT((struct sockaddr_in*)(void*)(sockaddr), ipXaddr, \
                             port)
#define DOMAIN_TO_NETCONN_TYPE(domain, netconn_type) (netconn_type)

#endif /* LWIP_IPV6 */

/** Callback registered in the netconn layer for each socket-netconn.
 * Processes recvevent (data available) and wakes up tasks waiting for select.
 */
static void event_callback(struct netconn *conn, enum netconn_evt evt,
                           u16_t len)
{
  int s;
  struct socket *sp;
  struct lwip_sock *sock;
  struct lwip_select_cb *scb;
  int last_select_cb_ctr;
  SYS_ARCH_DECL_PROTECT(lev);

  LWIP_UNUSED_ARG(len);

  /* Get socket */
  if (conn) {
    s = conn->socket;
    if (s < 0) {
      /* Data comes in right away after an accept, even though
       * the server task might not have created a new socket yet.
       * Just count down (or up) if that's the case and we
       * will use the data later. Note that only receive events
       * can happen before the new socket is set up. */
      SYS_ARCH_PROTECT(lev);
      if (conn->socket < 0) {
        if (evt == NETCONN_EVT_RCVPLUS) {
          conn->socket--;
        }
        SYS_ARCH_UNPROTECT(lev);
        return;
      }
      SYS_ARCH_UNPROTECT(lev);
    }

    sp = conn->priv;            // Get the socket structure.
    sock = sp->priv;            // And the LwIP socket state.
    if (!sock) {
      return;
    }
  } else {
    return;
  }

  SYS_ARCH_PROTECT(lev);
  /* Set event as required */
  switch (evt) {
    case NETCONN_EVT_RCVPLUS:
      sock->rcvevent++;
      break;
    case NETCONN_EVT_RCVMINUS:
      sock->rcvevent--;
      break;
    case NETCONN_EVT_SENDPLUS:
      sock->sendevent = 1;
      break;
    case NETCONN_EVT_SENDMINUS:
      sock->sendevent = 0;
      break;
    case NETCONN_EVT_ERROR:
      sock->errevent = 1;
      break;
    default:
      LWIP_ASSERT("unknown event", 0);
      break;
  }

  if (sock->select_waiting == 0) {
    /* noone is waiting for this socket, no need to check select_cb_list */
    SYS_ARCH_UNPROTECT(lev);
    return;
  }

#if RICH
  /* Now decide if anyone is waiting for this socket */
  /* NOTE: This code goes through the select_cb_list list multiple times
     ONLY IF a select was actually waiting. We go through the list the number
     of waiting select calls + 1. This list is expected to be small. */

  /* At this point, SYS_ARCH is still protected! */
again:
  for (scb = select_cb_list; scb != NULL; scb = scb->next) {
    /* remember the state of select_cb_list to detect changes */
    last_select_cb_ctr = select_cb_ctr;
    if (scb->sem_signalled == 0) {
      /* semaphore not signalled yet */
      int do_signal = 0;
      /* Test this select call for our socket */
      if (sock->rcvevent > 0) {
        if (scb->readset && FD_ISSET(s, scb->readset)) {
          do_signal = 1;
        }
      }
      if (sock->sendevent != 0) {
        if (!do_signal && scb->writeset && FD_ISSET(s, scb->writeset)) {
          do_signal = 1;
        }
      }
      if (sock->errevent != 0) {
        if (!do_signal && scb->exceptset && FD_ISSET(s, scb->exceptset)) {
          do_signal = 1;
        }
      }
      if (do_signal) {
        scb->sem_signalled = 1;
        /* Don't call SYS_ARCH_UNPROTECT() before signaling the semaphore, as this might
           lead to the select thread taking itself off the list, invalidating the semaphore. */
        sys_sem_signal(SELECT_SEM_PTR(scb->sem));
      }
    }
    /* unlock interrupts with each step */
    SYS_ARCH_UNPROTECT(lev);
    /* this makes sure interrupt protection time is short */
    SYS_ARCH_PROTECT(lev);
    if (last_select_cb_ctr != select_cb_ctr) {
      /* someone has changed select_cb_list, restart at the beginning */
      goto again;
    }
  }
#endif
  SYS_ARCH_UNPROTECT(lev);
}

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

static struct lwip_sock *alloc_socket(struct netconn *conn, int accepted)
{
  struct lwip_sock *lwsp = kmem_alloc(sizeof(struct lwip_sock));
  if (lwsp == NULL) {
    return NULL;
  }

  // Initialize the socket state.
  *lwsp = (struct lwip_sock){
    .conn = conn,
    .lastdata = NULL,
    .lastoffset = 0,
    .rcvevent = 0,
    .sendevent = (NETCONNTYPE_GROUP(conn->type) == NETCONN_TCP ?
                                    (accepted != 0) : 1),
    .errevent = 0,
    .err = 0,
    .select_waiting = 0,
  };

  return lwsp;
}

static int setup(vnode_t vp)
{
  struct socket *sp = vp->v_data;
  struct netconn *conn;
  int domain = sp->domain;
  int type = sp->type;
  int protocol = sp->protocol;

  if (domain != AF_INET && domain != AF_INET6) {
    return -EAFNOSUPPORT;
  }

  // Create a netconn.
  int netconn_type;
  switch (sp->type) {
  case SOCK_RAW:
    netconn_type = DOMAIN_TO_NETCONN_TYPE(domain, NETCONN_RAW);
    conn = netconn_new_with_proto_and_callback(netconn_type,
                                               protocol, event_callback);
    DPRINTF(NETDB_INET, ("lwip_socket(%s, SOCK_RAW, %d) = ",
                         domain == PF_INET ? "PF_INET" : "UNKNOWN",
                         protocol));
    break;
  case SOCK_DGRAM:
    netconn_type = DOMAIN_TO_NETCONN_TYPE(domain,
      (protocol == IPPROTO_UDPLITE) ? NETCONN_UDPLITE : NETCONN_UDP);
    conn = netconn_new_with_callback(netconn_type, event_callback);
    DPRINTF(NETDB_INET, ("lwip_socket(%s, SOCK_DGRAM, %d) = ",
                          domain == PF_INET ? "PF_INET" : "UNKNOWN",
                          protocol));
    break;
  case SOCK_STREAM:
    netconn_type = DOMAIN_TO_NETCONN_TYPE(domain, NETCONN_TCP);
    conn = netconn_new_with_callback(netconn_type, event_callback);
    DPRINTF(NETDB_INET, ("lwip_socket(%s, SOCK_STREAM, %d) = ",
                         domain == PF_INET ? "PF_INET" : "UNKNOWN", protocol));
    if (conn != NULL) {
      // Prevent automatic window updates, we do this on our own!
      netconn_set_noautorecved(conn, 1);
    }
    break;
  default:
    DPRINTF(NETDB_INET, ("lwip_socket(%d, %d/UNKNOWN, %d) = -1\n",
                         domain, type, protocol));
    return -EOPNOTSUPP;
  }

  if (!conn) {
    DPRINTF(NETDB_INET, ("-1 / ENOBUFS (could not create netconn)\n"));
    return -ENOBUFS;
  }

  sp->priv = alloc_socket(conn, 0);
  if (sp->priv == NULL) {
    netconn_delete(conn);
    return -ENOMEM;
  }

  conn->priv = sp;              // Save the socket information.
  return 0;
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
  struct sockaddr_in iaddr;
  if (addrlen > sizeof(iaddr) || addrlen < sizeof(sa_family_t)) {
    return -EINVAL;
  }

  int s = copyin(addr, &iaddr, addrlen);
  if (s != 0)
    return s;

  if (iaddr.sin_family != AF_UNIX) {
    return -EINVAL;
  }

  if (addrlen == sizeof(sa_family_t)) {
    // Unnamed address doesn't make sense here.
    return -EINVAL;
  }

  return -ENOPROTOOPT;
}

/** Send bytes to a connection.
 */
static ssize_t net_send(struct socket *sp, char *buffer, size_t size,
                        int nonblock)
{
  return 0;
}

/** Get bytes from a connection.
 */
static ssize_t net_recv(struct socket *sp, char *buffer, size_t size,
                        int nonblock)
{
  return 0;
}

static int doclose(file_t fp)
{
  struct socket *sp = fp->f_vnode->v_data;
  struct lwip_sock *lwsp = sp->priv;

  return 0;
}

static const struct domain_interface interface = {
  .setup = setup,
  .getopt = getopt,
  .setopt = setopt,
  .option_update = option_update,
  .bind = bindaddr,
  .send = net_send,
  .receive = net_recv,
  .close = doclose,
  .vnops = &vnops,
};

// Start up LwIP.
C_CONSTRUCTOR()
{
  tcpip_init(0, 0);
}

