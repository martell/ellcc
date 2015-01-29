/*
 * Copyright (c) 2014-2015 Richard Pennington.
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

#include <netinet/in.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>

#include "config.h"
#include "kernel.h"
#include "thread.h"
#include "kmem.h"
#include "network.h"
#include "command.h"
#include "lwip/lwip_interface.h"        // This is an ELK include file.
#include "lwip/tcpip.h"
#include "lwip/ip.h"
#include "lwip/udp.h"
#include "lwip/raw.h"

// Originally from lwip/ip4_addr.h.
#undef ip_addr_debug_print
#define ip_addr_debug_print(debug, ipaddr) \
  DPRINTF(debug, ("%" U16_F ".%" U16_F ".%" U16_F ".%" U16_F,      \
                  ipaddr != NULL ? ip4_addr1_16(ipaddr) : 0,       \
                  ipaddr != NULL ? ip4_addr2_16(ipaddr) : 0,       \
                  ipaddr != NULL ? ip4_addr3_16(ipaddr) : 0,       \
                  ipaddr != NULL ? ip4_addr4_16(ipaddr) : 0))

// Originally from lwip/ip4_addr.h.
#undef ip6_addr_debug_print
#define ip6_addr_debug_print(debug, ipaddr) \
  DPRINTF(debug, ("%" X16_F ":%" X16_F ":%" X16_F ":%" X16_F       \
                  ":%" X16_F ":%" X16_F ":%" X16_F ":%" X16_F,     \
                  ipaddr != NULL ? IP6_ADDR_BLOCK1(ipaddr) : 0,    \
                  ipaddr != NULL ? IP6_ADDR_BLOCK2(ipaddr) : 0,    \
                  ipaddr != NULL ? IP6_ADDR_BLOCK3(ipaddr) : 0,    \
                  ipaddr != NULL ? IP6_ADDR_BLOCK4(ipaddr) : 0,    \
                  ipaddr != NULL ? IP6_ADDR_BLOCK5(ipaddr) : 0,    \
                  ipaddr != NULL ? IP6_ADDR_BLOCK6(ipaddr) : 0,    \
                  ipaddr != NULL ? IP6_ADDR_BLOCK7(ipaddr) : 0,    \
                  ipaddr != NULL ? IP6_ADDR_BLOCK8(ipaddr) : 0))

// Originally from lwip/inet.h.
#define inet_addr_to_ipaddr(target_ipaddr, source_inaddr) \
  (ip4_addr_set_u32(target_ipaddr, (source_inaddr)->s_addr))
#define inet_addr_from_ipaddr(target_inaddr, source_ipaddr) \
  ((target_inaddr)->s_addr = ip4_addr_get_u32(source_ipaddr))

// Originally from lwip/sockets.h.
#define SIN_ZERO_LEN 8

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

/** A struct sockaddr replacement that has the same alignment as sockaddr_in/
 *  sockaddr_in6 if instantiated.
 */
union sockaddr_aligned {
  struct sockaddr sa;
#if LWIP_IPV6
  struct sockaddr_in6 sin6;
#endif /* LWIP_IPV6 */
  struct sockaddr_in sin;
};

#define ERR_TO_ERRNO_TABLE_SIZE \
  (sizeof(err_to_errno_table)/sizeof(err_to_errno_table[0]))

#define err_to_errno(err) \
  ((unsigned)(-(err)) < ERR_TO_ERRNO_TABLE_SIZE ? \
    err_to_errno_table[-(err)] : EIO)

#define IP4ADDR_PORT_TO_SOCKADDR(sin, ipXaddr, port) do { \
  (sin)->sin_family = AF_INET; \
  (sin)->sin_port = htons((port)); \
  inet_addr_from_ipaddr(&(sin)->sin_addr, ipX_2_ip(ipXaddr)); \
  memset((sin)->sin_zero, 0, SIN_ZERO_LEN); }while(0)
#define SOCKADDR4_TO_IP4ADDR_PORT(sin, ipXaddr, port) do { \
  inet_addr_to_ipaddr(ipX_2_ip(ipXaddr), &((sin)->sin_addr)); \
  (port) = ntohs((sin)->sin_port); }while(0)

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

#define IS_SOCK_ADDR_TYPE_VALID_OR_UNSPEC(name) \
 (((name)->sa_family == AF_UNSPEC) || \
 IS_SOCK_ADDR_TYPE_VALID(name))
#define SOCK_ADDR_TYPE_MATCH_OR_UNSPEC(name, sock) \
  (((name)->sa_family == AF_UNSPEC) || \
  SOCK_ADDR_TYPE_MATCH(name, sock))
#define IS_SOCK_ADDR_ALIGNED(name) \
  ((((mem_ptr_t)(name)) % 4) == 0)

#define LWIP_SETGETSOCKOPT_DATA_VAR_REF(name) API_VAR_REF(name)
#define LWIP_SETGETSOCKOPT_DATA_VAR_DECLARE(name) \
  API_VAR_DECLARE(struct lwip_setgetsockopt_data, name)
#define LWIP_SETGETSOCKOPT_DATA_VAR_FREE(name) \
  API_VAR_FREE(MEMP_SOCKET_SETGETSOCKOPT_DATA, name)

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

static int setup(vnode_t vp, int flags)
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

  if (flags & SOCK_NONBLOCK) {
    netconn_set_nonblocking(conn, 1);
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
  if (level != IPPROTO_IP) {
    return -EINVAL;
  }

  switch (optname) {
  case IP_ADD_MEMBERSHIP:
  case IP_ADD_SOURCE_MEMBERSHIP:
  case IP_BLOCK_SOURCE:
  case IP_DROP_MEMBERSHIP:
  case IP_DROP_SOURCE_MEMBERSHIP:
  case IP_FREEBIND:
  case IP_HDRINCL:
  case IP_MSFILTER:
  case IP_MTU:
  case IP_MTU_DISCOVER:
  case IP_MULTICAST_ALL:
  case IP_MULTICAST_IF:
  case IP_MULTICAST_LOOP:
  case IP_MULTICAST_TTL:
  case IP_NODEFRAG:
  case IP_OPTIONS:
  case IP_PKTINFO:
  case IP_RECVERR:
  case IP_RECVOPTS:
  case IP_RECVORIGDSTADDR:
  case IP_RECVTOS:
  case IP_RECVTTL:
  case IP_RETOPTS:
  case IP_ROUTER_ALERT:
  case IP_TOS:
  case IP_TRANSPARENT:
  case IP_TTL:
  case IP_UNBLOCK_SOURCE:
  default:
    return -ENOPROTOOPT;
  }
}

