/* Simple Bare metal program - linker script
 * *
 * * Note: This is written for QEMU
 * * QEMU loads the kernel address at 0x00010000
 * * The emulator starts at 0x00000000 and peripheral interrupts
 * * are disabled at startup.
 * *
 * * The code must be compiled and linked to be placed at
 * * 0x00010000
 * * IRQ handling can be ignored.
 * *
 * */

ENTRY(_Reset)
SECTIONS
{
    . = 0x10000;
    .init . : { init.o(.text) }
    .text : { *(.text) }
    .data : { *(.data) }
    __bss_start__ = .;
    .bss : { *(.bss) }
    __bss_end__ = .;
    . = ALIGN(8);
    . = . + 1024 * 1024;        /* Hard code a megabyte for now. */
    __mem_top = .;              /* Point to the top of memory. */
}

