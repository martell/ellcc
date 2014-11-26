/** ELK configuration parameters.
 */
#ifndef _config_h_
#define _config_h_

#define ELK_NAMESPACE 1         // Use __elk_ prefaced symbols.

#define HAVE_CAPABILITY 1       // Use capabilities.

// General limits.
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
#define NDRIVERS 64             // The maximum number of drivers.

// The file system module.
#define FS_MAX  16              // The maximum number of file system types.
#define CONFIG_BUF_CACHE 32     // The size of the buffer cache.
#define VFS_COMMANDS 1          // Enable vfs commands.

// The tty module.
#define MAX_INPUT 1024          // Maximum input line size.

// IRQ handling.
#define MAXIRQS 32              // The maximum number of IRQs.

// Memory management.
#define  HAVE_VM 0              // Use mem functions for memory management.

// RICH: These will be moved.
#define VEXPRESS_A9
#if defined (VERSATILEPB)
#define CONFIG_PL011_BASE 0x0101F1000
#elif defined (VEXPRESS_A9)
#define CONFIG_PL011_BASE 0x10009000
#else // Newer cores.
#define CONFIG_PL011_BASE 0x1c090000
#endif

#define CONFIG_PL011_IRQ 5

#endif
