TARGET=i386
KERNEL=kernel
KERNEL_EXE=kernel.elf
SRCS=init.S gdt.c idt.c main.c i8259.c \
     simple_console.c command.c simple_memman.c
QEMUARGS=-M pc -nographic -kernel $(KERNEL_EXE)

include ../mk/kernel.mk