static int setopt(file_t fp, int level, int optname, const void *optval,
                      socklen_t optlen)
{
  if (level != IPPROTO_IP) {
    return -EINVAL;
  }

  switch (optname) {
  case IP_ADD_MEMBERSHIP:
  case IP_ADD_SOURCE_MEMBERSHIP:
  case IP_BLOCK_SOURCE:
  case IP_DROP_MEMBERSHIP:
  case IP_DROP_SOURCE_MEMBERSHIP:
  case IP_FREEBIND:
  case IP_HDRINCL:
  case IP_MSFILTER:
  case IP_MTU:
  case IP_MTU_DISCOVER:
  case IP_MULTICAST_ALL:
  case IP_MULTICAST_IF:
  case IP_MULTICAST_LOOP:
  case IP_MULTICAST_TTL:
  case IP_NODEFRAG:
  case IP_OPTIONS:
  case IP_PKTINFO:
  case IP_RECVERR:
  case IP_RECVOPTS:
  case IP_RECVORIGDSTADDR:
  case IP_RECVTOS:
  case IP_RECVTTL:
  case IP_RETOPTS:
  case IP_ROUTER_ALERT:
  case IP_TOS:
  case IP_TRANSPARENT:
  case IP_TTL:
  case IP_UNBLOCK_SOURCE:
  default:
    return -ENOPROTOOPT;
  }
}

static int option_update(file_t fp)
{
  return 0;
}

// The list of available interfaces.
struct interface {
  char *name;                   // The interface name.
  int flags;                    // Interface flags.
  struct netif *netif;          // The LwIP interface structure.
  netif_init_fn init;           // The initialization function.
  netif_input_fn input;         // The input function.
  void *state;                  // The device state.
};

// The available interfaces.
static pthread_mutex_t interface_lock = PTHREAD_MUTEX_INITIALIZER;
static struct interface interfaces[CONFIG_NET_MAX_INET_INTERFACES];

/** Add an interface to the interface list.
 */
int lwip_add_interface(const char *name, netif_init_fn init,
                       netif_input_fn input, void *state)
{
  // interfaces[0] is reserved for the loopback interface.
  for (int i = 1; i < CONFIG_NET_MAX_INET_INTERFACES; ++i) {
    if (interfaces[i].name == NULL) {
      // Use this entry.
      int len = strlen(name) + 1;
      interfaces[i].name = kmem_alloc(len);
      if (interfaces[i].name == NULL) {
        return -ENOMEM;
      }
      strcpy(interfaces[i].name, name);
      interfaces[i].flags = 0;
      interfaces[i].netif = NULL;
      interfaces[i].init = init;
      interfaces[i].input = input;
      interfaces[i].state = state;
      return 0;
    }
  }

  return -EAGAIN;
}

