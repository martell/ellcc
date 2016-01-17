/* Just enough to get suckless and wireless_tools to compile.
 */
#ifndef _LINUX_NETLINK_H_
#define _LINUX_NETLINK_H_

#include <linux/socket.h>
#include <linux/types.h>

struct sockaddr_nl
{
  __kernel_sa_family_t nl_family;
  __u16 nl_pad;
  __u32 nl_pid;
  __u32 nl_groups;
};

struct nlmsghdr
{
  __u32 nlmsg_len;
  __u16 nlmsg_type;
  __u16 nlmsg_flags;
  __u32 nlmsg_seq;
  __u32 nlmsg_pid;
};

#define NETLINK_ROUTE 0
#define NETLINK_KOBJECT_UEVENT 15

#define NLMSG_ALIGNTO 4
#define NLMSG_ALIGN(length) (((length)+NLMSG_ALIGNTO-1)&~(NLMSG_ALIGNTO-1))
#define NLMSG_HDRLEN ((int)NLMSG_ALIGN(sizeof(struct nlmsghdr)))
#define NLMSG_LENGTH(length) ((length)+NLMSG_HDRLEN)
#define NLMSG_DATA(header) ((void*)(((char*)(header))+NLMSG_LENGTH(0)))

#endif /* _LINUX_NETLINK_H_ */
