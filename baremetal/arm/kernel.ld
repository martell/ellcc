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
    .bss : { *(.bss) }
    . = ALIGN(8);
    . = . + 0x1000; 
    __fiq_stack_top = .;        /* Allocate the FIQ stack. */
    . = ALIGN(8);
    . = . + 0x1000; 
    __irq_stack_top = .;        /* Allocate the IRQ stack. */
    . = ALIGN(8);
    . = . + 0x1000; 
    __svc_stack_top = .;        /* Allocate the SVC stack. */
    . = ALIGN(8);
    . = . + 0x1000; 
    __abt_stack_top = .;        /* Allocate the ABT stack. */
    . = ALIGN(8);
    . = . + 0x1000; 
    __und_stack_top = .;        /* Allocate the UND stack. */
    . = ALIGN(8);
    . = . + 0x1000;
    __sys_stack_top = .;        /* Allocate the SYS stack. */
}

