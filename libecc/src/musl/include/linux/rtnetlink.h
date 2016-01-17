/* Just enough to get libnl and wireless_tools to compile.
 */
#ifndef _LINUX_RTNETLINK_H_
#define _LINUX_RTNETLINK_H_

#include <linux/if_link.h>

enum
{
  RTM_NEWLINK = 16,
  RTM_DELLINK,
};

struct ifinfomsg
{
  unsigned char	ifi_family;
  unsigned char	__ifi_pad;
  unsigned short ifi_type;
  int ifi_index;
  unsigned ifi_flags;
  unsigned ifi_change;
};

struct rtattr
{
  unsigned short rta_len;
  unsigned short rta_type;
};

#define RTA_OK(rta,length) (  (length)>=(int)sizeof(struct rtattr) \
                            && (rta)->rta_len >= sizeof(struct rtattr) \
                            && (rta)->rta_len <= (length))
#define RTA_ALIGNTO 4
#define RTA_ALIGN(length) (((length)+RTA_ALIGNTO-1)&~(RTA_ALIGNTO-1))
#define RTA_NEXT(rta,length) ((length)-=RTA_ALIGN((rta)->rta_len), \
                              (struct rtattr*)(((char*)(rta)) \
                               +RTA_ALIGN((rta)->rta_len)))
#define RTMGRP_LINK 1

#endif /* _LINUX_RTNETLINK_H_ */
