# The x86_64 is special: it QEMU starts in i386 protected mode.
# Need to look like an i386 program to multiboot aware loaders.
TARGET=x86_64
KERNEL=kernel
KERNEL_EXE=kernel.elf
SRCS= init.S # simple_console.c command.c simple_memman.c
QEMUARGS=-M pc -nographic -kernel $(KERNEL_EXE)

all:

VPATH=../kernel
OS=linux
CFLAGS+=-I. -I../kernel -g -MD -MP -Werror -Wall -Wno-unused-function
NAMES:=$(basename $(filter %.c %.cpp %.S, $(SRCS)))
OBJS:=$(NAMES:%=%.o) $(filter %.o, $(SRCS))
DEPENDS:=$(NAMES:%=%.d)
KERNEL_EXE?=$(KERNEL).bin

clean:
	rm -f *.o *.d $(KERNEL).elf $(KERNEL)64.elf $(KERNEL).bin

ELLCC=../..
CC=$(ELLCC)/bin/ecc
AS=$(ELLCC)/bin/ecc
LD=$(ELLCC)/bin/ecc
OBJCOPY=$(ELLCC)/bin/ecc-objcopy

include $(ELLCC)/libecc/mkscripts/targets/$(TARGET)/setup.mk

.S.o:
	$(AS) $(ASFLAGS.$(TARGET)) -c $<

.c.o:
	$(CC) $(CFLAGS.$(TARGET)) -c $<

i386.o: i386.S
	$(CC) -target i386-ellcc-linux -c i386.S

all: $(KERNEL_EXE)

$(KERNEL).elf: Makefile kernel32.ld i386.o kernel.bin
	$(ELLCC)/bin/ecc-objcopy --remove-section .kernel i386.o
	$(ELLCC)/bin/ecc-objcopy --add-section .kernel=$(KERNEL).bin \
	    --set-section-flag .kernel=alloc,data,load,contents i386.o
	$(LD) -target i386-ellcc-linux -nostartfiles -T kernel32.ld \
	    i386.o \
	    -o $(KERNEL).elf -Wl,--build-id=none

$(KERNEL)64.elf: Makefile $(OBJS)
	$(LD) $(LDFLAGS) -nostartfiles -T kernel.ld \
	    $(ELLCC)/libecc/lib/$(TARGET)/$(OS)/crtbegin.o \
	    $(OBJS) \
	    $(ELLCC)/libecc/lib/$(TARGET)/$(OS)/crtend.o \
	    -o $(KERNEL)64.elf -Wl,--build-id=none

$(KERNEL).bin: $(KERNEL)64.elf
	$(OBJCOPY) -O binary $(KERNEL)64.elf $(KERNEL).bin

run: $(KERNEL_EXE)
	$(ELLCC)/bin/qemu-system-$(TARGET) $(QEMUARGS)

debug32: $(KERNEL_EXE)
	$(ELLCC)/bin/qemu-system-$(TARGET) -s -S $(QEMUARGS)

gdb32:
	$(ELLCC)/bin/ecc-gdb -x ../kernel/gdb.init $(KERNEL).elf

tui32:
	$(ELLCC)/bin/ecc-gdb -tui -x ../kernel/gdb.init $(KERNEL).elf

debug: $(KERNEL_EXE)
	$(ELLCC)/bin/qemu-system-$(TARGET) -s $(QEMUARGS)

gdb:
	$(ELLCC)/bin/ecc-gdb -x gdb.init $(KERNEL)64.elf

tui:
	$(ELLCC)/bin/ecc-gdb -tui -x gdb.init $(KERNEL)64.elf
-include $(DEPENDS)

