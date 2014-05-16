TARGET=ppc
KERNEL=kernel
KERNEL_EXE=kernel.elf
SRCS=init.S main.c # simple_console.c command.c simple_memman.c
QEMUARGS=-M ppce500 -nographic -kernel $(KERNEL_EXE)

include ../mk/kernel.mk
