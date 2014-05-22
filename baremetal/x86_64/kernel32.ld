/* Linker script.
 *
 * Note: This is written for QEMU
 */

ENTRY(_start)
SECTIONS
{
    . = 0x00100000;
    .init . : { i386.o(.text) }
    . = ALIGN(8);
    .data : { *(.data) }
    . = 0x00110000;
    start = .;
    .kernel : { *(.kernel) }
}

