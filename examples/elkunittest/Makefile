# This is a generic Makefile to build programs using ELLCC
# targeting multiple Linux and ELK (standalone) environments.
# "make help" shows the make targets supported as well as the
# available target configurations.
# This Makefile can be placed a directory containing C and/or C++
# source files. If ELLCC is defined either on the make command
# line or in the environment, it will be used as the name of the
# ELLCC installation directory. Otherwise, if ecc is found in
# the PATH, its parent directory is assumed to be in the toplevel
# of the ELLCC installation directory.

# Define ELLCC if ecc and friends are not in your PATH or if
# ELLCC is not defined in the environment.
# e.g. ELLCC= ~/ellcc

# Define the target processor/environment.

# Set the default configuration.
TARGET=x86_64-linux-eng

# See if a previous configuration exists.
-include config

ecc=$(shell if [ `which ecc 2> /dev/null` ] ; then \
      echo `which ecc`; else echo ""; fi)

# Define the name of the generated program.
PROG=$(shell basename `pwd`)

# Find the ELLCC bin directory.
ifneq ($(ELLCC),)
  ELLCCBIN=$(ELLCC)/bin/
else
  ifneq ($(ecc),)
    ELLCCBIN=$(shell dirname $(ecc))/
  else
    # Make a guess.
    ELLCCBIN=~/ellcc/bin/
  endif
endif

CXXFLAGS += -MD -MP

ifeq ($(VERBOSE),)
  OUT=@
  OUTC=@echo "Compiling $<";
  OUTA=@echo "Assembling $<";
  OUTL=@echo "Linking $(PROG)";
  OUTAR=@echo "Making library $(LIBNAME)";
  OUTR=@echo "Running $(PROG)";
  OUTD=@echo "Debugging $(PROG)";
else
  OUT=
  OUTC=
  OUTA=
  OUTL=
  OUTAR=
  OUTR=
  OUTD=
endif

# Check for a new configuration.
ifneq ("TARGET=$(TARGET)","$(shell cat config)")
  NEEDCLEAN=clean
endif

# Save the current configuration so TARGET does not need to
# be specified on every make command line.
$(shell echo TARGET=$(TARGET) >config)

# Set up the ELLCC commands.
CC=$(ELLCCBIN)ecc
CXX=$(ELLCCBIN)ecc++
ELLCCPREFIX= $(ELLCCBIN)ecc-
AR=$(ELLCCPREFIX)ar rcu
RANLIB=$(ELLCCPREFIX)ranlib
OBJCOPY=$(ELLCCPREFIX)objcopy
GDB=$(ELLCCPREFIX)gdb

# Specify a target for cross compilation and cross linking.
CFLAGS=-target $(TARGET)
CXXFLAGS=-target $(TARGET)
LDFLAGS=-target $(TARGET)

CFLAGS += -g -O3
CXXFLAGS += -g -O3

# Handle any C and C++ source files.
CSRCS=$(wildcard *.c)
CXXSRCS=$(wildcard *.cxx)
CPPSRCS=$(wildcard *.cpp)
OBJS:=$(CSRCS:.c=.o) $(CXXSRCS:.cxx=.o) $(CPPSRCS:.cpp=.o)
CXXSRCS:=$(CXXSRCS) $(CPPSRCS)
# Link for C or C++.
ifneq ("$(CXXSRCS)", " ")
  LD=$(CXX)
else
  LD=$(CC)
endif

$(PROG): $(NEEDCLEAN) $(OBJS)
	$(OUTL)$(LD) $(LDFLAGS) -o $(PROG) $(OBJS)

%.o: %.c
	$(OUTC)$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.cxx
	$(OUTC)$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o: %.cpp
	$(OUTC)$(CXX) $(CXXFLAGS) -c -o $@ $<

# Find out how to run a program in the current configuration.
ARCH=$(shell $(ELLCCBIN)ecc-getarch $(TARGET))
# First, use QEMU Linux user mode for Linux targets.
ifeq ($(findstring linux-,$(TARGET)),linux-)
  RUN=./$(PROG) #$(ELLCCBIN)qemu-$(ARCH) $(PROG)
