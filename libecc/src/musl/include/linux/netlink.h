/* Just enough to get suckless to compile.
 */
#ifndef _LINUX_NETLINK_H_
#define _LINUX_NETLINK_H_

#include <linux/socket.h>
#include <inttypes.h>

struct sockaddr_nl
{
  __kernel_sa_family_t nl_family;
  unsigned short nl_pad;
  uint32_t nl_pid;
  uint32_t nl_groups;
};

#define NETLINK_KOBJECT_UEVENT 15

#endif /* _LINUX_NETLINK_H_ */
