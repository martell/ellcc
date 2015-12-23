/* Just enough to get toybox to compile.
 */
#ifndef _LINUX_FS_H_
#define _LINUX_FS_H_

#define FS_IOC_GETFLAGS _IOR('f', 1, long)
#define FS_IOC_SETFLAGS _IOW('f', 2, long)
#define FS_IOC_GETVERSION _IOR('v', 1, long)
#define FS_IOC_SETVERSION _IOW('v', 2, long)

#define FS_SECRM_FL 0x00000001
#define FS_UNRM_FL 0x00000002
#define FS_COMPR_FL 0x00000004
#define FS_SYNC_FL 0x00000008
#define FS_IMMUTABLE_FL 0x00000010
#define FS_APPEND_FL 0x00000020
#define FS_NODUMP_FL 0x00000040
#define FS_NOATIME_FL 0x00000080
#define FS_INDEX_FL 0x00001000
#define FS_JOURNAL_DATA_FL 0x00004000
#define FS_NOTAIL_FL 0x00008000
#define FS_DIRSYNC_FL 0x00010000
#define FS_TOPDIR_FL 0x00020000

#endif /* _LINUX_FS_H_ */
