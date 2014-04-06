OS=linux
TARGET=mips
CFLAGS+=-g
QEMUFLAGS=-M malta 
KERNEL=kernel
SRCS=init.S startup.c
LIBS=-L $(ELLCC)/libecc/lib/$(TARGET)/$(OS) -lc -lcompiler_rt
NAMES:=$(basename $(filter %.c %.cpp %.S, $(SRCS)))
OBJS:=$(NAMES:%=%.o)

ELLCC=/home/rich/ellcc
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
	$(ELLCC)/bin/qemu-system-$(TARGET) $(QEMUFLAGS) -nographic -kernel $(KERNEL).elf

debug:
	$(ELLCC)/bin/qemu-system-$(TARGET) -s -S $(QEMUFLAGS) -nographic -kernel $(KERNEL).elf

clean:
	rm -f $(OBJS) $(KERNEL).elf $(KERNEL).bin
