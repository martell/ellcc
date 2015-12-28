#ifndef _LINUX_IF_PACKET_H_
#define _LINUX_IF_PACKET_H_

struct sockaddr_ll
{
  unsigned short sll_family;
  uint16_t sll_protocol;
  int sll_ifindex;
  unsigned short sll_hatype;
  unsigned char sll_pkttype;
  unsigned char sll_halen;
  unsigned char sll_addr[8];
};

#endif /* _LINUX_IF_PACKET_H_ */
