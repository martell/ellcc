/* Definitions for the ARM private memory region.
 */
#define VEXPRESS_A9
#if defined (VERSATILEPB)
#define PERIPHBASE ?
#elif defined (VEXPRESS_A9)
#define PERIPHBASE 0x1E000000
#else // Newer cores.
#define PERIPHBASE 0x2C000000
#endif

#define SCU     0x0000  // Snoop control unit.
#define GIC     0x0100  // Interrupt controller.
#define GTIMER  0x0200  // Global timer.
#define PTIMERS 0x0600  // Private timers and watchdogs.
#define IRQDIST 0x1000  // Interrupt distributor.

#define REG(reg) (*(unsigned int *)(address + (reg)))
#define ADR(reg) ((unsigned int *)(BASE + (reg)))

static volatile unsigned char * const address = (unsigned char *)BASE;


