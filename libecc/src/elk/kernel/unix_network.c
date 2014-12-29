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

static int setup(void **priv, int domain, int protocol, int type)
{
  return -EPROTONOSUPPORT;
}

static int getopt(file_t fp, int level, int optname, void *optval,
                      socklen_t *optlen)
{
  return -ENOPROTOOPT;
}

static const struct domain_interface interface = {
  .setup = setup,
  .getopt = getopt,
  .vnops = &vnops,
};

// Register the interface.
C_CONSTRUCTOR()
{
  unix_interface = &interface;
}

