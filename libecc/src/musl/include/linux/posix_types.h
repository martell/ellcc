/* Just enough to get iproute2 and u-boot to compile.
 */
#ifndef _LINUX_POSIX_TYPES_H_
#define _LINUX_POSIX_TYPES_H_

#include <asm/types.h>
#ifndef __kernel_uid32_t
typedef __u32 __kernel_uid32_t;
typedef __u64 __kernel_ino_t;
typedef int __kernel_daddr_t;
#endif

#endif /* _LINUX_POSIX_TYPES_H_ */
