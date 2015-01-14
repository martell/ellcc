#ifndef _ethernetif_h_
#define _ethernetif_h_

#include "config.h"
#include "netif/etharp.h"

#define ETHERNET_MTU		1500

struct etherops
{
  unsigned flags;               // Driver flags, see below.
  // The hardware initialize function.
  int (*init)(void *i, u8_t *hwaddr_len, u8_t *hwaddr, u16_t *mtu, void *mcast);
  // Check the device for room in the transmit buffer.
  int (*startoutput)(void *i, uint16_t total_len, int fragcnt);
  // Write blocks.
  void (*output)(void *i, void *data, uint16_t len);
  // End writing, send.
  void (*endoutput)(void *i, uint16_t total_len);
  // Check existence, get length.
  int (*startinput)(void *i);
  // Read blocks.
  void (*input)(void *i, void *data, uint16_t len);
  // End reading.
  void (*endinput)(void *i);
  // Drop or queue the packet if the interface allows it.
  void (*input_nomem)(void *i, uint16_t len);
};

// Flag values.

// Calculate the number of fragments for startoutput().
#define ETHIF_FRAGCNT   0x00001

struct ethernetif
{
  const char *name;             // The ELK name for this device.
  const struct etherops *ops;   // The low level callback functions.
  void *priv;                   // Driver private data.
};

#if ELK_NAMESPACE
#define ethernetif_add_interface __elk_ethernetif_add_interface
#endif

int ethernetif_add_interface(struct ethernetif *ethernetif);

#endif // _ethernetif_h_
