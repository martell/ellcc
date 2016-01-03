/* Just enough to get iproute2 to compile.
 */
#ifndef _LINUX_POSIX_TYPES_H_
#define _LINUX_POSIX_TYPES_H_

#include <asm/types.h>
#ifndef __kernel_uid32_t
typedef __u32 __kernel_uid32_t;
#endif

#endif /* _LINUX_POSIX_TYPES_H_ */
