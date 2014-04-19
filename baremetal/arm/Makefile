OS=linux
TARGET=arm
CFLAGS+=-g -MD -MP -Werror -Wall -Wno-unused-function
QEMUFLAGS=-cpu any -M versatilepb -m 128M
KERNEL=kernel
SRCS=init.S main.c simple_console.c simple_memman.c scheduler.c
NAMES:=$(basename $(filter %.c %.cpp %.S, $(SRCS)))
OBJS:=$(NAMES:%=%.o)
DEPENDS:=$(NAMES:%=%.d)

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

all: $(KERNEL).bin

$(KERNEL).bin: Makefile kernel.ld $(OBJS)
	$(LD) $(LDFLAGS) -nostartfiles -T $(KERNEL).ld \
	    $(ELLCC)/libecc/lib/$(TARGET)/$(OS)/crtbegin.o \
	    $(OBJS) \
	    $(ELLCC)/libecc/lib/$(TARGET)/$(OS)/crtend.o \
	    -o $(KERNEL).elf -Wl,--build-id=none
	$(OBJCOPY) -O binary $(KERNEL).elf $(KERNEL).bin

run: $(KERNEL).bin
	$(ELLCC)/bin/qemu-system-$(TARGET) $(QEMUFLAGS) -nographic -kernel $(KERNEL).bin

debug:
	$(ELLCC)/bin/qemu-system-$(TARGET) -s -S $(QEMUFLAGS) -nographic -kernel $(KERNEL).bin

clean:
	rm -f $(OBJS) $(KERNEL).elf $(KERNEL).bin $(DEPENDS)

-include $(DEPENDS)