static int inet_ioctl(file_t fp, u_long request, void *buf)
{
  int s;                        // Call status.
  int index;                    // The interface index.
  struct interface *interface;  // The interface.
  struct netif *netif;          // The LwIP interface.
  struct ifreq req;             // For a potential request.
  err_t err;                    // LwIP error return value.

// Get the request struct.
#define GET_REQUEST()                                           \
  s = copyin(buf, &req, sizeof(req));                           \
  if (s) break;

// Return the request struct.
#define SET_REQUEST()                                           \
  s = copyout(&req, buf, sizeof(req));                          \
  if (s) break;

// Look up the interface name.
#define GET_INDEX()                                             \
  for (index = 0; index < CONFIG_NET_MAX_INET_INTERFACES; ++index) { \
    interface = &interfaces[index];                             \
    if (interface->name == NULL) {                              \
      continue;                                                 \
    }                                                           \
    interface->name[IFNAMSIZ - 1] = '\0';                       \
    if (strcmp(interface->name, req.ifr_name) == 0) {           \
      break;                                                    \
    }                                                           \
  }                                                             \
  if (index >= CONFIG_NET_MAX_INET_INTERFACES) {                \
    s = -ENODEV;                                                \
    break;                                                      \
  }

// Does the requester have net admin priveleges?
#define IS_ADMIN()                                              \
  if (!capable(CAP_NET_ADMIN)) {                                \
    s = -EPERM;                                                 \
    break;                                                      \
  }

// Create the netif if it doesn't exist.
#define CHECK_NETIF()                                           \
  if (interface->netif == NULL) {                               \
    netif = kmem_alloc(sizeof(struct netif));                   \
    if (netif == NULL) {                                        \
      s = -ENOMEM;                                              \
      break;                                                    \
    }                                                           \
    err = netifapi_netif_add(netif, NULL, NULL, NULL,           \
                             interface->state,                  \
                             interface->init,                   \
                             interface->input);                 \
    if (err != ERR_OK) {                                        \
      kmem_free(netif);                                         \
      s = -err_to_errno(err);                                   \
      break;                                                    \
    }                                                           \
    interface->netif = netif;                                   \
  } else {                                                      \
    netif = interface->netif;                                   \
  }

#define UPDATE_FLAGS()                                          \
    if (netif->flags & NETIF_FLAG_UP)                           \
      interface->flags |= IFF_UP;                               \
    if (netif->flags & NETIF_FLAG_BROADCAST)                    \
      interface->flags |= IFF_BROADCAST;                        \
    if (netif->flags & NETIF_FLAG_LINK_UP)                      \
      interface->flags |= IFF_RUNNING;                          \
    if (!(netif->flags & NETIF_FLAG_ETHARP))                    \
      interface->flags |= IFF_NOARP;                            \
    if (netif->flags & NETIF_FLAG_POINTTOPOINT)                 \
      interface->flags |= IFF_POINTOPOINT;

#define WHEN_CHANGED(flag, set, cleared)                        \
      if (changed & (flag)) {                                   \
        if (req.ifr_flags & (flag)) {                           \
          set;                                                  \
        } else {                                                \
          cleared;                                              \
        }                                                       \
      }

  pthread_mutex_lock(&interface_lock);
  switch (request) {
  case SIOCGIFNAME:     // Return the interface name.
    GET_REQUEST();
    if (req.ifr_ifindex < 0 ||
       req.ifr_ifindex >= CONFIG_NET_MAX_INET_INTERFACES) {
      return -EINVAL;
    }
    if (interfaces[req.ifr_ifindex].name == NULL) {
      // Interface not found.
      s = -ENODEV;
      break;
    }

    strcpy(req.ifr_name, interfaces[req.ifr_ifindex].name);
    SET_REQUEST();
    break;

  case SIOCGIFINDEX:    // Return the interface index.
    GET_REQUEST();
    GET_INDEX();
    // Found.
    req.ifr_ifindex = index;
    SET_REQUEST();
    break;

  case SIOCGIFFLAGS:
    GET_REQUEST();
    GET_INDEX();
    CHECK_NETIF();
    UPDATE_FLAGS();
    // RICH: Can we get additional flags via device status?
    req.ifr_flags = interface->flags;
    SET_REQUEST();
    break;

  case SIOCSIFFLAGS:
    IS_ADMIN();
    GET_REQUEST();
    GET_INDEX();
    CHECK_NETIF();
    UPDATE_FLAGS();
    // Act on any flag changes.
    int changed = req.ifr_flags ^ interface->flags;
    // Update the driver with any flags that have changed.
    WHEN_CHANGED(IFF_UP, netifapi_netif_set_up(netif),
                 netifapi_netif_set_down(netif));
    // RICH: Additional flags?
    UPDATE_FLAGS();
    interface->flags = req.ifr_flags;
    break;

  case SIOCGIFPFLAGS:           // These don't seem to be defined by Linux.
  case SIOCSIFPFLAGS:
    s = -EOPNOTSUPP;
    break;

  case SIOCGIFADDR:
    GET_REQUEST();
    GET_INDEX();
    CHECK_NETIF();
    // Get the interface address.
    req.ifr_addr.sa_family = AF_INET;
    memcpy(req.ifr_addr.sa_data, &netif->ip_addr, sizeof(netif->ip_addr));
    SET_REQUEST();
    break;

  case SIOCSIFADDR:
    IS_ADMIN();
    GET_REQUEST();
    GET_INDEX();
    if (req.ifr_addr.sa_family != AF_INET) {
      s = -EINVAL;
      break;
    }

    CHECK_NETIF();
    // Set the interface address.
    err = netifapi_netif_set_addr(netif, (ip_addr_t *)&req.ifr_addr.sa_data,
                                  &netif->netmask, &netif->gw);
    if (err != ERR_OK)
      s = -err_to_errno(err);
    break;

  case SIOCSIFDSTADDR:
    s = -EOPNOTSUPP;
    break;

  case SIOCGIFBRDADDR:
  case SIOCSIFBRDADDR:
    s = -EOPNOTSUPP;
    break;

  case SIOCGIFNETMASK:
    GET_REQUEST();
    GET_INDEX();
    CHECK_NETIF();
    // Get the interface netbask.
    req.ifr_netmask.sa_family = AF_INET;
    memcpy(req.ifr_netmask.sa_data, &netif->netmask, sizeof(netif->netmask));
    SET_REQUEST();
    break;

  case SIOCSIFNETMASK:
    IS_ADMIN();
    GET_REQUEST();
    GET_INDEX();
    if (req.ifr_netmask.sa_family != AF_INET) {
      s = -EINVAL;
      break;
    }

    CHECK_NETIF();
    // Set the interface netbask.
    err = netifapi_netif_set_addr(netif, &netif->ip_addr,
                                  (ip_addr_t *)&req.ifr_addr.sa_data,
                                  &netif->gw);
    if (err != ERR_OK)
      s = -err_to_errno(err);
    break;

  case SIOCGIFMETRIC:
    GET_REQUEST();
    GET_INDEX();
    CHECK_NETIF();
    req.ifr_metric = 0;
    SET_REQUEST();
    break;

  case SIOCSIFMETRIC:
    s = -EOPNOTSUPP;
    break;

  case SIOCGIFMTU:
    GET_REQUEST();
    GET_INDEX();
    CHECK_NETIF();
    req.ifr_mtu = netif->mtu;
    SET_REQUEST();
    break;

  case SIOCSIFMTU:
    IS_ADMIN();
    GET_REQUEST();
    GET_INDEX();
    CHECK_NETIF();
    netif->mtu = req.ifr_mtu;
    break;

  case SIOCGIFHWADDR:
    GET_REQUEST();
    GET_INDEX();
    CHECK_NETIF();
    req.ifr_hwaddr.sa_family = ARPHRD_ETHER;
    memcpy(req.ifr_hwaddr.sa_data, &netif->hwaddr, netif->hwaddr_len);
    SET_REQUEST();
    break;

  case SIOCSIFHWADDR:
    IS_ADMIN();
    GET_REQUEST();
    GET_INDEX();
    if (req.ifr_netmask.sa_family != ARPHRD_ETHER) {
      s = -EINVAL;
      break;
    }

    CHECK_NETIF();
    // Set the interface MAC address.
    netif->hwaddr_len = NETIF_MAX_HWADDR_LEN;
    memcpy(&netif->hwaddr, req.ifr_hwaddr.sa_data, netif->hwaddr_len);
    break;

  case SIOCSIFHWBROADCAST:
  case SIOCGIFMAP:
  case SIOCSIFMAP:
  case SIOCADDMULTI:
  case SIOCDELMULTI:
  case SIOCGIFTXQLEN:
  case SIOCSIFTXQLEN:
  case SIOCSIFNAME:
  case SIOCGIFCONF:
  default:
    s = -EOPNOTSUPP;
    break;
  }

  pthread_mutex_unlock(&interface_lock);
  return s;
}

