/**
 * @file ethernetif.c
 * Ethernet Interface
 *
 * Modified by SRC from the original ethernetif.c distributed with lwIP.
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
 */

#include "config.h"
#include "kernel.h"

#include <stdint.h>
#include <string.h>

#include "lwip/opt.h"

#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/stats.h"
#include "lwip/snmp.h"
#include "lwip/ethip6.h"
#include "netif/etharp.h"
#include "netif/ppp/pppoe.h"
#include "lwip/tcpip.h"

#include "ethernetif.h"
#include "lwip/lwip_interface.h"  // This is an ELK include file.

// RICH: This should come from somewhere else.
/* Define those to better describe your network interface. */
#define IFNAME0 'e'
#define IFNAME1 'n'

#ifndef ETHERNETIF_MAXFRAMES
#define ETHERNETIF_MAXFRAMES 0
#endif

/**
 * This function should be called when a packet is ready to be read
 * from the interface. It uses the function input() that
 * should handle the actual reception of bytes from the network
 * interface. Then the type of the received packet is determined and
 * the appropriate input function is called.
 *
 * @param netif the lwip network interface structure for this ethernetif
 */
/* RICH: This will be called from the interrupt handler.
static */ void input(struct netif *netif)
{
  struct ethernetif *ethernetif;
  struct eth_hdr *ethhdr;
  struct pbuf *p, *q;
  int len;
  int frames = 0;

  ethernetif = netif->state;
  do {
    if((len = ethernetif->ops->startinput(ethernetif->priv)) == 0)
      break;

    // Move received packet into a new pbuf.
#if ETH_PAD_SIZE
    len += ETH_PAD_SIZE; // Allow room for Ethernet padding.
#endif

    // We allocate a pbuf chain of pbufs from the pool.
    p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);

    if (p != NULL) {

#if ETH_PAD_SIZE
      pbuf_header(p, -ETH_PAD_SIZE); /* drop the padding word */
#endif

      /* We iterate over the pbuf chain until we have read the entire
       * packet into the pbuf.
       */
      for(q = p; q != NULL; q = q->next) {
        /* Read enough bytes to fill this pbuf in the chain. The
         * available data in the pbuf is given by the q->len
         * variable.
         * This does not necessarily have to be a memcpy, you can also
         * preallocate pbufs for a DMA-enabled MAC and after receiving
         * truncate it to the actually received size. In this case,
         * ensure the tot_len member of the pbuf is the sum of the chained
         * pbuf len members.
         */
        ethernetif->ops->input(ethernetif->priv,
                               q->payload, q->len);
      }

      ethernetif->ops->endinput(ethernetif->priv);

#if ETH_PAD_SIZE
      pbuf_header(p, ETH_PAD_SIZE); /* reclaim the padding word */
#endif

      LINK_STATS_INC(link.recv);
    } else {
      /* In many embedded systems, we might have less available RAM
       * than what the Ethernet chip has, so if we can't allocate a
       * pbuf, the frame can sit waiting in the chip and there is no
       * need to drop it.  Let the driver decide what to do.
       */
#if ETH_PAD_SIZE
      len -= ETH_PAD_SIZE; // Allow room for Ethernet padding.
#endif
      ethernetif->ops->input_nomem(ethernetif->priv, len);
      LINK_STATS_INC(link.memerr);
      LINK_STATS_INC(link.drop);
      return;
    }

    // Points to packet payload, which starts with an Ethernet header.
    ethhdr = p->payload;

    switch (htons(ethhdr->type)) {
    // IP or ARP packet?
    case ETHTYPE_IP:
    case ETHTYPE_ARP:
#if PPPOE_SUPPORT
    /* PPPoE packet? */
    case ETHTYPE_PPPOEDISC:
    case ETHTYPE_PPPOE:
#endif // PPPOE_SUPPORT
      // Full packet send to tcpip_thread to process.
      if (netif->input(p, netif) != ERR_OK)
       { LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_input: IP input error\n"));
         pbuf_free(p);
         p = NULL;
       }
      break;

    default:
      pbuf_free(p);
      p = NULL;
      break;
    }
  } while((!ETHERNETIF_MAXFRAMES) || (++frames < ETHERNETIF_MAXFRAMES));
}

