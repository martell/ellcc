OS=linux
TARGET=arm
CFLAGS+=-g
QEMUFLAGS=-cpu any -M versatilepb -m 128M
KERNEL=kernel
SRCS=init.S startup.c
LIBS=-L $(ELLCC)/libecc/lib/$(TARGET)/$(OS) -lc -lcompiler_rt
NAMES:=$(basename $(filter %.c %.cpp %.S, $(SRCS)))
OBJS:=$(NAMES:%=%.o)

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
	$(LD) $(LDFLAGS) -nostdlib -T $(KERNEL).ld $(OBJS) $(LIBS) \
	    -o $(KERNEL).elf -Wl,--build-id=none
	$(OBJCOPY) -O binary $(KERNEL).elf $(KERNEL).bin

run: $(KERNEL).bin
	$(ELLCC)/bin/qemu-system-$(TARGET) $(QEMUFLAGS) -nographic -kernel $(KERNEL).bin

debug:
	$(ELLCC)/bin/qemu-system-$(TARGET) -s -S $(QEMUFLAGS) -nographic -kernel $(KERNEL).bin

clean:
	rm -f $(OBJS) $(KERNEL).elf $(KERNEL).bin
