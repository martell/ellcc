/* ELK running as a VM enabled OS.
 */
#define _GNU_SOURCE
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>

#include <sys/cdefs.h>
#include <stdio.h>

#include "command.h"

int main(int argc, char **argv)
{
  setprogname("elk");
  printf("%s started. Type \"help\" for a list of commands.\n", getprogname());

  int sfd = socket(AF_INET, SOCK_STREAM /*|SOCK_NONBLOCK */, 0);
  if (sfd < 0) {
    printf("socket(AF_INET) failed: %s\n", strerror(errno));
  }
  struct ifreq ifreq;
  strcpy(ifreq.ifr_name, "ln01");
  int s = ioctl(sfd, SIOCGIFFLAGS, &ifreq);
  if (s < 0) {
    printf("ioctl(SIOCGIFFLAGS) failed: %s\n", strerror(errno));
  }
  struct in_addr in_addr;
  s = ioctl(sfd, SIOCGIFADDR, &ifreq);
  if (s < 0) {
    printf("ioctl(SIOCGIFADDR) failed: %s\n", strerror(errno));
  } else {
    memcpy(&in_addr.s_addr, ifreq.ifr_addr.sa_data, sizeof(in_addr.s_addr));
  }
  inet_aton("192.124.43.4", &in_addr);
  ifreq.ifr_addr.sa_family = AF_INET;
  memcpy(ifreq.ifr_addr.sa_data, &in_addr, sizeof(in_addr));
  s = ioctl(sfd, SIOCSIFADDR, &ifreq);
  if (s < 0) {
    printf("ioctl(SIOCSIFADDR) failed: %s\n", strerror(errno));
  }

  s = ioctl(sfd, SIOCGIFADDR, &ifreq);
  if (s < 0) {
    printf("ioctl(SIOCGIFADDR) failed: %s\n", strerror(errno));
  } else {
    memcpy(&in_addr.s_addr, ifreq.ifr_addr.sa_data, sizeof(in_addr.s_addr));
  }

  inet_aton("255.255.255.0", &in_addr);
  ifreq.ifr_netmask.sa_family = AF_INET;
  memcpy(ifreq.ifr_netmask.sa_data, &in_addr, sizeof(in_addr));
  s = ioctl(sfd, SIOCSIFNETMASK, &ifreq);
  if (s < 0) {
    printf("ioctl(SIOCSIFNETMASK) failed: %s\n", strerror(errno));
  }

  ifreq.ifr_hwaddr.sa_family = ARPHRD_ETHER;
  ifreq.ifr_hwaddr.sa_data[0] = 0x01;
  ifreq.ifr_hwaddr.sa_data[1] = 0x02;
  ifreq.ifr_hwaddr.sa_data[2] = 0x03;
  ifreq.ifr_hwaddr.sa_data[3] = 0x04;
  ifreq.ifr_hwaddr.sa_data[4] = 0x05;
  ifreq.ifr_hwaddr.sa_data[5] = 0x06;
  s = ioctl(sfd, SIOCSIFHWADDR, &ifreq);
  if (s < 0) {
    printf("ioctl(SIOCSIFHWADDR) failed: %s\n", strerror(errno));
  }
  printf("Try the command 'inetif'\n");

  // Enter the kernel command processor.
  do_commands(getprogname());
}

