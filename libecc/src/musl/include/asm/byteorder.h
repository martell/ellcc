/* Just enough to get iproute2 to compile.
 */
#ifndef _ASM_BYTEORDER_H_
#define _ASM_BYTEORDER_H_

#include <sys/endian.h>
#define __cpu_to_be16(x) htobe16((x))
#define ___constant_swab32(v) ((__u32)( \
 (((__u32)(v) & (__u32)0x000000FFUL) << 24) | \
 (((__u32)(v) & (__u32)0x0000FF00UL) <<  8) | \
 (((__u32)(v) & (__u32)0x00FF0000UL) >>  8) | \
 (((__u32)(v) & (__u32)0xFF000000UL) >> 24)))

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define __LITTLE_ENDIAN_BITFIELD
#define __constant_cpu_to_be32(x) ((__u32)___constant_swab32((x)))
#else
#define __BIG_ENDIAN_BITFIELD
define __constant_cpu_to_be32(x) ((__u32)(x))
#endif

#endif /* _ASM_BYTEORDER_H_ */