static int inet_bind(file_t fp, struct sockaddr *addr, socklen_t addrlen)
{
  struct socket *sp = fp->f_vnode->v_data;
  struct lwip_sock *lwsp = sp->priv;
  ipX_addr_t local_addr;
  u16_t local_port;
  err_t err;


  if (!SOCK_ADDR_TYPE_MATCH(addr, lwsp)) {
    // Sockaddr does not match socket type (IPv4/IPv6).
    return -EINVAL;
  }

  // Check size, family and alignment of 'addr'.
  if (!(IS_SOCK_ADDR_LEN_VALID(addrlen) &&
        IS_SOCK_ADDR_TYPE_VALID(addr) && IS_SOCK_ADDR_ALIGNED(addr))) {
    // Invalid address.
    return -EINVAL;
  }

  LWIP_UNUSED_ARG(addrlen);

  SOCKADDR_TO_IPXADDR_PORT((addr->sa_family == AF_INET6), addr, &local_addr,
                           local_port);
  DPRINTF(NETDB_INET, ("inet_bind(addr="));
  ipX_addr_debug_print(addr->sa_family == AF_INET6, NETDB_INET, &local_addr);
  DPRINTF(NETDB_INET, (" port=%"U16_F")\n", local_port));

  err = netconn_bind(lwsp->conn, ipX_2_ip(&local_addr), local_port);

  if (err != ERR_OK) {
    DPRINTF(NETDB_INET, ("inet_bind() failed, err=%d\n", err));
    return -err_to_errno(err);
  }

  DPRINTF(NETDB_INET, ("inet_bind() succeeded\n"));
  return 0;
}

/** Set a socket into listen mode.
 * The socket may not have been used for another connection previously.
 *
 * @param s the socket to set to listening mode
 * @param backlog (ATTENTION: needs TCP_LISTEN_BACKLOG=1)
 * @return 0 on success, non-zero on failure
 */
static int inet_listen(struct socket *sp, int backlog)
{
  struct lwip_sock *lwsp = sp->priv;
  err_t err;

  DPRINTF(NETDB_INET, ("lwip_listen(backlog=%d)\n", backlog));

  // Limit the "backlog" parameter to fit in an u8_t.
  backlog = LWIP_MIN(LWIP_MAX(backlog, 0), 0xff);

  err = netconn_listen_with_backlog(lwsp->conn, (u8_t)backlog);

  if (err != ERR_OK) {
    DPRINTF(NETDB_INET, ("lwip_listen() failed, err=%d\n", err));
    if (NETCONNTYPE_GROUP(netconn_type(lwsp->conn)) != NETCONN_TCP) {
      return -EOPNOTSUPP;
    }
    return -err_to_errno(err);
  }

  return 0;
}

static int inet_connect(struct socket *sp, const struct sockaddr *addr,
                        socklen_t addrlen)
{
  struct lwip_sock *lwsp = sp->priv;
  err_t err;

  if (!SOCK_ADDR_TYPE_MATCH_OR_UNSPEC(addr, lwsp)) {
    // The sockaddr does not match socket type (IPv4/IPv6).
    return -EINVAL;
  }

  LWIP_UNUSED_ARG(addrlen);
  if (addr->sa_family == AF_UNSPEC) {
    DPRINTF(NETDB_INET, ("lwip_connect(AF_UNSPEC)\n"));
    err = netconn_disconnect(lwsp->conn);
  } else {
    ipX_addr_t remote_addr;
    u16_t remote_port;

    /* check size, family and alignment of 'addr' */
    if (!(IS_SOCK_ADDR_LEN_VALID(addrlen) &&
          IS_SOCK_ADDR_TYPE_VALID_OR_UNSPEC(addr) &&
          IS_SOCK_ADDR_ALIGNED(addr))) {
      // Invalid address.
      return -EINVAL;
    }

    SOCKADDR_TO_IPXADDR_PORT((addr->sa_family == AF_INET6), addr, &remote_addr,
                             remote_port);
    DPRINTF(NETDB_INET, ("lwip_connect(addr="));
    ipX_addr_debug_print(addr->sa_family == AF_INET6, NETDB_INET, &remote_addr);
    DPRINTF(NETDB_INET, (" port=%"U16_F")\n", remote_port));

    err = netconn_connect(lwsp->conn, ipX_2_ip(&remote_addr), remote_port);
  }

  if (err != ERR_OK) {
    DPRINTF(NETDB_INET, ("lwip_connect() failed, err=%d\n", err));
    return -err_to_errno(err);
  }

  DPRINTF(NETDB_INET, ("lwip_connect() succeeded\n"));
  return 0;
}

static int inet_accept4(struct socket *sp, struct socket *newsp,
                        struct sockaddr *addr, socklen_t *addrlen, int flags)
{
  struct lwip_sock *lwsp = sp->priv;
  struct netconn *newconn;
  ipX_addr_t naddr;
  u16_t port = 0;
  err_t err;

  DPRINTF(NETDB_INET, ("lwip_accept()...\n"));

  if (netconn_is_nonblocking(lwsp->conn) && (lwsp->rcvevent <= 0)) {
    DPRINTF(NETDB_INET, ("lwip_accept(): returning EWOULDBLOCK\n"));
    return -EWOULDBLOCK;
  }

  // Wait for a new connection.
  err = netconn_accept(lwsp->conn, &newconn);
  if (err != ERR_OK) {
    DPRINTF(NETDB_INET, ("lwip_accept(): netconn_acept failed, err=%d\n", err));
    if (NETCONNTYPE_GROUP(netconn_type(lwsp->conn)) != NETCONN_TCP) {
      return -EOPNOTSUPP;
    }
    return -err_to_errno(err);
  }

  ASSERT(newconn != NULL);
  // Prevent automatic window updates, we do this on our own!
  netconn_set_noautorecved(newconn, 1);

