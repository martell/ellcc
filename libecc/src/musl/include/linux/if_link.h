/* Just enough to get wireless_tools to compile.
 */
#ifndef _LINUX_IF_LINK_H_
#define _LINUX_IF_LINK_H_

enum
{
  IFLA_UNSPEC,
  IFLA_ADDRESS,
  IFLA_BROADCAST,
  IFLA_IFNAME,
  IFLA_MTU,
  IFLA_LINK,
  IFLA_QDISC,
  IFLA_STATS,
  IFLA_COST,
  IFLA_PRIORITY,
  IFLA_MASTER,
};

#endif /* _LINUX_IF_LINK_H_ */