/**
 * This function should do the actual transmission of the packet. The packet is
 * contained in the pbuf that is passed to the function. This pbuf
 * might be chained.
 *
 * @param netif The lwip network interface structure for this ethernetif
 * @param p The MAC packet to send (e.g. IP packet including MAC addresses and
 *           type)
 * @return ERR_OK if the packet could be sent
 *         ERR_IF if the packet couldn't be sent
 *
 * @note Returning ERR_MEM here if a DMA queue of your MAC is full can lead to
 *       strange results. You might consider waiting for space in the DMA queue
 *       to become availale since the stack doesn't retry to send a packet
 *       dropped because of memory failure (except for the TCP timers).
 */

static err_t linkoutput(struct netif *netif, struct pbuf *p)
{
  struct ethernetif *ethernetif = netif->state;
  struct pbuf *q;
  uint16_t total_len = p->tot_len;      // The length of the packet.
  int fragcnt = 0;                      // The number of fragments.
  unsigned flags = ethernetif->ops->flags;

  if (flags & ETHIF_FRAGCNT) {
    // Pre-calculate the number of fragments for startoutput().
    for(q = p; q != NULL; q = q->next) {
      ++fragcnt;
    }
  }

  if(!ethernetif->ops->startoutput(ethernetif->priv, total_len, fragcnt))
    return ERR_IF;

#if ETH_PAD_SIZE
  pbuf_header(p, -ETH_PAD_SIZE); // Drop the padding word.
#endif

  for(q = p; q != NULL; q = q->next) {
    /* Send the data from the pbuf to the interface, one pbuf at a
     * time. The size of the data in each pbuf is kept in the ->len
     * variable.
     */
    ethernetif->ops->output(ethernetif->priv, q->payload, q->len);
  }

  ethernetif->ops->endoutput(ethernetif->priv, total_len);

#if ETH_PAD_SIZE
  pbuf_header(p, ETH_PAD_SIZE); // Reclaim the padding word.
#endif

  LINK_STATS_INC(link.xmit);

  return ERR_OK;
}

/**
 * Should be called at the beginning of the program to set up the
 * network interface. It calls the function init() to do the
 * actual setup of the hardware.
 *
 * This function should be passed as a parameter to netif_add().
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return ERR_OK if the loopif is initialized
 *         ERR_MEM if private data couldn't be allocated
 *         any other err_t on error
 */
static err_t init(struct netif *netif)
{
  struct ethernetif *ethernetif;

  LWIP_ASSERT("netif != NULL", (netif != NULL));
  LWIP_ASSERT("state != NULL", (netif->state != NULL));

  ethernetif = netif->state;

#if LWIP_NETIF_HOSTNAME
  // Initialize interface hostname.
  netif->hostname = "lwip";
#endif // LWIP_NETIF_HOSTNAME

  /* Initialize the snmp variables and counters inside the struct netif.
   * The last argument should be replaced with your link speed, in units
   * of bits per second.
   */
#ifdef LINK_SPEED_OF_YOUR_NETIF_IN_BPS  // RICH: Where to get this?
  NETIF_INIT_SNMP(netif, snmp_ifType_ethernet_csmacd,
                  LINK_SPEED_OF_YOUR_NETIF_IN_BPS);
#endif

  netif->name[0] = IFNAME0;
  netif->name[1] = IFNAME1;
  /* We directly use etharp_output() here to save a function call.
   * You can instead declare your own function an call etharp_output()
   * from it if you have to do some checks before sending (e.g. if link
   * is available...).
   */
  netif->output = etharp_output;
#if LWIP_IPV6
  netif->output_ip6 = ethip6_output;
#endif // LWIP_IPV6
  netif->linkoutput = linkoutput;

  // Device capabilities.
  // Don't set NETIF_FLAG_ETHARP if this device is not an ethernet one.
  netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;

  // Initialize the hardware and send back the Mac address.
  int s = ethernetif->ops->init(ethernetif->priv,
                                &netif->hwaddr_len, netif->hwaddr,
                                &netif->mtu, NULL);
  if (s < 0) {
    return ERR_ARG;     // RICH: Better error?
  }

  return ERR_OK;
}

// The array of known drivers.
static int count;
static struct ethernetif *interfaces[CONFIG_NET_MAX_INET_INTERFACES];

/** Register a low level interface.
 */
int ethernetif_add_interface(struct ethernetif *ethernetif)
{
  if (count >= CONFIG_NET_MAX_INET_INTERFACES) {
    return -EAGAIN;
  }

  interfaces[count++] = ethernetif;
  return 0;
}

C_CONSTRUCTOR()
{
  // Register the known interfaces.
  for (int i = 0; i < count; ++i) {
    struct ethernetif *eif = interfaces[i];
    // Add an interface for each unit.
    lwip_add_interface(eif->name, init, tcpip_input, eif);
  }
}
