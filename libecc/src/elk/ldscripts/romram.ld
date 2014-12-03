ENTRY(_start)
SECTIONS
{
  /* Program code goes here. */
  .text : {
    *(.text)
  } > rom

  /* Handle constructors and destructors. */
  .preinit_array : {
    PROVIDE_HIDDEN(__preinit_array_start = .);
    KEEP(*(.preinit_array))
    PROVIDE_HIDDEN(__preinit_array_end = .);
  } > rom

  .elk_init_array : {
    PROVIDE_HIDDEN(__elk_init_array_start = .);
    KEEP(*(SORT_BY_INIT_PRIORITY(.elk_init_array.*) 
           SORT_BY_INIT_PRIORITY(.elk_ctors.*)))
    KEEP(*(.elk_init_array .elk_ctors))
    PROVIDE_HIDDEN(__elk_init_array_end = .);
  } > rom
  .elk_fini_array  : {
    PROVIDE_HIDDEN(__elk_fini_array_start = .);
    KEEP(*(SORT_BY_INIT_PRIORITY(.elk_fini_array.*)
           SORT_BY_INIT_PRIORITY(.elk_dtors.*)))
    KEEP(*(.elk_fini_array .elk_dtors))
    PROVIDE_HIDDEN(__elk_fini_array_end = .);
  } > rom

  .init_array : {
    PROVIDE_HIDDEN(__init_array_start = .);
    KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*) 
           SORT_BY_INIT_PRIORITY(.ctors.*)))
    KEEP(*(.init_array
      EXCLUDE_FILE(*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o ) .ctors))
    PROVIDE_HIDDEN(__init_array_end = .);
  } > rom
  .fini_array  : {
    PROVIDE_HIDDEN(__fini_array_start = .);
    KEEP(*(SORT_BY_INIT_PRIORITY(.fini_array.*)
           SORT_BY_INIT_PRIORITY(.dtors.*)))
    KEEP(*(.fini_array
      EXCLUDE_FILE (*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o ) .dtors))
    PROVIDE_HIDDEN(__fini_array_end = .);
  } > rom

  .ctors : {
    KEEP(*crtbegin.o(.ctors))
    KEEP(*crtbegin?.o(.ctors))
    KEEP(*(EXCLUDE_FILE(*crtend.o *crtend?.o) .ctors))
    KEEP(*(SORT(.ctors.*)))
    KEEP(*(.ctors))
  } > rom
  .dtors : {
    KEEP(*crtbegin.o(.dtors))
    KEEP(*crtbegin?.o(.dtors))
    KEEP(*(EXCLUDE_FILE(*crtend.o *crtend?.o) .dtors))
    KEEP(*(SORT(.dtors.*)))
    KEEP(*(.dtors))
  } > rom
  __text_end__ = .;

  .syspage : {
    __syspage_physical_base__ = . ;
    __syspage_base__ = . ;
    . = . + __syspage_size__ ;
  } > ram
  /* Set up a copy of R/W data in ROM. */
  .data : AT(__text_end__) {
    PROVIDE_HIDDEN(__data_start__ = .);
    *(.data)
  } > ram
  .sdata : AT(.) {
    PROVIDE(_SDA_BASE_ = 32768);
    *(.sdata .sdata.* .gnu.linkonce.s.*)
    PROVIDE_HIDDEN(__data_end__ = .);
  } >ram
  . = ALIGN(8);
  /* Define the uninitialized data area. */
  .bss : {
    PROVIDE_HIDDEN(__bss_start__ = .);
    *(.bss)
    PROVIDE_HIDDEN(__bss_end__ = .);
  } > ram
  . = ALIGN(8);
  __end = .;                            /* The kernel heap grows from here. */
  __mem_top = ORIGIN(ram) + LENGTH(ram);/* Point to the top of memory. */
}


