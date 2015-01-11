/** ELK configuration parameters.
 */
#ifndef _config_h_
#define _config_h_

#define ELK_NAMESPACE 1         // Use __elk_ prefaced symbols.

#define CONFIG_HAVE_CAPABILITY 1                // Use capabilities.

// General limits.

// The thread module.
#define CONFIG_THREADS 1024            		// Number of threads supported.
#define CONFIG_THREAD_COMMANDS 1       		// Enable thread commands.
#define CONFIG_THREAD_SIZE (PAGE_SIZE * 1)      //  The thread page size.

#define CONFIG_PRIORITIES 3                     // The number of priorities:
                                                // (0..CONFIG_PRIORITIES - 1).
                                                // 0 is highest.
#define CONFIG_PROCESSORS 1                     // The number of processors.

// The file module.
#define CONFIG_ENABLEFDS 1                      // Enable file descriptors.
#define CONFIG_INITFDS 4                        // The initial size of an fdset.
#define CONFIG_FDMULTIPLIER 2                   // fdset expansion multiplier.

// The network module.
#define CONFIG_NET_MAX_INET_INTERFACES 16
#define CONFIG_SO_BUFFER_PAGES 64
#define CONFIG_SO_BUFFER_DEFAULT_PAGES 10
#define CONFIG_SO_RCVBUF_MIN 1
#define CONFIG_SO_RCVBUF_MAX CONFIG_SO_BUFFER_PAGES
#define CONFIG_SO_RCVBUF_DEFAULT CONFIG_SO_BUFFER_DEFAULT_PAGES
#define CONFIG_SO_SNDBUF_MIN 1
#define CONFIG_SO_SNDBUF_MAX CONFIG_SO_BUFFER_PAGES
#define CONFIG_SO_SNDBUF_DEFAULT CONFIG_SO_BUFFER_DEFAULT_PAGES
#define CONFIG_INET_COMMANDS 1

// The device module.
#define CONFIG_MAXDEVNAME 12                    // The device name.
#define CONFIG_NDRIVERS 64                      // The max number of drivers.

// The file system module.
#define CONFIG_FS_MAX  16                       // The number of file systems.
#define CONFIG_BUF_CACHE 32                     // The size of the buffer cache.
#define CONFIG_VFS_COMMANDS 1                   // Enable vfs commands.

// The tty module.
#define CONFIG_MAX_INPUT 1024                   // Maximum input line size.

// IRQ handling.
#define CONFIG_MAX_IRQS 32                      // The maximum number of IRQs.

// The virtual memory  module.
#define CONFIG_VM 1                             // Enable virtual memory.
#define CONFIG_VM_COMMANDS 1

#define CONFIG_PM_COMMANDS 1                    // Paged memory alloc commands.
#define CONFIG_KM_COMMANDS 1                    // Kernel heap alloc commands.

#define CONFIG_SIGNALS 1                        // Implement signals.

#define CONFIG_DIAG_MSGSZ 128   // Diagnostic meessage size.

#define CONFIG_DIAG_SCREEN 0
#define CONFIG_DIAG_BOCHS 0
#define CONFIG_DIAG_QEMU 0
#define CONFIG_DIAG_SERIAL 1
#define CONFIG_NS16550_BASE 0x3F8

#define CONF_IMPORT(a) extern char a[]
#define CONF_ADDRESS(a) ((unsigned long)(a))
#define CONF_INT(a) ((int)(intptr_t)(a))
#define CONF_UNSIGNED(a) ((unsigned)(uintptr_t)(a))
#define CONF_SIZE(a) ((size_t)(uintptr_t)(a))

/* Configuration macros.
 * These are used in ELK .cfg files for link-time configuration.
 */

// Add a feature to a binary.
#define ADD_FEATURE(feature) EXTERN(__elk_ ## feature)

#endif
