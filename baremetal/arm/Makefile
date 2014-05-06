TARGET=arm
KERNEL=kernel
SRCS=init.S main.c console.c simple_memman.c scheduler.c timer.c \
     arm_sp804.c irq.c arm_gic.c command.c test.c semaphore.c
QEMUARGS=-M vexpress-a9 -m 128M -nographic -kernel $(KERNEL_EXE)

include ../mk/kernel.mk
