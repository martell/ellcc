/** ELK configuration parameters.
 */
#ifndef _config_h_
#define _config_h_

// General limits.
#define MAX_INPUT 128           // max bytes in terminal input.
#define OPEN_MAX CONFIG_OPEN_MAX// max open files per process.

// The thread module.
#define THREAD_COMMANDS 1       // Enable thread commands.
#define THREADS 1024            // The number of threads supported.
#define PRIORITIES 3            // The number of priorities to support:
                                // (0..PRIORITIES - 1). 0 is highest.
#define PROCESSORS 1            // The number of processors to support.

// The file module.
#define ENABLEFDS 1             // Enable file descriptors.
#define INITFDS 4               // The initial size of an fdset.
#define FDMULTIPLIER 2          // How much to expand an fdset by.

// The network module.
#define ENABLENET 1             // Enable networking.

// The device module.
#define MAXDEVNAME 12           // The device name.

// The file system module.
#define FS_MAX  16              // The maximum number of file system types.
#define CONFIG_BUF_CACHE 32     // The size of the buffer cache.
#define VFS_COMMANDS 1          // Enable vfs commands.
#endif