  /* Note that POSIX only requires us to check addr is non-NULL. addrlen must
   * not be NULL if addr is valid.
   */
  if (addr != NULL) {
    union sockaddr_aligned tempaddr;
    // Get the IP address and port of the remote host.
    err = netconn_peer(newconn, ipX_2_ip(&naddr), &port);
    if (err != ERR_OK) {
      DPRINTF(NETDB_INET, ("lwip_accept(): netconn_peer failed, err=%d\n",
                           err));
      netconn_delete(newconn);
      return -err_to_errno(err);
    }
    ASSERT(addrlen != NULL);

    IPXADDR_PORT_TO_SOCKADDR(NETCONNTYPE_ISIPV6(newconn->type), &tempaddr,
                                                &naddr, port);
    MEMCPY(addr, &tempaddr, *addrlen);
  }

  struct lwip_sock *nlwsp = alloc_socket(newconn, 1);
  if (nlwsp == NULL) {
    netconn_delete(newconn);
    return -ENOMEM;
  }

  newsp->priv = nlwsp;

  /* See event_callback: If data comes in right away after an accept, even
   * though the server task might not have created a new socket yet.
   * In that case, newconn->socket is counted down (newconn->socket--),
   * so nsock->rcvevent is >= 1 here!
   */
  SYS_ARCH_PROTECT(lev);
  nlwsp->rcvevent += (s16_t)(-1 - newconn->socket);
  newconn->priv = newsp;
  SYS_ARCH_UNPROTECT(lev);

  if (addr != NULL) {
    DPRINTF(NETDB_INET, (" addr="));
    ipX_addr_debug_print(NETCONNTYPE_ISIPV6(newconn->type), NETDB_INET, &naddr);
    DPRINTF(NETDB_INET, (" port=%"U16_F"\n", port));
  }

  return 0;
}

