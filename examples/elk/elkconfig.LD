/** ELK Configuration.
 */
#include "config.h"

#define VEXPRESS_A9		// The Versatile Express Cortex-A9 board.

#undef FLASH			// The kernel is in flash.

#define MMU			// Use an MMU.

#define MMAP			// Supply mmap().

#define	FS			// Use a file system.

#define CONSOLE			// Fully functional console.

#define THREADS			// Thread support.

#define TIME			// System time support.

#define ALL_COMMANDS		// Include all commands.

#include "components.ld"
#include "memory.ld"
