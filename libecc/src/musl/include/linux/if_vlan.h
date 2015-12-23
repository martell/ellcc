/* Just enough to get toybox to compile.
 */
#ifndef _LINUX_IF_VLAN_H_
#define _LINUX_IF_VLAN_H_

enum
{
  ADD_VLAN_CMD,
  DEL_VLAN_CMD,
  SET_VLAN_INGRESS_PRIORITY_CMD,
  SET_VLAN_EGRESS_PRIORITY_CMD,
  GET_VLAN_INGRESS_PRIORITY_CMD,
  GET_VLAN_EGRESS_PRIORITY_CMD,
  SET_VLAN_NAME_TYPE_CMD,
  SET_VLAN_FLAG_CMD,
  GET_VLAN_REALDEV_NAME_CMD,
  GET_VLAN_VID_CMD
};

struct vlan_ioctl_args
{
  int cmd;
  char device1[24];
  union
  {
    char device2[24];
    int VID;
    unsigned int skb_priority;
    unsigned int name_type;
    unsigned int bind_type;
    unsigned int flag;
  } u;
  short vlan_qos;
};

#endif /* _LINUX_IF_VLAN_H_ */
