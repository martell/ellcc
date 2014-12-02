EXTERN(__elk_cons)
EXTERN(__elk_pl011_dev)
EXTERN(__elk_thread)
EXTERN(__elk_time_commands)
EXTERN(__elk_time)
EXTERN(__elk_memman)
EXTERN(__elk_vm)
EXTERN(__elk_ramfs)
EXTERN(__elk_devfs)
EXTERN(__elk_vfs_syscalls)
EXTERN(__elk_test_commands)
EXTERN(__elk_command)
EXTERN(__elk_mkdir_bin)
EXTERN(__elk_ls_bin)
EXTERN(__elk_cat_bin)
EXTERN(__elk_pwd_bin)
EXTERN(__elk_echo_bin)
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
__syspage_base__ = 0x80000000;
__syspage_size__ = 0xE000;
__device_base__ = 0xC0000000;
__user_limit__ = 0x7FFFFFFF;
__virtual_offset__ = 0x80000000 - __syspage_physical_base__;
__dev_offset__ = 0xC0000000;
__mmu_enabled__ = 1;
__kernel_base__ = __syspage_base__;
PROVIDE(load_address = __syspage_physical_base__ + __syspage_size__);
PROVIDE(kernel_start = __syspage_base__ + __syspage_size__);
PROVIDE(stack_heap_size = 4M);
ENTRY(_start)
SECTIONS
{
.text kernel_start : AT(load_address) { *(.text) }
.preinit_array : {
PROVIDE_HIDDEN(__preinit_array_start = .);
KEEP(*(.preinit_array))
PROVIDE_HIDDEN(__preinit_array_end = .);
}
.elk_init_array : {
PROVIDE_HIDDEN(__elk_init_array_start = .);
KEEP(*(SORT_BY_INIT_PRIORITY(.elk_init_array.*)
SORT_BY_INIT_PRIORITY(.elk_ctors.*)))
KEEP(*(.elk_init_array .elk_ctors))
PROVIDE_HIDDEN(__elk_init_array_end = .);
}
.elk_fini_array : {
PROVIDE_HIDDEN(__elk_fini_array_start = .);
KEEP(*(SORT_BY_INIT_PRIORITY(.elk_fini_array.*)
SORT_BY_INIT_PRIORITY(.elk_dtors.*)))
KEEP(*(.elk_fini_array .elk_dtors))
PROVIDE_HIDDEN(__elk_fini_array_end = .);
}
.init_array : {
PROVIDE_HIDDEN(__init_array_start = .);
KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*)
SORT_BY_INIT_PRIORITY(.ctors.*)))
KEEP(*(.init_array
EXCLUDE_FILE(*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o ) .ctors))
PROVIDE_HIDDEN(__init_array_end = .);
}
.fini_array : {
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
.sdata :
{
PROVIDE (_SDA_BASE_ = 32768);
*(.sdata .sdata.* .gnu.linkonce.s.*)
}
.data : {
*(.data)
}
__data_end__ = .;
. = ALIGN(8);
.bss : {
__bss_start__ = .;
*(.bss)
__bss_end__ = .;
}
. = ALIGN(8);
__end = .;
. = . + stack_heap_size;
__mem_top = .;
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
