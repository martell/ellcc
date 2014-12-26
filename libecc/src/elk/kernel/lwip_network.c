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

#include <sys/socket.h>

#include "config.h"
#include "kernel.h"
#include "syscalls.h"
#include "lwip/tcpip.h"
#include "crt1.h"

// Make LWIP networking a select-able feature.
FEATURE_CLASS(lwip_network, network)

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

static int sys_getsockopt(int sockfd, int level, int optname, void *optval,
                          socklen_t *optlen)
{
  return -ENOSYS;
}

static int sys_listen(int sockfd, int backlog)
{
  return -ENOSYS;
}

#if defined(SYS_socketcall) || defined(SYS_recv)
static int sys_recv(int sockfd, void *buf, size_t len, int flags)
{
  return -ENOSYS;
}
#endif

static int sys_recvfrom(int sockfd, void *buf, size_t len, int flags,
                        struct sockaddr *src_addr, socklen_t *addrlen)
{
  return -ENOSYS;
}

static int sys_recvmmsg(int sockfd, struct msghdr *msgvec, unsigned int vlen,
                        unsigned int flags, struct timespec *timeout)
{
  return -ENOSYS;
}

static int sys_recvmsg(int sockfd, struct msghdr *msgvec, unsigned int flags)
{
  return -ENOSYS;
}

#if defined(SYS_socketcall) || defined(SYS_send)
static int sys_send(int sockfd, const void *buf, size_t len, int flags)
{
  return -ENOSYS;
}
#endif

static int sys_sendto(int sockfd, const void *buf, size_t len, int flags,
                        struct sockaddr *src_addr, socklen_t *addrlen)
{
  return -ENOSYS;
}

static int sys_sendmmsg(int sockfd, const struct msghdr *msgvec,
                        unsigned int vlen, unsigned int flags)
{
  return -ENOSYS;
}

static int sys_sendmsg(int sockfd, const struct msghdr *msgvec,
                       unsigned int flags)
{
  return -ENOSYS;
}

static int sys_setsockopt(int sockfd, int level, int optname,
                          const void *optval, socklen_t *optlen)
{
  return -ENOSYS;
}

static int sys_socket(int domain, int type, int protocol)
{
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
                          (socklen_t *)arg[4]);
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
  SYSCALL(socket);
  SYSCALL(socketpair);

#else

  SYSCALL(socketcall);

#endif
}

// Start up LWIP.
C_CONSTRUCTOR()
{
  tcpip_init(0, 0);
}

