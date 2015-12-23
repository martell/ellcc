/* Just enough to get toybox to compile.
 */
#ifndef _LINUX_NBD_H_
#define _LINUX_NBD_H_

#define NBD_SET_SOCK _IO(0xAB, 0)
#define NBD_SET_BLKSIZE _IO(0xAB, 1)
#define NBD_DO_IT _IO(0xAB, 3)
#define NBD_CLEAR_SOCK  _IO(0xAB, 4)
#define NBD_CLEAR_QUE  _IO(0xAB, 5)
#define NBD_SET_SIZE_BLOCKS _IO(0xAB, 7)
#define NBD_SET_TIMEOUT _IO(0xab, 9)

#endif /* _LINUX_NBD_H_ */