else
  # Otherwise, set up to run QEMU in system mode for ELK configurations.
  INCARCH=$(shell $(ELLCCBIN)ecc-getincarch $(TARGET))
  QEMUINST=", enter 'control-A x' to exit QEMU";
  MAKEBIN=$(OBJCOPY) -O binary $(PROG) $(PROG).bin;
  ifeq ($(INCARCH),arm)
    RUN=$(QEMUINST)$(ELLCCBIN)qemu-system-$(ARCH) $(QEMUDBG) -M vexpress-a9 \
	  -m 128M -nographic -kernel $(PROG)
  endif
  ifeq ($(INCARCH),i386)
    RUN=$(QEMUINST) $(ELLCCBIN)qemu-system-$(ARCH) $(QEMUDBG) -M pc -nographic \
          -kernel $(PROG)
  endif
  ifeq ($(INCARCH),microblaze)
    RUN=$(QEMUINST) $(ELLCCBIN)qemu-system-$(ARCH) $(QEMUDBG) \
          -M petalogix-s3adsp1800 -nographic -kernel $(PROG)
  endif
  ifeq ($(INCARCH),mips)
    RUN=$(QEMUINST) $(MAKEBIN) $(ELLCCBIN)qemu-system-$(ARCH) $(QEMUDBG) \
          -M malta -nographic -kernel $(PROG).bin
  endif
  ifeq ($(INCARCH),ppc)
    RUN=$(QEMUINST) $(ELLCCBIN)qemu-system-$(ARCH) $(QEMUDBG) -M ppce500 \
          -nographic -kernel $(PROG)
  endif
  ifeq ($(INCARCH),x86_64)
    RUN=$(QEMUINST) $(ELLCCBIN)qemu-system-$(ARCH) $(QEMUDBG) -M pc -nographic \
          -kernel $(PROG)
  endif
endif

run: $(PROG)
	$(OUTR)$(RUN)

log: $(PROG)
	$(OUTR)$(RUN) -d in_asm,cpu -D $(PROG).log

debug:
	@echo "Type 'make gdb' or 'make tui' in another window"
	$(OUTD)$(MAKE) TARGET=$(TARGET) QEMUDBG="-s -S" run

gdb:
	$(GDB) -ex "target remote :1234" $(PROG)

tui:
	$(GDB) -tui -ex "target remote :1234" $(PROG)

covshow: $(PROG)
	$(OUTR)$(MAKE) run
	$(OUT)$(ELLCCBIN)llvm-profdata merge -o $(PROG).profdata default.profraw
	$(OUT)$(ELLCCBIN)llvm-cov show ./$(PROG) \
	  -instr-profile=$(PROG).profdata $(CSRCS) $(CXXSRCS)

covreport: $(PROG)
	$(OUTR)$(MAKE) run
	$(OUT)$(ELLCCBIN)llvm-profdata merge -o $(PROG).profdata default.profraw
	$(OUT)$(ELLCCBIN)llvm-cov report ./$(PROG) \
	  -instr-profile=$(PROG).profdata $(CSRCS) $(CXXSRCS)

help:
	@echo "make                       - build for the default configuration"
	@echo "make TARGET=mips-linux-eng - build for a specific configuration"
	@echo "make run                   - run the program"
	@echo "make debug                 - debug the ELK program"
	@echo "make gdb                   - debug twith gdb"
	@echo "make tui                   - debug with gdb in TUI mode"
	@echo "make covshow               - show coverage information"
	@echo "make covreport             - report coverage results"
	@echo "make clean                 - clean up the directory"
	@echo "make distclean             - really clean up the directory"
	@echo "Available Linux configurations:"
	@cd $(ELLCCBIN)../libecc/config; ls *-linux-*
	@echo "Available ELK configurations:"
	@cd $(ELLCCBIN)../libecc/config; ls *-elk-*

clean:
	rm -f *.o $(PROG) $(PROG).bin $(PROG).log

distclean: clean
	rm config

# Allow "make <config>".
Configs := $(patsubst %.notyet,,$(shell cd $(ELLCCBIN)../libecc/config; echo *))
$(Configs):
	$(OUT)$(MAKE) TARGET=$@

.PHONY: run help clean
