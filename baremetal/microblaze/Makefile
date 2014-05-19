TARGET=microblaze
KERNEL=kernel
KERNEL_EXE=kernel.elf
SRCS=init.S main.c # simple_console.c command.c simple_memman.c
QEMUARGS=-M petalogix-s3adsp1800 -nographic -kernel $(KERNEL_EXE)

include ../mk/kernel.mk
