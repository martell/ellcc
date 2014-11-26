CFLAGS += -g -Werror -Wall -std=c99 -D_XOPEN_SOURCE=700 -D_BSD_SOURCE
CFLAGS += -I$(SRCPATH)/$(LIB)/sys/$(ARCH) \
          -I$(SRCPATH)/$(LIB)/sys \
          -I$(SRCPATH)/$(LIB)/include

VPATH := $(VPATH):$(SRCPATH)/$(LIB)/$(ARCH)
# Startup code.
CRTSRCS += crt1.S

# Target specific code.
SRCS.arm += arm_gic.c arm_sp804.c
SRCS.i386 += gdt.c idt.c i8259.c
SRCS.microblaze +=
SRCS.mips +=
SRCS.ppc +=
SRCS.x86_64 += idt.c

# Target independent code.
VPATH := $(VPATH):$(SRCPATH)/$(LIB)/kernel
# The ELK kernel.
SRCS += command.c test_commands.c time_commands.c \
	__elk_start.c time.c irq.c nirq.c thread.c file.c fdconsole.c \
	console.c simple_console.c simple_memman.c simple_exit.c \
	device.c

VPATH := $(VPATH):$(SRCPATH)/$(LIB)/mem
# Memory management.
SRCS += kmem.c page.c vm_nommu.c vm.c

# File systems.
VPATH := $(VPATH):$(SRCPATH)/$(LIB)/fs/vfs
#Virtual file system.
SRCS += vfs_mount.c vfs_bio.c vfs_lookup.c vfs_security.c \
        vfs_vnode.c vfs_syscalls.c

VPATH := $(VPATH):$(SRCPATH)/$(LIB)/fs/devfs
# Device file system.
SRCS += devfs_vnops.c

VPATH := $(VPATH):$(SRCPATH)/$(LIB)/fs/ramfs
# RAM file system.
SRCS += ramfs_vnops.c

VPATH := $(VPATH):$(SRCPATH)/$(LIB)/fs/fifofs
# FIFO file system.
SRCS += fifo_vnops.c

#VPATH := $(VPATH):$(SRCPATH)/$(LIB)/fs/binfs
# bin file system.
#SRCS += binfs_vnops.c

# Devices
# Serial devices.
VPATH := $(VPATH):$(SRCPATH)/$(LIB)/dev/serial
# ARM PL011 UART.
SRCS.arm += pl011.c
SRCS += serial.c tty.c cons.c

SRCS += $(SRCS.$(ARCH))
