/* Linker script.
 *
 * Note: This is written for QEMU
 * QEMU loads the kernel address at 0x80100400
 */

ENTRY(_start)
SECTIONS
{
    . = 0x00003000;
    .init . : { init.o(.text) }
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

    HIDDEN (_gp = ALIGN (16) + 0x7ff0);
    .got : { *(.got) }
    .data : { *(.data) }
    . = ALIGN(8);
    __bss_start__ = .;
    .bss : { *(.bss) }
    __bss_end__ = .;
    . = ALIGN(8);
    __end = .;                  /* The kernel heap grows from here. */
    . = . + 1024 * 1024;        /* Hard code a megabyte for now. */
    __mem_top = .;              /* Point to the top of memory. */
}

