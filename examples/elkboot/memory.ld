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
 *
 * Define the location and size of the read only (rom) and
 * read/write (ram) memory areas.
 * See romram.ld for how output sections are assigned to
 * these memory regions.
 */

__mmu_enabled__ = 0; 
__virtual_offset__ = 0x00000000;
__user_limit__ = 0xFFFFFFFF;
PROVIDE(syspage_size = 0x4000);

MEMORY
{
  rom (rx) : ORIGIN = 0x00000000, LENGTH = 64M
  ram (! rx) : ORIGIN = 0x48000000, LENGTH = 32M
}

INCLUDE romram.ld
