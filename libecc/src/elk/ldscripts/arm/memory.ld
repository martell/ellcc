/* The ARM memory map.
 *
 * syspage layout:
 *
 * +------------------+ __syspage_base__
 * | Vector page      |
 * +------------------+ +0x1000
 * | Interrupt stack  |
 * +------------------+ +0x2000
 * | Abort mode stack |
 * +------------------+ +0x3000
 * | Undefined stack  |
 * +------------------+ +0x4000
 * | Svc mode stack   |
 * |                  |
 * |                  |
 * +------------------+ +0x6000
 * | Sys mode stack   |
 * |                  |
 * |                  |
 * +------------------+ +0x8000 - No MMU kernel starts here.
 * | PGD for boot     |
 * | (MMU only)       |
 * |                  |
 * +------------------+ +0xC000
 * | PTE0 for boot    |
 * | (MMU only)       |
 * +------------------+ +0xD000
 * | PTE1 for UART I/O|
 * | (MMU only)       |
 * +------------------+ +0xE000 - MMU kernel starts here.
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

// The following are in all syspages.
__intstk_top__ = __syspage_base__ + 0x2000;
__abortstk_top__ = __syspage_base__ + 0x3000;
__undefinedstk_top__ = __syspage_base__ + 0x4000;
__svcstk_top__ = __syspage_base__ + 0x6000;
__sysstk_top__ = __syspage_base__ + 0x8000;

// The following are in MMU syspages only.
__boot_pgd__ = __syspage_base__ + 0x8000;
__boot_pgd_physical__ = __syspage_physical_base__ + 0x8000;
__boot_pte0__ = __syspage_base__ + 0xC000;
__boot_pte0_physical__ = __syspage_physical_base__ + 0xC000;
__boot_pte1__ = __syspage_base__ + 0xD000;
__boot_pte1_physical__ = __syspage_physical_base__ + 0xD000;

// Set up any non-zero physical bases.
#ifdef VEXPRESS_A9
  __syspage_physical_base__ = 0x48000000;
#else
  __syspage_physical_base__ = 0x00000000;
#endif

// The syspage size has to be large enough for the preceeding allocations.
#ifdef MMU
  // MMU systems.
  __syspage_base__ = 0x80000000;
  __syspage_size__ = 0xE000;
  __device_base__ = 0xC0000000;
  __user_limit__ = 0x7FFFFFFF;
  __virtual_offset__ = 0x80000000 - __syspage_physical_base__;
  __dev_offset__ = 0xC0000000;
  __mmu_enabled__ = 1;
#else
  // Non-MMU systems.
  __syspage_base__ = __syspage_physical_base__;
  __syspage_size__ = 0x8000;
  __device_base__ = 0x00000000;
  __user_limit__ = 0xFFFFFFFF;
  __virtual_offset__ = 0x00000000;
  __dev_offset__ = 0x00000000;
  __mmu_enabled__ = 0;
#endif


__kernel_base__ = __syspage_base__;
PROVIDE(load_address = __syspage_physical_base__ + __syspage_size__);
PROVIDE(kernel_start = __syspage_base__ + __syspage_size__);

PROVIDE(stack_heap_size = 4M);          /* RICH: Lose this. */

#ifdef FLASH
  // RICH: need symbolic.
  #ifdef MMU
    #include "ram.ld"
  #else
    MEMORY
    {
      rom (rx) : ORIGIN = 0x00000000, LENGTH = 64M
      ram (! rx) : ORIGIN = 0x48000000, LENGTH = 32M
    }
    #include "romram.ld"
  #endif
#else
  #include "ram.ld"
#endif

__pl011_physical_base__ = 0x10009000;
__pl011_base__ = __pl011_physical_base__ + __dev_offset__;
__pl011_size__ = 0x100000;
__pl011_irq__ = 5;
__pl011_clock__ = 14745600;
__pl011_baud_rate__ = 115200;

__arm_priv_physical_base__ = 0x1E000000;
__arm_priv_base__ = __arm_priv_physical_base__ + __dev_offset__;
__arm_priv_size__ = 0x100000;

__sp804_physical_base__ = 0x10011000;
__sp804_base__ = 0x10011000 + __dev_offset__;
__sp804_size__ = 0x100000;
__sp804_irq__ = 2;
__sp804_clock__ = 1000000; /* HZ */
