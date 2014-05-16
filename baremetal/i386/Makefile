TARGET=i386
KERNEL=kernel
KERNEL_EXE=kernel.elf
SRCS=init.S main.c # simple_console.c command.c simple_memman.c
QEMUARGS=-M pc -nographic -kernel $(KERNEL_EXE)

include ../mk/kernel.mk
