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
 * Copyright (c) 2008 KIYOHARA Takashi
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THI
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
  u16_t mtu;
  u8_t hwaddr[ETHARP_HWADDR_LEN];
};

static void delay(int microsecs)
{
  // RICH: Implement somewhere.
}


static uint32_t mac_readreg(vaddr_t base, int reg)
{
  uint32_t cmd;
  int timo = 3 * 1000 * 1000;           /* XXXX: 3sec */

  bus_write_32(base + LAN9118_MAC_CSR_CMD,
               LAN9118_MAC_CSR_CMD_BUSY | LAN9118_MAC_CSR_CMD_R | reg);
  do {
    cmd = bus_read_32(base + LAN9118_MAC_CSR_CMD);
    if (!(cmd & LAN9118_MAC_CSR_CMD_BUSY))
      break;

    delay(100);
  } while (timo -= 100);

   if (timo <= 0)
     diag_printf("%s: command busy\n", __func__);

  return bus_read_32(base + LAN9118_MAC_CSR_DATA);
}

static void mac_writereg(vaddr_t base, int reg, uint32_t val)
{
  uint32_t cmd;
  int timo = 3 * 1000 * 1000;           /* XXXX: 3sec */

  bus_write_32(base + LAN9118_MAC_CSR_DATA, val);
  bus_write_32(base + LAN9118_MAC_CSR_CMD,
               LAN9118_MAC_CSR_CMD_BUSY | LAN9118_MAC_CSR_CMD_W | reg);
  do {
    cmd = bus_read_32(base + LAN9118_MAC_CSR_CMD);
    if (!(cmd & LAN9118_MAC_CSR_CMD_BUSY))
      break;
    delay(100);
  } while (timo -= 100);
  if (timo <= 0)
    diag_printf("%s: command busy\n", __func__);
}

static int setup_phy(vaddr_t base)
{
  // Turn off the clock.
  bus_write_32(base + LAN9118_HW_CFG,
               LAN9118_HW_CFG_MBO | LAN9118_HW_CFG_PHY_CLK_SEL_CD);
  delay(1);

  // Use the internal PHY.
  bus_write_32(base + LAN9118_HW_CFG,
               LAN9118_HW_CFG_MBO | LAN9118_HW_CFG_PHY_CLK_SEL_IPHY);
  delay(1);

  // Reset PHY.
  uint32_t pmt_ctrl = bus_read_32(base + LAN9118_PMT_CTRL);
  bus_write_32(base + LAN9118_PMT_CTRL, pmt_ctrl | LAN9118_PMT_CTRL_PHY_RST);
  while (bus_read_32(base + LAN9118_PMT_CTRL) & LAN9118_PMT_CTRL_PHY_RST)
    continue;

  return 0;
}

