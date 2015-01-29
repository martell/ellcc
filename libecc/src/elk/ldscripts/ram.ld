ENTRY(_start)
SECTIONS
{
  /* Program code goes here. */
  .text kernel_start : AT(load_address) { *(.text) }

  /* Handle constructors and destructors. */
  .preinit_array : {
    PROVIDE_HIDDEN(__preinit_array_start = .);
    KEEP(*(.preinit_array))
    PROVIDE_HIDDEN(__preinit_array_end = .);
  }

  .elk_preinit_array : {
    PROVIDE_HIDDEN(__elk_preinit_array_start = .);
    KEEP(*(SORT_BY_INIT_PRIORITY(.elk_preinit_array.*)))
    KEEP(*(.elk_preinit_array))
    PROVIDE_HIDDEN(__elk_preinit_array_end = .);
  }
  .elk_prefini_array  : {
    PROVIDE_HIDDEN(__elk_prefini_array_start = .);
    KEEP(*(SORT_BY_INIT_PRIORITY(.elk_prefini_array.*)))
    KEEP(*(.elk_prefini_array))
    PROVIDE_HIDDEN(__elk_prefini_array_end = .);
  }

  .elk_sysinit_array : {
    PROVIDE_HIDDEN(__elk_sysinit_array_start = .);
    KEEP(*(SORT_BY_INIT_PRIORITY(.elk_sysinit_array.*)))
    KEEP(*(.elk_sysinit_array))
    PROVIDE_HIDDEN(__elk_sysinit_array_end = .);
  }
  .elk_sysfini_array  : {
    PROVIDE_HIDDEN(__elk_sysfini_array_start = .);
    KEEP(*(SORT_BY_INIT_PRIORITY(.elk_sysfini_array.*)))
    KEEP(*(.elk_sysfini_array))
    PROVIDE_HIDDEN(__elk_sysfini_array_end = .);
  }

  .elk_maininit_array : {
    PROVIDE_HIDDEN(__elk_maininit_array_start = .);
    KEEP(*(SORT_BY_INIT_PRIORITY(.elk_maininit_array.*)))
    KEEP(*(.elk_maininit_array))
    PROVIDE_HIDDEN(__elk_maininit_array_end = .);
  }
  .elk_mainfini_array  : {
    PROVIDE_HIDDEN(__elk_mainfini_array_start = .);
    KEEP(*(SORT_BY_INIT_PRIORITY(.elk_mainfini_array.*)))
    KEEP(*(.elk_mainfini_array))
    PROVIDE_HIDDEN(__elk_mainfini_array_end = .);
  }

  .init_array : {
    PROVIDE_HIDDEN(__init_array_start = .);
    KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*) 
           SORT_BY_INIT_PRIORITY(.ctors.*)))
    KEEP(*(.init_array
      EXCLUDE_FILE(*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o ) .ctors))
    PROVIDE_HIDDEN(__init_array_end = .);
  }
  .fini_array  : {
    PROVIDE_HIDDEN(__fini_array_start = .);
    KEEP(*(SORT_BY_INIT_PRIORITY(.fini_array.*)
           SORT_BY_INIT_PRIORITY(.dtors.*)))
    KEEP(*(.fini_array
      EXCLUDE_FILE (*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o ) .dtors))
    PROVIDE_HIDDEN(__fini_array_end = .);
  }

  .ctors : {
    KEEP(*crtbegin.o(.ctors))
    KEEP(*crtbegin?.o(.ctors))
    KEEP(*(EXCLUDE_FILE(*crtend.o *crtend?.o) .ctors))
    KEEP(*(SORT(.ctors.*)))
    KEEP(*(.ctors))
  }
  .dtors : {
    KEEP(*crtbegin.o(.dtors))
    KEEP(*crtbegin?.o(.dtors))
    KEEP(*(EXCLUDE_FILE(*crtend.o *crtend?.o) .dtors))
    KEEP(*(SORT(.dtors.*)))
    KEEP(*(.dtors))
  }
  __text_end__ = .;
  __data_start__ = .;

  HIDDEN (_gp = ALIGN (16) + 0x7ff0);
  .got : { *(.got) }
  .sdata          :
  {
    PROVIDE (_SDA_BASE_ = 32768);
    *(.sdata .sdata.* .gnu.linkonce.s.*)
  }
  .data : {
    *(.data)
  }
  __data_end__ = .;
  . = ALIGN(8);
  /* Define the uninitialized data area. */
  .bss : {
    __bss_start__ = .;
    *(.bss)
    __bss_end__ = .;
  }
  . = ALIGN(8);
  __end = .;                    /* The kernel heap grows from here. */
  . = . + stack_heap_size;
  __mem_top = .;                /* Allocate the stack and heap. */
}
