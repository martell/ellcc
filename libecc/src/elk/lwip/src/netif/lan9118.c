/**
 * @file
 * LAN 9118 Ethernet Driver.
 *
 */

/*
 * Copyright (c) 2015 Richard Pennington.
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
 *
 * This file is part of ELK and the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 * Author: Richard Pennington <rich@pennware.com>
 *
 */

#include <unistd.h>
#include <errno.h>

#include "config.h"
#include "kernel.h"
#include "busio.h"

#include "lan9118reg.h"
#include "ethernetif.h"

// Make the driver a loadable feature.
FEATURE(lwip_lan9118)

/** Driver private data.
 */
struct data
{
  vaddr_t base;
  u8_t hwaddr[ETHARP_HWADDR_LEN];
};

// The hardware initialize function.
static int init(void *i, int unit, u8_t *hwaddr_len, u8_t *hwaddr, void *mcast)
{
  struct data *dp = i;

  // Set the MAC hardware address length.
  *hwaddr_len = ETHARP_HWADDR_LEN;
  // Set MAC hardware address.
  memcpy(hwaddr, dp->hwaddr, ETHARP_HWADDR_LEN);
  // Do whatever else is needed to initialize the interface.
  int c = 10;
  while (!(bus_read_32(dp->base + LAN9118_PMT_CTRL) & LAN9118_PMT_CTRL_READY)) {
    usleep(500000);     // 500 milliseconds.
    if (--c == 0) {
      return -EBUSY;
    }
  }

  // Soft reset.
  bus_write_32(dp->base + LAN9118_HW_CFG, LAN9118_HW_CFG_SRST);
  uint32_t reg;
  do {
    reg = bus_read_32(dp->base + LAN9118_HW_CFG);
    if (reg & LAN9118_HW_CFG_SRST_TO) {
      return -ETIMEDOUT;
    }
  } while (reg & LAN9118_HW_CFG_SRST);

  diag_printf("9118 good\n");
  return 0;
}

// Check room. RICH: Clarify.
static int startoutput(void *i, int unit)
{
  return 0;
}

// Write blocks.
static void output(void *i, int unit, void *data, uint16_t len)
{
}

// End writing, send.
static void endoutput(void *i, int unit, uint16_t total_len)
{
}

// Check existence, get length.
static int startinput(void *i, int unit)
{
  return 0;
}

// Read blocks.
static void input(void *i, int unit, void *data, uint16_t len)
{
}

// End reading.
static void endinput(void *i, int unit)
{
}

// Drop or queue the packet if the interface allows it.
static void input_nomem(void *i, int unit, uint16_t len)
{
}

static const struct etherops ops = {
  init,
  startoutput,
  output,
  endoutput,
  startinput,
  input,
  endinput,
  input_nomem,
};

static struct data data[] = {
  { .base = 0xdb000000,         // RICH: Address assignment.
    .hwaddr = "\x06\x05\x04\x03\x02\x01",
  },
  { .base = 0xda000000,
    .hwaddr = "\x16\x15\x14\x13\x12\x11",
  },
};
#define UNITS (sizeof(data) / sizeof(data[0]))

const static struct ethernetif ethernetif[] = {
  { .name = "la0", .ops = &ops, .priv = &data[0], },
  { .name = "la1", .ops = &ops, .priv = &data[1], },
};

ELK_CONSTRUCTOR()
{
  // Register the interface(s).
  for (int u = 0; u < UNITS; ++u) {
    ethernetif_add_interface((struct ethernetif *)&ethernetif[u]);
  }
}
