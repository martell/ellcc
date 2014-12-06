EXTERN(__elk_simple_exit)
EXTERN(__elk_simple_console)
__intstk_top__ = __syspage_base__ + 0x2000;
__abortstk_top__ = __syspage_base__ + 0x3000;
__undefinedstk_top__ = __syspage_base__ + 0x4000;
__sysstk_top__ = __syspage_base__ + 0x8000;
__boot_pgd__ = __syspage_base__ + 0x8000;
__boot_pgd_physical__ = __syspage_physical_base__ + 0x8000;
__boot_pte0__ = __syspage_base__ + 0xC000;
__boot_pte0_physical__ = __syspage_physical_base__ + 0xC000;
__boot_pte1__ = __syspage_base__ + 0xD000;
__boot_pte1_physical__ = __syspage_physical_base__ + 0xD000;
__syspage_physical_base__ = 0x48000000;
__syspage_base__ = __syspage_physical_base__;
__syspage_size__ = 0x8000;
__device_base__ = 0x00000000;
__user_limit__ = 0xFFFFFFFF;
__virtual_offset__ = 0x00000000;
__dev_offset__ = 0x00000000;
__mmu_enabled__ = 0;
__kernel_base__ = __syspage_base__;
PROVIDE(load_address = __syspage_physical_base__ + __syspage_size__);
PROVIDE(kernel_start = __syspage_base__ + __syspage_size__);
PROVIDE(stack_heap_size = 4M);
MEMORY
{
rom (rx) : ORIGIN = 0x00000000, LENGTH = 64M
ram (! rx) : ORIGIN = 0x48000000, LENGTH = 32M
}
ENTRY(_start)
SECTIONS
{
.text : {
*(.text)
} > rom
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
.elk_fini_array : {
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
.fini_array : {
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
.bss : {
PROVIDE_HIDDEN(__bss_start__ = .);
*(.bss)
PROVIDE_HIDDEN(__bss_end__ = .);
} > ram
. = ALIGN(8);
__end = .;
__mem_top = ORIGIN(ram) + LENGTH(ram);
}
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
__sp804_clock__ = 1000000;
