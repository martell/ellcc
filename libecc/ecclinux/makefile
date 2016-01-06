# Cross compiling variables
ARCH		?= arm
TARGET		?= arm-linux-engeabihf

ELLCC		:= $(shell realpath $(shell pwd)/../../..)
HOSTCC		?= $(ELLCC)/bin/ecc
CC		:= $(ELLCC)/bin/ecc -target $(TARGET) -I$(ELLCC)/libecc/include/$(ARCH)

CPP		:= $(CC) -E
LD		:= $(ELLCC)/bin/ecc-ld -m armelf_linux_eabi
OBJCOPY		:= $(ELLCC)/bin/ecc-objcopy
OBJDUMP                := $(ELLCC)/bin/ecc-objdump
AR             := $(ELLCC)/bin/ecc-ar
NM             := $(ELLCC)/bin/ecc-nm
STRIP          := $(ELLCC)/bin/ecc-strip
SIZE           := $(ELLCC)/bin/ecc-size

MAXJOBS		?= 1

ifdef ARCH
export ARCH
endif
ifdef CROSS_COMPILE
export CROSS_COMPILE
endif
ifdef CC
export CC
endif
ifdef CPP
export CPP
endif
ifdef LD
export LD
endif
ifdef HOSTCC
export HOSTCC
endif
 ifdef HOSTCC
 export HOSTCC
 endif
ifdef OBJCOPY
export OBJCOPY
endif
ifdef OBJDUMP
export OBJDUMP
endif
ifdef AR
export AR
endif
ifdef NM
export NM
endif
ifdef STRIP
export STRIP
endif
ifdef SIZE
export SIZE
endif

ifdef O
export O
endif

# This allows you to compile out of tree depending on ARCH and CC

# Remove spaces from CC
#empty		:=
#space		:= $(empty) $(empty)
#CC_NOSPACES	:= $(subst $(space),_,$(CC))

# Where to put build output
#ARCH		?= x86
#O		:= build/$(ARCH)-$(CC_NOSPACES)
O		:= $(BUILD)

LINUXFLAGS	= CPP="$(CPP)" CC="$(CC)" HOSTCC="$(HOSTCC)" LD="$(LD)" \
                  OBJCOPY="$(OBJCOPY)" OBJDUMP="$(OBJDUMP)" AR="$(AR)" \
                  NM="$(NM)" STRIP="$(STRIP)" SIZE="$(SIZE)"

linuxmake	= $(MAKE) -j $(MAXJOBS) -f Makefile $(LINUXFLAGS) O=$(1) $(2)

%:
	@$(call linuxmake,$(O),$@)

%/: force
	@$(call linuxmake,$(O),$@)

%.o: %.c
	@$(call linuxmake,$(O),$@)

%.i: %.c
	@$(call linuxmake,$(O),$@)

%.s: %.c
	@$(call linuxmake,$(O),$@)

%.s: %.S
	@$(call linuxmake,$(O),$@)

all:
	@$(call linuxmake,$(O),$@)

.PHONY: force
force: ;
	:
