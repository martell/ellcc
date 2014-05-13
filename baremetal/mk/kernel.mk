all:

VPATH=../kernel
OS=linux
CFLAGS+=-I. -I../kernel -g -MD -MP -Werror -Wall -Wno-unused-function
NAMES:=$(basename $(filter %.c %.cpp %.S, $(SRCS)))
OBJS:=$(NAMES:%=%.o)
DEPENDS:=$(NAMES:%=%.d)
KERNEL_EXE?=$(KERNEL).bin

clean:
	rm -f *.o *.d $(KERNEL).elf $(KERNEL).bin

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

all: $(KERNEL_EXE)

$(KERNEL).elf: Makefile kernel.ld $(OBJS)
	$(LD) $(LDFLAGS) -nostartfiles -T $(KERNEL).ld \
	    $(ELLCC)/libecc/lib/$(TARGET)/$(OS)/crtbegin.o \
	    $(OBJS) \
	    $(ELLCC)/libecc/lib/$(TARGET)/$(OS)/crtend.o \
	    -o $(KERNEL).elf -Wl,--build-id=none

$(KERNEL).bin: $(KERNEL).elf
	$(OBJCOPY) -O binary $(KERNEL).elf $(KERNEL).bin

run: $(KERNEL_EXE)
	$(ELLCC)/bin/qemu-system-$(TARGET) $(QEMUARGS)

debug: $(KERNEL_EXE)
	$(ELLCC)/bin/qemu-system-$(TARGET) -s -S $(QEMUARGS)

gdb:
	$(ELLCC)/bin/ecc-gdb -x ../kernel/gdb.init $(KERNEL).elf

-include $(DEPENDS)

