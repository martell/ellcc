# Build ELLCC.

# Get the names of the subdirectories.

CROSSSUBDIRS := \
       gnu-build$(TARGETDIR)/binutils/arm-elf \
       gnu-build$(TARGETDIR)/binutils/ppc64-elf \
       gnu-build$(TARGETDIR)/binutils/i386-elf \
       gnu-build$(TARGETDIR)/binutils/ppc-elf \
       gnu-build$(TARGETDIR)/binutils/microblaze-elf \
       gnu-build$(TARGETDIR)/binutils/sparc-elf \
       gnu-build$(TARGETDIR)/binutils/mips-elf \
       gnu-build$(TARGETDIR)/binutils/x86_64-elf \
       gnu-build$(TARGETDIR)/gdb \
       gnu-build$(TARGETDIR)/binutils

SUBDIRS := \
       $(CROSSSUBDIRS) \
       src/qemu

all install clean veryclean check:: gnu-build$(TARGETDIR)
	@for dir in $(SUBDIRS) ; do \
	  echo Making $@ in $$dir ; \
	  $(MAKE) -C $$dir $@ || exit 1; \
	done

installcross:
	@for dir in $(CROSSSUBDIRS) ; do \
	  echo Making install in $$dir ; \
	  $(MAKE) -C $$dir install || exit 1; \
	done

DOCDIRS := gnu-build$(TARGETDIR)/binutils gnu-build$(TARGETDIR)/gdb

install::
	@for dir in $(DOCDIRS) ; do \
	  echo Making install-html in $$dir ; \
	  $(MAKE) -C $$dir install-html || exit 1; \
	done

install::
	mkdir -p ../libecc/ldscripts/linux
	cp gnu-build$(TARGETDIR)/binutils/ld/ldscripts/* ../libecc/ldscripts/linux/

-include $(prefix)/libecc/mkscripts/targets/$(TARGET)/setup.mk

ifneq ($(TARGET),$(build))
  HOST=--host=$(TARGET)-$(OS)
else
  HOST=
endif

ifneq ($(CC),gcc)
  ifeq ($(haslibs),yes)
    CFLAGS=$(CFLAGS.$(TARGET))
    CXXFLAGS=$(CXXFLAGS.$(TARGET))
  endif
endif

# binutils fails on Mac OS X Mavericks on calls to sbrk() because
# it is deprecated.
ifeq ($(shell uname),Darwin)
  CFLAGS += -Wno-error=deprecated-declarations
endif

binutils.configure:
	cd $(DIR) ; \
    mkdir -p binutils ; \
    cd binutils ; \
    ../../src/binutils/configure \
        CC=$(CC) CFLAGS="$(CFLAGS)" CXX=$(CXX) CXXFLAGS="$(CXXFLAGS)" \
        AR=$(AR) --enable-64-bit-bfd \
        --bindir=$(bindir) $(HOST) --enable-targets=$(targetlist) \
        --program-prefix=ecc- --prefix=$(prefix)/lib \
        --datadir=$(prefix)/lib/share --target=x86_64-unknown-linux

gas.configure:
	cd $(DIR) ; \
	mkdir -p binutils/$(target) ; \
	cd binutils/$(target) ; \
    ../../../src/binutils/gas/configure \
      CC=$(CC) CFLAGS="$(CFLAGS)" CXX="$cxx" CXXFLAGS="$(CXXFLAGS)" AR=$(AR) \
      --bindir=$(bindir) --target=$(target) --program-prefix=${target}- \
      $(HOST) --prefix=$(prefix)/lib --datadir=$(prefix)/lib/share

gdb.configure:
	cd $(DIR) ; \
    mkdir -p gdb ; \
    cd gdb ; \
    ../../src/gdb/configure \
        CC=$(CC) CFLAGS="$(CFLAGS)" CXX=$(CXX) CXXFLAGS="$(CXXFLAGS)" \
	--enable-64-bit-bfd --enable-targets=$(targetlist) \
	--program-prefix=ecc- --prefix=$(prefix)/lib --bindir=$(bindir) \
	$(HOST) --datadir=$(prefix)/lib/share --enable-gdbserver=no \
	--enable-sim=no
