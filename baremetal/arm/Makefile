TARGET=arm
KERNEL=kernel
SRCS=init.S main.c console.c simple_memman.c command.c \
     scheduler.c timer.c arm_sp804.c irq.c arm_gic.c semaphore.c \
     test.c
QEMUARGS=-M vexpress-a9 -m 128M -nographic -kernel $(KERNEL_EXE)

include ../mk/kernel.mk
