/* Simple Bare metal program - linker script
 * *
 * * Note: This is written for QEMU
 * * QEMU loads the kernel address at 0x00010000
 * * The emulator starts at 0x00000000 and peripheral interrupts
 * * are disabled at startup.
 * *
 * * The code must be compiled and linked to be placed at
 * * 0x00010000
 * *
 * */

ENTRY(_Reset)
SECTIONS
{
    . = 0x10000;
    .init . : { init.o(.text) *(.init) }
    .text : { *(.text) }
    /* Handle constructors and destructors. */
    .preinit_array : {
      PROVIDE_HIDDEN (__preinit_array_start = .);
      KEEP (*(.preinit_array))
      PROVIDE_HIDDEN (__preinit_array_end = .);
    }
    .init_array : {
      PROVIDE_HIDDEN (__init_array_start = .);
      KEEP (*(SORT_BY_INIT_PRIORITY(.init_array.*) 
              SORT_BY_INIT_PRIORITY(.ctors.*)))
      KEEP (*(.init_array
        EXCLUDE_FILE (*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o ) .ctors))
      PROVIDE_HIDDEN (__init_array_end = .);
    }
    .fini_array  : {
      PROVIDE_HIDDEN (__fini_array_start = .);
      KEEP (*(SORT_BY_INIT_PRIORITY(.fini_array.*)
              SORT_BY_INIT_PRIORITY(.dtors.*)))
      KEEP (*(.fini_array
        EXCLUDE_FILE (*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o ) .dtors))
      PROVIDE_HIDDEN (__fini_array_end = .);
    }

    .ctors : {
      KEEP (*crtbegin.o(.ctors))
      KEEP (*crtbegin?.o(.ctors))
      KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .ctors))
      KEEP (*(SORT(.ctors.*)))
      KEEP (*(.ctors))
    }
    .dtors : {
      KEEP (*crtbegin.o(.dtors))
      KEEP (*crtbegin?.o(.dtors))
      KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .dtors))
      KEEP (*(SORT(.dtors.*)))
      KEEP (*(.dtors))
    }

    .data : { *(.data) }
    __bss_start__ = .;
    .bss : { *(.bss) }
    __bss_end__ = .;
    . = ALIGN(8);
    __end = .;                  /* The kernel heap grows from here. */
    . = . + 1024 * 1024;        /* Hard code a megabyte for now. */
    __mem_top = .;              /* Point to the top of memory. */
}

