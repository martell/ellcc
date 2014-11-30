/* The ARM memory map.
 *
 * syspage layout:
 *
 * +------------------+ ___syspage_base__
 * | Vector page      |
 * |                  |
 * +------------------+ +0x1000
 * | Interrupt stack  |
 * |                  |
 * +------------------+ +0x2000
 * | Sys mode stack   |
 * |                  |
 * +------------------+ +0x3000
 * | Boot information |
 * +------------------+ +0x3400
 * | Abort mode stack |
 * +------------------+ +0x3800
 * | Boot stack       |
 * +------------------+ +0x4000 - No MMU kernel starts here.
 * | PGD for boot     |
 * | (MMU only)       |
 * |                  |
 * +------------------+ +0x8000
 * | PTE0 for boot    |
 * | (MMU only)       |
 * +------------------+ +0x9000
 * | PTE1 for UART I/O|
 * | (MMU only)       |
 * +------------------+ +0xA000 - MMU kernel starts here.
 *
 * Note1: Kernel PGD must be stored at 16k aligned address.
 *
 * Note2: PTE0 must be stored at 4k aligned address.
 *
 * Note2: Interrupt stack should be placed after NULL page
 * to detect the stack overflow.
 */

/* MMU enabled. RAM at address zero. */
/*
__syspage_base__ = 0x00000000;
__syspage_physical_base__ = 0x00000000;
PROVIDE(load_address = 0x000A000);
__user_limit__ = 0x7FFFFFFF;
__kernel_base__ = 0x80000000;
PROVIDE(kernel_start = 0x0000A000);
__virtual_offset__ = 0x80000000;
__mmu_enabled__ = 1;
*/

/* No MMU. RAM at address zero. */
/*
__syspage_base__ = 0x00000000;
__syspage_physical_base__ = 0x00000000;
PROVIDE(load_address = 0x00004000);
__user_limit__ = 0xFFFFFFFF;
__kernel_base__ = 0x00000000;
PROVIDE(kernel_start = 0x00004000);
__virtual_offset__ = 0x00000000;
__mmu_enabled__ = 0;
*/

/* vexpress-a9: No MMU. RAM starts at address 0x48000000. */
/*
__syspage_base__ = 0x48000000;
__syspage_physical_base__ = 0x48000000;
PROVIDE(load_address = 0x48004000);
__user_limit__ = 0xFFFFFFFF;
__kernel_base__ = 0x48000000;
PROVIDE(kernel_start = 0x48004000);
__virtual_offset__ = 0x00000000;
__mmu_enabled__ = 0;
*/

/* vexpress-a9: MMU. RAM starts at address 0x48000000. */
__syspage_base__ = 0x80000000;
__syspage_physical_base__ = 0x48000000;
PROVIDE(load_address = 0x4800A000);
__user_limit__ = 0x7FFFFFFF;
__kernel_base__ = 0x80000000;
PROVIDE(kernel_start = 0x8000A000);
__virtual_offset__ = 0x00000000;
__mmu_enabled__ = 1;

PROVIDE(stack_heap_size = 4M);          /* RICH: Lose this. */
INCLUDE ram.ld