// The hardware initialize function.
static int init(void *i, u8_t *hwaddr_len, u8_t *hwaddr,
                u16_t *mtu, void *mcast)
{
  struct data *dp = i;

  // Set the maximum transfer unit.
  *mtu = dp->mtu;

  // Set the MAC hardware address length.
  *hwaddr_len = ETHARP_HWADDR_LEN;
  // Set MAC hardware address.
  memcpy(hwaddr, dp->hwaddr, ETHARP_HWADDR_LEN);
  // Do whatever else is needed to initialize the interface.
  int c = 10;
  while (!(bus_read_32(dp->base + LAN9118_PMT_CTRL) & LAN9118_PMT_CTRL_READY)) {
    delay(500000);     // 500 milliseconds.
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

  if (bus_read_32(dp->base + LAN9118_BYTE_TEST) == LAN9118_BYTE_TEST_VALUE) {
    // The byte order is wrong.
    bus_write_32(dp->base + LAN9118_WORD_SWAP, 0xffffffff);
    if (bus_read_32(dp->base + LAN9118_BYTE_TEST) != LAN9118_BYTE_TEST_VALUE) {
      return -EIO;
    }
  }

  // MAke sure the EEPROM is not busy.
  while (bus_read_32(dp->base + LAN9118_E2P_CMD) & LAN9118_E2P_CMD_EPCB)
    continue;

   if (!(bus_read_32(dp->base + LAN9118_E2P_CMD) & LAN9118_E2P_CMD_MACAL)) {
     mac_writereg(dp->base, LAN9118_ADDRL,
                  dp->hwaddr[0] |
                  dp->hwaddr[1] << 8 |
                  dp->hwaddr[2] << 16 |
                  dp->hwaddr[3] << 24);
     mac_writereg(dp->base, LAN9118_ADDRH,
                  dp->hwaddr[4] | dp->hwaddr[5] << 8);
  }

  // RICH: Set flow control here.

  int s = setup_phy(dp->base);
  if (s != 0) {
    return s;
  }

  // Set the trasmit FIFO size.
  reg = bus_read_32(dp->base + LAN9118_HW_CFG);
  reg &= ~LAN9118_HW_CFG_TX_FIF_MASK;
  reg |= LAN9118_HW_CFG_TX_FIF_SZ(LAN9118_TX_FIF_SZ);
  bus_write_32(dp->base + LAN9118_HW_CFG, reg);

  // Configure the GPIO.
  bus_write_32(dp->base + LAN9118_GPIO_CFG,
               LAN9118_GPIO_CFG_LEDX_EN(2)  |
               LAN9118_GPIO_CFG_LEDX_EN(1)  |
               LAN9118_GPIO_CFG_LEDX_EN(0)  |
               LAN9118_GPIO_CFG_GPIOBUFN(2) |
               LAN9118_GPIO_CFG_GPIOBUFN(1) |
               LAN9118_GPIO_CFG_GPIOBUFN(0));

  bus_write_32(dp->base + LAN9118_IRQ_CFG, LAN9118_IRQ_CFG_IRQ_EN);
  bus_write_32(dp->base + LAN9118_INT_STS,
               bus_read_32(dp->base + LAN9118_INT_STS));
  bus_write_32(dp->base + LAN9118_FIFO_INT,
               LAN9118_FIFO_INT_TXSL(0) | LAN9118_FIFO_INT_RXSL(0));

  // Enable interrupts.
  bus_write_32(dp->base + LAN9118_INT_EN,
#if 0   // not yet...
               LAN9118_INT_PHY_INT | /* PHY */
               LAN9118_INT_PME_INT | /* Power Management Event */
#endif
               LAN9118_INT_RXE     | /* Receive Error */
               LAN9118_INT_TSFL    | /* TX Status FIFO Level */
               LAN9118_INT_RXDF_INT| /* RX Dropped Frame Interrupt */
               LAN9118_INT_RSFF    | /* RX Status FIFO Full */
               LAN9118_INT_RSFL);    /* RX Status FIFO Level */

  bus_write_32(dp->base + LAN9118_RX_CFG, LAN9118_RX_CFG_RXDOFF(2));
  bus_write_32(dp->base + LAN9118_TX_CFG, LAN9118_TX_CFG_TX_ON);
  reg = mac_readreg(dp->base, LAN9118_MAC_CR);
  mac_writereg(dp->base, LAN9118_MAC_CR,
               reg | LAN9118_MAC_CR_TXEN | LAN9118_MAC_CR_RXEN);

  // RICH: lan9118_set_filter(sc);
  return 0;
}

/** Check the interface for room in the transmit buffer.
 * This function could wait until space is available if the
 * transmitter is active.
 */
static int startoutput(void *i, uint16_t total_len, int fragcnt)
{
  struct data *dp = i;
  unsigned tx;
  /* Calculate the buffer space needed for this packet.
   * Two control words + fragment data.
   * RICH: This is wrong. Adjust.
   */
  size_t size = total_len + (fragcnt * (2 * sizeof(uint32_t)));

  // Find the amount of free TX buffer space.
  tx = LAN9118_TX_FIFO_INF_TDFREE(bus_read_32(dp->base + LAN9118_TX_FIFO_INF));
diag_printf("fragcnt = %u size = %zu tx = %u\n", fragcnt, total_len, tx);
  if (tx >= size) {
    return 1;
  }

  // RICH: Delay until space is available?
  return 0;
}

// Write data to the interface.
static void output(void *i, void *data, uint16_t len)
{
  struct data *dp = i;

}

// End writing, send.
static void endoutput(void *i, uint16_t total_len)
{
}

// Check existence, get length.
static int startinput(void *i)
{
  return 0;
}

// Read blocks.
static void input(void *i, void *data, uint16_t len)
{
}

// End reading.
static void endinput(void *i)
{
}

// Drop or queue the packet if the interface allows it.
static void input_nomem(void *i, uint16_t len)
{
}

static const struct etherops ops = {
  .flags = ETHIF_FRAGCNT,
  .init = init,
  .startoutput = startoutput,
  .output = output,
  .endoutput = endoutput,
  .startinput = startinput,
  .input = input,
  .endinput = endinput,
  .input_nomem = input_nomem,
};

static struct data data[] = {
  { .base = 0xdb000000,         // RICH: Address assignment.
    .mtu = ETHERNET_MTU,
    .hwaddr = "\x06\x05\x04\x03\x02\x01",
  },
  { .base = 0xda000000,
    .mtu = ETHERNET_MTU,
    .hwaddr = "\x16\x15\x14\x13\x12\x11",
  },
};
#define UNITS (sizeof(data) / sizeof(data[0]))

const static struct ethernetif ethernetif[] = {
  { .name = "la0", .ops = &ops, .priv = &data[0], },
  { .name = "la1", .ops = &ops, .priv = &data[1], },
};

ELK_PRECONSTRUCTOR()
{
  // Register the interface(s).
  for (int u = 0; u < UNITS; ++u) {
    ethernetif_add_interface((struct ethernetif *)&ethernetif[u]);
  }
}
