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
 */

/** ELK specific LWIP configuration.
 */

#ifndef _lwipopts_h_
#define _lwipopts_h_

#define LWIP_SOCKET                     1       // ELK has its own socket API.
#define LWIP_SOCKET_TYPE(name) struct { int name; void *priv; }
#define LWIP_SOCKET_SET_ERRNO           0       // ELK takes care of errno.
#define LWIP_NETIF_API                  1
#define MEM_LIBC_MALLOC                 1       // Use ELK memory management.
#define MEMP_MEM_MALLOC                 1
#define MEM_ALIGNMENT                   4
#define IP_FORWARD                      1
#define IP_SOF_BROADCAST                1
#define IP_SOF_BROADCAST_RECV           1
#define LWIP_RANDOMIZE_INITIAL_LOCAL_PORTS 0    // May change.
#define LWIP_BROADCAST_PING             1
#define LWIP_MULTICAST_PING             1
#define LWIP_DHCP                       1
#define LWIP_AUTOIP                     0
#define LWIP_SNMP                       1
#define LWIP_IGMP                       1
#define LWIP_DNS                        1
#define LWIP_HAVE_LOOPIF                1
#define LWIP_HAVE_SLIPIF                0
#define TCPIP_THREAD_STACKSIZE          0
#define TCPIP_THREAD_PRIO               1
#define LWIP_NETCONN                    1
#define LWIP_TCP_KEEPALIVE              1
#define LWIP_SO_SNDTIMEO                1
#define LWIP_SO_RCVTIMEO                1
#define LWIP_SO_RCVBUF                  1
#define SO_REUSE                        1
#define LWIP_FIONREAD_LINUXMODE         1
#define PPP_SUPPORT                     0
#define PPPOE_SUPPORT                   0
#define PPP_IPV6_SUPPORT                1
#define LWIP_IPV6                       0
#define LWIP_IPV6_FORWARD               0
#define LWIP_TCPIP_CORE_LOCKING         1

#endif // _lwipopts_h_
