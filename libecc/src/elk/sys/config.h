/** ELK configuration parameters.
 */
#ifndef _config_h_
#define _config_h_

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

#endif
