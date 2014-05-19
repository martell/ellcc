# The x86_64 is special: it QEMU starts in i386 protected mode.
# Need to look like an i386 program to multiboot aware loaders.
TARGET=i386
KERNEL=kernel
KERNEL_EXE=kernel.elf
SRCS=i386.S # simple_console.c command.c simple_memman.c
QEMUARGS=-M pc -nographic -kernel $(KERNEL_EXE)

include ../mk/kernel.mk
