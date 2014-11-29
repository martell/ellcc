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
#define  HAVE_VM 1              // Use mem functions for memory management.

#define CONFIG_DIAG_MSGSZ 128   // Diagnostic meessage size.

#define CONFIG_DIAG_SCREEN 0
#define CONFIG_DIAG_BOCHS 0
#define CONFIG_DIAG_QEMU 0
#define CONFIG_DIAG_SERIAL 1
#define CONFIG_NS16550_BASE 0x3F8

#define VEXPRESS_A9 1
//#define INTEGRATORCP 1
#if defined (VERSATILEPB)
#define CONFIG_PL011_BASE 0x0101F1000
#define CONFIG_PL011_IRQ 5
#define PERIPHBASE ?
#define CONFIG_SP804_BASE 0x0101E2000
#elif defined (VEXPRESS_A9)
#define CONFIG_PL011_BASE 0x10009000
#define CONFIG_PL011_PHYSICAL_BASE 0x10009000   // RICH
#define CONFIG_PL011_IRQ 5
#define PERIPHBASE 0x1E000000
#define CONFIG_SP804_BASE 0x10011000
#elif defined(INTEGRATORCP)
#define CONFIG_PL011_BASE 0x16000000
#define CONFIG_PL011_PHYSICAL_BASE 0x16000000   // RICH
#define CONFIG_PL011_IRQ 1
#define PERIPHBASE ?
#define CONFIG_SP804_BASE ?
#else // Newer cores.
#define CONFIG_PL011_BASE 0x1c090000
#define CONFIG_PL011_IRQ 5
#define PERIPHBASE 0x2C000000
#define CONFIG_SP804_BASE 0x1C110000
#endif

#endif
