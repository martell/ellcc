/* Just enough to get toybox to compile.
 */
#ifndef _LINUX_LOOP_H_
#define _LINUX_LOOP_H_

#include <inttypes.h>
#define LO_NAME_SIZE 64
#define LO_KEY_SIZE 32
struct loop_info64 {
  uint64_t lo_device;
  uint64_t lo_inode;
  uint64_t lo_rdevice;
  uint64_t lo_offset;
  uint64_t lo_sizelimit;
  uint32_t lo_number;
  uint32_t lo_encrypt_type;
  uint32_t lo_encrypt_key_size;
  uint32_t lo_flags;
  uint8_t  lo_file_name[LO_NAME_SIZE];
  uint8_t  lo_crypt_name[LO_NAME_SIZE];
  uint8_t  lo_encrypt_key[LO_KEY_SIZE];
  uint64_t lo_init[2];
};

#define LOOP_SET_FD 0x4C00
#define LOOP_CLR_FD 0x4C01
#define LOOP_SET_STATUS64 0x4C04
#define LOOP_GET_STATUS64 0x4C05

#endif /* _LINUX_LOOP_H_ */
