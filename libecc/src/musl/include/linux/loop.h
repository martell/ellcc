/* Just enough to get toybox to compile.
 */
#ifndef _LINUX_LOOP_H_
#define _LINUX_LOOP_H_

#include <linux/types.h>
#define LO_NAME_SIZE 64
#define LO_KEY_SIZE 32
struct loop_info64
{
  __u64 lo_device;
  __u64 lo_inode;
  __u64 lo_rdevice;
  __u64 lo_offset;
  __u64 lo_sizelimit;
  __u32 lo_number;
  __u32 lo_encrypt_type;
  __u32 lo_encrypt_key_size;
  __u32 lo_flags;
  __u8  lo_file_name[LO_NAME_SIZE];
  __u8  lo_crypt_name[LO_NAME_SIZE];
  __u8  lo_encrypt_key[LO_KEY_SIZE];
  __u64 lo_init[2];
};

#define LOOP_SET_FD 0x4C00
#define LOOP_CLR_FD 0x4C01
#define LOOP_SET_STATUS64 0x4C04
#define LOOP_GET_STATUS64 0x4C05

#endif /* _LINUX_LOOP_H_ */
