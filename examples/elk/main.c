/* ELK running as a VM enabled OS.
 */
#define _GNU_SOURCE
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <sys/cdefs.h>
#include <stdio.h>

#include "command.h"

int main(int argc, char **argv)
{
  setprogname("elk");
  printf("%s started. Type \"help\" for a list of commands.\n", getprogname());

  int sfd = socket(AF_INET, SOCK_DGRAM /*|SOCK_NONBLOCK */, IPPROTO_UDP);
  if (sfd < 0) {
    printf("socket(AF_INET) failed: %s\n", strerror(errno));
  }

#if 1
  int s;
  struct ifreq ifreq;
  struct in_addr in_addr;

  // Set the device name for subsequent calls.
  strcpy(ifreq.ifr_name, "la0");

  // Set the interface IP address.
  inet_aton("192.124.43.4", &in_addr);
  ifreq.ifr_addr.sa_family = AF_INET;
  memcpy(ifreq.ifr_addr.sa_data, &in_addr, sizeof(in_addr));
  s = ioctl(sfd, SIOCSIFADDR, &ifreq);
  if (s < 0) {
    printf("ioctl(SIOCSIFADDR) failed: %s\n", strerror(errno));
  }

  // Set the interface netmask.
  inet_aton("255.255.255.0", &in_addr);
  ifreq.ifr_netmask.sa_family = AF_INET;
  memcpy(ifreq.ifr_netmask.sa_data, &in_addr, sizeof(in_addr));
  s = ioctl(sfd, SIOCSIFNETMASK, &ifreq);
  if (s < 0) {
    printf("ioctl(SIOCSIFNETMASK) failed: %s\n", strerror(errno));
  }

#if 0
  // Set the interface MAC address.
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
#endif

  s = ioctl(sfd, SIOCGIFFLAGS, &ifreq);
  if (s < 0) {
    printf("ioctl(SIOCGIFFLAGS) failed: %s\n", strerror(errno));
  }
  ifreq.ifr_flags |= IFF_UP;
  s = ioctl(sfd, SIOCSIFFLAGS, &ifreq);
  if (s < 0) {
    printf("ioctl(SIOCSIFFLAGS) failed: %s\n", strerror(errno));
  }

  struct sockaddr_in sockaddr_in;
  inet_aton("192.124.43.4", (struct in_addr *)&sockaddr_in.sin_addr);
  sockaddr_in.sin_family = AF_INET;
  sockaddr_in.sin_port = htons(8080);
  s = bind(sfd, (struct sockaddr *)&sockaddr_in, sizeof(sockaddr_in));
  if (s < 0) {
    printf("bind() failed: %s\n", strerror(errno));
  }

  // RICH: 192.124.43.4 uses the loopback interface.
  inet_aton("192.124.43.4", (struct in_addr *)&sockaddr_in.sin_addr);
  sockaddr_in.sin_family = AF_INET;
  sockaddr_in.sin_port = htons(8080);
  char buf[100];
  strcpy(buf, "hello world\n");
  size_t len = strlen(buf) + 1;
  s = sendto(sfd, buf, len, 0, (struct sockaddr *)&sockaddr_in, sizeof(sockaddr_in));
  if (s < 0) {
    printf("sendto() failed: %s\n", strerror(errno));
  } else {
    s = recv(sfd, buf, 100, 0); // MSG_DONTWAIT);
    if (s < 0) {
      printf("recv() failed: %s\n", strerror(errno));
    } else {
      printf("got '%s'\n", buf);
    }
  }
#endif

  printf("Try the command 'inetif'\n");

  // Enter the kernel command processor.
  do_commands(getprogname());
}

