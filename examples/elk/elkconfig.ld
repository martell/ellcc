EXTERN(__elk_cons)
EXTERN(__elk_pl011_dev)
EXTERN(__elk_thread)
EXTERN(__elk_time_commands)
EXTERN(__elk_time)
EXTERN(__elk_memman)
EXTERN(__elk_vm_nommu)
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
