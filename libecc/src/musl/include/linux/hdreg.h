/* Just enough to get ntfs-3g to compile.
 */
#ifndef _LINUX_HDREG_H_
#define _LINUX_HDREG_H_
struct hd_geometry
{
  unsigned char heads;
  unsigned char sectors;
  unsigned short cylinders;
  unsigned long start;
};
#endif /* _LINUX_HDREG_H_ */