static ssize_t inet_sendto(struct socket *sp, const char *buffer, size_t size,
                           int flags, const struct sockaddr *to,
                           socklen_t tolen)
{
  struct lwip_sock *lwsp = sp->priv;
  int err;
  u16_t short_size;
  int remote_port;
#if !LWIP_TCPIP_CORE_LOCKING
  struct netbuf buf;
#endif

  if (NETCONNTYPE_GROUP(netconn_type(lwsp->conn)) == NETCONN_TCP) {
#if LWIP_TCP
    int write_flags = NETCONN_COPY |
      ((flags & MSG_MORE)     ? NETCONN_MORE      : 0) |
      ((flags & MSG_DONTWAIT) ? NETCONN_DONTBLOCK : 0);
    size_t written = 0;
    int s = netconn_write_partly(lwsp->conn, buffer, size, write_flags,
                                 &written);

    DPRINTF(NETDB_INET, ("lwip_send() err=%d written=%"SZT_F"\n", s, written));
    return (s == ERR_OK ? written : -err_to_errno(s));
#else /* LWIP_TCP */
    LWIP_UNUSED_ARG(flags);
    return -EIO;
#endif /* LWIP_TCP */
  }

  if ((to != NULL) && !SOCK_ADDR_TYPE_MATCH(to, lwsp)) {
    // The sockaddr does not match the socket type (IPv4/IPv6).
    return -EINVAL;
  }

  // @todo: split into multiple sendto's?
  LWIP_ASSERT("lwip_sendto: size must fit in u16_t", size <= 0xffff);
  short_size = (u16_t)size;
  if (!(((to == NULL) && (tolen == 0)) ||
        (IS_SOCK_ADDR_LEN_VALID(tolen) &&
         IS_SOCK_ADDR_TYPE_VALID(to) && IS_SOCK_ADDR_ALIGNED(to)))) {
    // An invalid address was given.
    return -EINVAL;
  }

#if LWIP_TCPIP_CORE_LOCKING
  /** Special speedup for fast UDP/RAW sending: call the raw API directly
   * instead of using the netconn functions.
   */
  {
    struct pbuf* p;
    ipX_addr_t *remote_addr;
    ipX_addr_t remote_addr_tmp;

#if LWIP_NETIF_TX_SINGLE_PBUF
    p = pbuf_alloc(PBUF_TRANSPORT, short_size, PBUF_RAM);
    if (p != NULL) {
#if LWIP_CHECKSUM_ON_COPY
      u16_t chksum = 0;
      if (NETCONNTYPE_GROUP(netconn_type(lwsp->conn)) != NETCONN_RAW) {
        chksum = LWIP_CHKSUM_COPY(p->payload, buffer, short_size);
      } else
#endif /* LWIP_CHECKSUM_ON_COPY */
      MEMCPY(p->payload, buffer, size);
#else /* LWIP_NETIF_TX_SINGLE_PBUF */
    p = pbuf_alloc(PBUF_TRANSPORT, short_size, PBUF_REF);
    if (p != NULL) {
      p->payload = (void*)buffer;
#endif /* LWIP_NETIF_TX_SINGLE_PBUF */

      if (to != NULL) {
        SOCKADDR_TO_IPXADDR_PORT(to->sa_family == AF_INET6,
          to, &remote_addr_tmp, remote_port);
        remote_addr = &remote_addr_tmp;
      } else {
        remote_addr = &lwsp->conn->pcb.ip->remote_ip;
#if LWIP_UDP
        if (NETCONNTYPE_GROUP(lwsp->conn->type) == NETCONN_UDP) {
          remote_port = lwsp->conn->pcb.udp->remote_port;
        } else
#endif /* LWIP_UDP */
        {
          remote_port = 0;
        }
      }

      LOCK_TCPIP_CORE();
      if (NETCONNTYPE_GROUP(netconn_type(lwsp->conn)) == NETCONN_RAW) {
#if LWIP_RAW
        err = lwsp->conn->last_err = raw_sendto(lwsp->conn->pcb.raw, p,
                                                ipX_2_ip(remote_addr));
#else /* LWIP_RAW */
        err = ERR_ARG;
#endif /* LWIP_RAW */
      }
#if LWIP_UDP && LWIP_RAW
      else
#endif /* LWIP_UDP && LWIP_RAW */
      {
#if LWIP_UDP
#if LWIP_CHECKSUM_ON_COPY && LWIP_NETIF_TX_SINGLE_PBUF
        err = lwsp->conn->last_err = udp_sendto_chksum(lwsp->conn->pcb.udp, p,
          ipX_2_ip(remote_addr), remote_port, 1, chksum);
#else /* LWIP_CHECKSUM_ON_COPY && LWIP_NETIF_TX_SINGLE_PBUF */
        err = lwsp->conn->last_err = udp_sendto(lwsp->conn->pcb.udp, p,
          ipX_2_ip(remote_addr), remote_port);
#endif /* LWIP_CHECKSUM_ON_COPY && LWIP_NETIF_TX_SINGLE_PBUF */
#else /* LWIP_UDP */
        err = ERR_ARG;
#endif /* LWIP_UDP */
      }
      UNLOCK_TCPIP_CORE();
      pbuf_free(p);
    } else {
      err = ERR_MEM;
    }
  }
#else /* LWIP_TCPIP_CORE_LOCKING */
  /* initialize a buffer */
  buf.p = buf.ptr = NULL;
#if LWIP_CHECKSUM_ON_COPY
  buf.flags = 0;
#endif /* LWIP_CHECKSUM_ON_COPY */
  if (to) {
    SOCKADDR_TO_IPXADDR_PORT((to->sa_family) == AF_INET6, to, &buf.addr,
                              remote_port);
  } else {
    remote_port = 0;
    ipX_addr_set_any(NETCONNTYPE_ISIPV6(netconn_type(lwsp->conn)), &buf.addr);
  }
  netbuf_fromport(&buf) = remote_port;


  DPRINTF(NETDB_INET, ("lwip_sendto(%d, buffer=%p, short_size=%"U16_F",
                       flags=0x%x to=",
              s, buffer, short_size, flags));
  ipX_addr_debug_print(NETCONNTYPE_ISIPV6(netconn_type(lwsp->conn)),
    NETDB_INET, &buf.addr);
  DPRINTF(NETDB_INET, (" port=%"U16_F"\n", remote_port));

  /* make the buffer point to the data that should be sent */
#if LWIP_NETIF_TX_SINGLE_PBUF
  /* Allocate a new netbuf and copy the data into it. */
  if (netbuf_alloc(&buf, short_size) == NULL) {
    err = ERR_MEM;
  } else {
#if LWIP_CHECKSUM_ON_COPY
    if (NETCONNTYPE_GROUP(netconn_type(lwsp->conn)) != NETCONN_RAW) {
      u16_t chksum = LWIP_CHKSUM_COPY(buf.p->payload, buffer, short_size);
      netbuf_set_chksum(&buf, chksum);
      err = ERR_OK;
    } else
#endif /* LWIP_CHECKSUM_ON_COPY */
    {
      err = netbuf_take(&buf, buffer, short_size);
    }
  }
#else /* LWIP_NETIF_TX_SINGLE_PBUF */
  err = netbuf_ref(&buf, buffer, short_size);
#endif /* LWIP_NETIF_TX_SINGLE_PBUF */
  if (err == ERR_OK) {
    /* send the buffer */
    err = netconn_send(lwsp->conn, &buf);
  }

  /* deallocated the buffer */
  netbuf_free(&buf);
#endif /* LWIP_TCPIP_CORE_LOCKING */
  return (err == ERR_OK ? short_size : -err_to_errno(err));
}

/** Get bytes from a connection.
 */
static ssize_t inet_recvfrom(struct socket *sp, char *buffer, size_t size,
                             int flags, struct sockaddr *from,
                             socklen_t *fromlen)
{
  struct lwip_sock *lwsp = sp->priv;
  void *buf = NULL;
  struct pbuf *p;
  u16_t buflen, copylen;
  int off = 0;
  u8_t done = 0;
  err_t err;

  DPRINTF(NETDB_INET, ("lwip_recvfrom(%p, %"SZT_F", 0x%x, ..)\n", buffer, size,
                       flags));

  do {
    DPRINTF(NETDB_INET, ("lwip_recvfrom: top while lwsp->lastdata=%p\n",
                         lwsp->lastdata));
    // Check if there is data left from the last recv operation.
    if (lwsp->lastdata) {
      buf = lwsp->lastdata;
    } else {
      // If this is non-blocking call, then check first.
      if (((flags & MSG_DONTWAIT) || netconn_is_nonblocking(lwsp->conn)) &&
          (lwsp->rcvevent <= 0)) {
        if (off > 0) {
          /* update receive window */
          netconn_recved(lwsp->conn, (u32_t)off);
          /* already received data, return that */
          return off;
        }

        DPRINTF(NETDB_INET, ("lwip_recvfrom(): returning EWOULDBLOCK\n"));
        return -EWOULDBLOCK;
      }

      /* No data was left from the previous operation, so we try to get
       * some from the network.
       */
      if (NETCONNTYPE_GROUP(netconn_type(lwsp->conn)) == NETCONN_TCP) {
        err = netconn_recv_tcp_pbuf(lwsp->conn, (struct pbuf **)&buf);
      } else {
        err = netconn_recv(lwsp->conn, (struct netbuf **)&buf);
      }

      DPRINTF(NETDB_INET, ("lwip_recvfrom: netconn_recv err=%d, netbuf=%p\n",
                           err, buf));

      if (err != ERR_OK) {
        if (off > 0) {
          // Update the receive window.
          netconn_recved(lwsp->conn, (u32_t)off);
          // Already received data, return that.
          return off;
        }

        // We should really do some error checking here.
        DPRINTF(NETDB_INET, ("lwip_recvfrom(): buf == NULL, error is \"%s\"!\n",
          lwip_strerr(err)));
        if (err == ERR_CLSD) {
          return 0;
        } else {
          return -err_to_errno(err);
        }
      }

      LWIP_ASSERT("buf != NULL", buf != NULL);
      lwsp->lastdata = buf;
    }

    if (NETCONNTYPE_GROUP(netconn_type(lwsp->conn)) == NETCONN_TCP) {
      p = (struct pbuf *)buf;
    } else {
      p = ((struct netbuf *)buf)->p;
    }
    buflen = p->tot_len;
    DPRINTF(NETDB_INET, ("lwip_recvfrom: buflen=%"U16_F" size=%"
                         SZT_F" off=%d sock->lastoffset=%"U16_F"\n",
                         buflen, size, off, lwsp->lastoffset));

    buflen -= lwsp->lastoffset;

    if (size > buflen) {
      copylen = buflen;
    } else {
      copylen = (u16_t)size;
    }

    /* Copy the contents of the received buffer into
     * the supplied memory pointer buffer.
     */
    pbuf_copy_partial(p, (u8_t*)buffer + off, copylen, lwsp->lastoffset);

    off += copylen;

    if (NETCONNTYPE_GROUP(netconn_type(lwsp->conn)) == NETCONN_TCP) {
      LWIP_ASSERT("invalid copylen, size would underflow", size >= copylen);
      size -= copylen;
      if ( (size <= 0) || 
           (p->flags & PBUF_FLAG_PUSH) || 
           (lwsp->rcvevent <= 0) || 
           ((flags & MSG_PEEK)!=0)) {
        done = 1;
      }
    } else {
      done = 1;
    }

    // Check to see from where the data was.
    if (done) {
#if !SOCKETS_DEBUG
      if (from && fromlen)
#endif /* !SOCKETS_DEBUG */
      {
        u16_t port;
        ipX_addr_t tmpaddr;
        ipX_addr_t *fromaddr;
        union sockaddr_aligned saddr;
        DPRINTF(NETDB_INET, ("lwip_recvfrom(): addr="));
        if (NETCONNTYPE_GROUP(netconn_type(lwsp->conn)) == NETCONN_TCP) {
          fromaddr = &tmpaddr;
          // TODO: this does not work for IPv6, yet.
          netconn_getaddr(lwsp->conn, ipX_2_ip(fromaddr), &port, 0);
        } else {
          port = netbuf_fromport((struct netbuf *)buf);
          fromaddr = netbuf_fromaddr_ipX((struct netbuf *)buf);
        }

        IPXADDR_PORT_TO_SOCKADDR(NETCONNTYPE_ISIPV6(netconn_type(lwsp->conn)),
                                 &saddr, fromaddr, port);
        ipX_addr_debug_print(NETCONNTYPE_ISIPV6(netconn_type(lwsp->conn)),
                             NETDB_INET, fromaddr);
        DPRINTF(NETDB_INET, (" port=%"U16_F" len=%d\n", port, off));
#if SOCKETS_DEBUG
        if (from && fromlen)
#endif /* SOCKETS_DEBUG */
        {
          MEMCPY(from, &saddr, *fromlen);
        }
      }
    }

    // If we don't peek the incoming message...
    if ((flags & MSG_PEEK) == 0) {
      /* If this is a TCP socket, check if there is data left in the
       * buffer. If so, it should be saved in the sock structure for next
       * time around.
       */
      if ((NETCONNTYPE_GROUP(netconn_type(lwsp->conn)) == NETCONN_TCP) &&
          (buflen - copylen > 0)) {
        lwsp->lastdata = buf;
        lwsp->lastoffset += copylen;
        DPRINTF(NETDB_INET, ("lwip_recvfrom: lastdata now netbuf=%p\n", buf));
      } else {
        lwsp->lastdata = NULL;
        lwsp->lastoffset = 0;
        DPRINTF(NETDB_INET, ("lwip_recvfrom: deleting netbuf=%p\n", buf));
        if (NETCONNTYPE_GROUP(netconn_type(lwsp->conn)) == NETCONN_TCP) {
                              pbuf_free((struct pbuf *)buf);
        } else {
          netbuf_delete((struct netbuf *)buf);
        }
      }
    }
  } while (!done);

  if ((off > 0) &&
      (NETCONNTYPE_GROUP(netconn_type(lwsp->conn)) == NETCONN_TCP)) {
    // Update the receive window.
    netconn_recved(lwsp->conn, (u32_t)off);
  }
  return off;
}

static int getaddrname(struct socket *sp, struct sockaddr *addr,
                       socklen_t *addrlen, int local)
{
  struct lwip_sock *lwsp = sp->priv;
  union sockaddr_aligned saddr;
  ipX_addr_t naddr;
  u16_t port;
  err_t err;

  /* get the IP address and port */
  /* @todo: this does not work for IPv6, yet */
  err = netconn_getaddr(lwsp->conn, ipX_2_ip(&naddr), &port, local);
  if (err != ERR_OK) {
    return -err_to_errno(err);
  }

  IPXADDR_PORT_TO_SOCKADDR(NETCONNTYPE_ISIPV6(netconn_type(lwsp->conn)),
                           &saddr, &naddr, port);

  DPRINTF(NETDB_INET, ("lwip_getaddrname((, addr="));
  ipX_addr_debug_print(NETCONNTYPE_ISIPV6(netconn_type(lwsp->conn)),
                       NETDB_INET, &naddr);
  DPRINTF(NETDB_INET, (" port=%"U16_F")\n", port));

  MEMCPY(addr, &saddr, *addrlen);
  return 0;
}

static int inet_getpeername(struct socket *sp, struct sockaddr *addr,
                            socklen_t *addrlen)
{
  return getaddrname(sp, addr, addrlen, 0);
}

static int inet_getsockname(struct socket *sp, struct sockaddr *addr,
                            socklen_t *addrlen)
{
  return getaddrname(sp, addr, addrlen, 1);
}

/** Unimplemented: Close one end of a full-duplex connection.
 * Currently, the full connection is closed.
 * RICH: Is this comment still valid?
 */
static int inet_shutdown(struct socket *sp, int how)
{
  struct lwip_sock *lwsp = sp->priv;
  err_t err;
  u8_t shut_rx = 0, shut_tx = 0;

  DPRINTF(NETDB_INET, ("lwip_shutdown(how=%d)\n", how));

  if (lwsp->conn != NULL) {
    if (NETCONNTYPE_GROUP(netconn_type(lwsp->conn)) != NETCONN_TCP) {
      return -EOPNOTSUPP;
    }
  } else {
    return -ENOTCONN;
  }

  if (how == SHUT_RD) {
    shut_rx = 1;
  } else if (how == SHUT_WR) {
    shut_tx = 1;
  } else if(how == SHUT_RDWR) {
    shut_rx = 1;
    shut_tx = 1;
  } else {
    return -EINVAL;
  }

  err = netconn_shutdown(lwsp->conn, shut_rx, shut_tx);
  return (err == ERR_OK ? 0 : -err_to_errno(err));
}

static int inet_close(file_t fp)
{
  struct socket *sp = fp->f_vnode->v_data;
  struct lwip_sock *lwsp = sp->priv;
  int is_tcp = 0;

  DPRINTF(NETDB_INET, ("lwip_close()\n"));

  if(lwsp->conn != NULL) {
    is_tcp = NETCONNTYPE_GROUP(netconn_type(lwsp->conn)) == NETCONN_TCP;
  } else {
    LWIP_ASSERT("lwsp->lastdata == NULL", lwsp->lastdata == NULL);
  }

  netconn_delete(lwsp->conn);

  void *lastdata = lwsp->lastdata;
  kmem_free(lwsp);
  if (lastdata != NULL) {
    if (is_tcp) {
      pbuf_free((struct pbuf *)lastdata);
    } else {
      netbuf_delete((struct netbuf *)lastdata);
    }
  }
  return 0;
}

static const struct domain_interface interface = {
  .setup = setup,
  .getopt = getopt,
  .setopt = setopt,
  .option_update = option_update,
  .ioctl = inet_ioctl,
  .bind = inet_bind,
  .listen = inet_listen,
  .connect = inet_connect,
  .accept4 = inet_accept4,
  .sendto = inet_sendto,
  .recvfrom = inet_recvfrom,
  .getpeername = inet_getpeername,
  .getsockname = inet_getsockname,
  .shutdown = inet_shutdown,
  .close = inet_close,
  .vnops = &net_vnops,
};

#if CONFIG_INET_COMMANDS
static int inetifCommand(int argc, char **argv)
{
  if (argc <= 0) {
    printf("show inet interface information\n");
    return COMMAND_OK;
  }

  static const struct
  {
    int flag;
    const char *name;
  } flags[] = {
    { IFF_UP, "UP" },
    { IFF_BROADCAST, "BROADCAST" },
    { IFF_DEBUG, "DEBUG" },
    { IFF_LOOPBACK, "LOOPBACK" },
    { IFF_POINTOPOINT, "POINTOPOINT" },
    { IFF_RUNNING, "RUNNING" },
    { IFF_NOARP, "NOARP" },
    { IFF_PROMISC, "PROMISC" },
    { IFF_NOTRAILERS, "NOTRAILERS" },
    { IFF_ALLMULTI, "ALLMULTI" },
    { IFF_MASTER, "MASTER" },
    { IFF_SLAVE, "SLAVE" },
    { IFF_MULTICAST, "MULTICAST" },
    { IFF_PORTSEL, "PORTSEL" },
    { IFF_AUTOMEDIA, "AUTOMEDIA" },
    { IFF_DYNAMIC, "DYNAMIC" },
    { IFF_LOWER_UP, "LOWER_UP" },
    { IFF_DORMANT, "DORMANT" },
    { IFF_ECHO, "ECHO" },
    { 0, NULL }
  };

  pthread_mutex_lock(&interface_lock);
  for (int i = 0; i < CONFIG_NET_MAX_INET_INTERFACES; ++i) {
    struct interface *interface = &interfaces[i];
    if (interface->name == NULL) {
      continue;
    }

    struct netif *netif = interface->netif;
    printf("%d %s: %s", i, interface->name,
           netif ? "active" : "inactive\n");
    if (netif == NULL) {
      continue;
    }
    UPDATE_FLAGS();
    printf(" flags=%d (0x%02X)<", interface->flags, netif->flags);
    int comma = 0;
    for (int j = 0; flags[j].flag; ++j) {
      if (interface->flags & flags[j].flag) {
        printf("%s%s", comma ? "," : "", flags[j].name);
        comma = 1;
      }
    }
    printf(">  mtu %u\n", netif->mtu);
    struct in_addr in_addr;
    in_addr.s_addr = netif->ip_addr.addr;
    printf("\tinet %s", inet_ntoa(in_addr));
    in_addr.s_addr = netif->netmask.addr;
    printf("  netmask %s", inet_ntoa(in_addr));
    in_addr.s_addr = ~in_addr.s_addr;
    in_addr.s_addr |= netif->ip_addr.addr;
    printf("  broadcast %s\n", inet_ntoa(in_addr));
    if (netif->hwaddr_len) {
      printf("\tether ");
      int colon = 0;
      for (int j = 0; j < netif->hwaddr_len; ++j) {
        printf("%s%02x", colon ? ":" : "", netif->hwaddr[j]);
        colon = 1;
      }
      printf("\n");
    }
  }

  pthread_mutex_unlock(&interface_lock);
  return COMMAND_OK;
}

/** Create a section heading for the help command.
 */
static int sectionCommand(int argc, char **argv)
{
  if (argc <= 0 ) {
    printf("INET Commands:\n");
  }
  return COMMAND_OK;
}
#endif

ELK_PRECONSTRUCTOR()
{
  inet_interface = &interface;

#if CONFIG_INET_COMMANDS
  command_insert(NULL, sectionCommand);
  command_insert("inetif", inetifCommand);
#endif
}

// Start up LwIP.
C_CONSTRUCTOR()
{
  tcpip_init(0, 0);

#if LWIP_HAVE_LOOPIF
  struct netif *netif = netif_find("lo0");
  if (netif) {
    // Add the loopback interface.
    interfaces[0].name = "lo";
    interfaces[0].flags = IFF_LOOPBACK;
    interfaces[0].netif = netif;
    interfaces[0].init = NULL;
    interfaces[0].input = NULL;
    interfaces[0].state = NULL;
  }
#endif
}

