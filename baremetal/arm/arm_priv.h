/* Definitions for the ARM private memory region.
 */
#define VEXPRESS_A9
#if defined (VERSATILEPB)
#define BASE ?
#elif defined (VEXPRESS_A9)
#define BASE 0x1E000000
#else // Newer cores.
#define BASE 0x2C000000
#endif

#define REG(reg) (*(unsigned int *)(address + (reg)))
#define ADR(reg) ((unsigned int *)(BASE + (reg)))

static volatile unsigned char * const address = (unsigned char *)BASE;


