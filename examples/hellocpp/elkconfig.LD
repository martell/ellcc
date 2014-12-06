/** The simplest ELK Configuration with I/O.
 */
#include "config.h"

#define VEXPRESS_A9		// The Versatile Express Cortex-A9 board.

#define FLASH			// The kernel is in flash.

#undef MMU			// Use an MMU.

#undef MMAP			// Supply mmap().

#undef	FS			// Use a file system.

#undef CONSOLE			// Fully functional console.
#define SIMPLE_CONSOLE          // Simple polled console.
#define SIMPLE_EXIT             // Simple exit handling.

#undef THREADS			// Thread support.

#undef TIME			// System time support.

#undef ALL_COMMANDS		// Include all commands.

#include "components.ld"
#include "memory.ld"
