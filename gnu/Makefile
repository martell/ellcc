# Build GNU tools.

ifeq ($(VERBOSE),)
  MFLAGS=--no-print-directory
  OUT=@
else
  MFLAGS=
  OUT=
endif

# Get the names of the subdirectories.

CROSSSUBDIRS := \
       gnu-build$(TARGETDIR)/binutils/aarch64-elf \
       gnu-build$(TARGETDIR)/binutils/arm-elf \
       gnu-build$(TARGETDIR)/binutils/i386-elf \
       gnu-build$(TARGETDIR)/binutils/microblaze-elf \
       gnu-build$(TARGETDIR)/binutils/mips-elf \
       gnu-build$(TARGETDIR)/binutils/nios2-elf \
       gnu-build$(TARGETDIR)/binutils/ppc-elf \
       gnu-build$(TARGETDIR)/binutils/ppc64-elf \
       gnu-build$(TARGETDIR)/binutils/sparc-elf \
       gnu-build$(TARGETDIR)/binutils/x86_64-elf \
       gnu-build$(TARGETDIR)/gdb \
       gnu-build$(TARGETDIR)/binutils

SUBDIRS := \
       $(CROSSSUBDIRS) \
       src/qemu

all install clean veryclean check:: gnu-build$(TARGETDIR)
	$(OUT)for dir in $(SUBDIRS) ; do \
	  echo Making $@ in $$dir ; \
	  $(MAKE) -C $$dir $@ || exit 1; \
	done

installcross:
	$(OUT)for dir in $(CROSSSUBDIRS) ; do \
	  echo Making install in $$dir ; \
	  $(MAKE) -C $$dir install || exit 1; \
	done

DOCDIRS := gnu-build$(TARGETDIR)/binutils gnu-build$(TARGETDIR)/gdb

install::
	$(OUT)for dir in $(DOCDIRS) ; do \
	  echo Making install-html in $$dir ; \
	  $(MAKE) -C $$dir install-html || exit 1; \
	done

ifneq ($(TARGET),$(build))
  ifeq ($(MINGW),1)
    HOST=--host=$(TARGET)
  else
    HOST=--host=$(TARGET)-$(OS)
  endif
else
  HOST=
endif

ifneq ($(MINGW),1)
  ifneq ($(CC),gcc)
    ifeq ($(haslibs),yes)
      override CPP := "$(CC) -target $(TUPLE) -E"
      override CC := "$(CC) -target $(TUPLE)"
      override CXX := "$(CXX) -target $(TUPLE)"
    endif
  else
      override CPP := "$(CC) -E"
  endif
else
    override CPP := "$(CC) -E"
endif

# binutils fails on Mac OS X Mavericks on calls to sbrk() because
# it is deprecated.
ifeq ($(shell uname),Darwin)
  CFLAGS += -Wno-error=deprecated-declarations
endif

ifneq ($(CC),gcc)
  ifneq ($(MINGW),1)
    CFLAGS += -Wno-error=pointer-bool-conversion
  endif
endif

binutils.configure:
	$(OUT)if [ -e $(DIR)/binutils/Makefile ] ; then \
          echo Configuring binutils for $(TUPLE) aready done ; \
        else  \
          echo Configuring binutils for $(TUPLE) in $(DIR)/binutils ; \
	  cd $(DIR) ; \
	  mkdir -p binutils ; \
	  cd binutils ; \
	  ../../src/binutils/configure \
	    CC=$(CC) CFLAGS="$(CFLAGS)" CXX=$(CXX) CXXFLAGS="$(CXXFLAGS)" \
	    CPP=$(CPP) \
	    AR=$(AR) --enable-64-bit-bfd \
	    --bindir=$(bindir) $(HOST) --enable-targets=$(targetlist) \
	    --program-prefix=ecc- --prefix=$(prefix)/lib --enable-plugins=no \
	    --datadir=$(prefix)/lib/share --target=x86_64-unknown-linux ; \
	fi

gas.configure:
	$(OUT)if [ -e $(DIR)/binutils/$(target)/Makefile ] ; then \
          echo Configuring gas for $(TUPLE) aready done ; \
        else  \
          echo Configuring gas for $(TUPLE) in $(DIR)binutils/$(target) ; \
	  cd $(DIR) ; \
	  mkdir -p binutils/$(target) ; \
	  cd binutils/$(target) ; \
	  ../../../src/binutils/gas/configure \
	    CC=$(CC) CFLAGS="$(CFLAGS)" CXX=$(CXX) CXXFLAGS="$(CXXFLAGS)" \
	    AR=$(AR) CPP=$(CPP) \
	    --bindir=$(bindir) --target=$(target) --program-prefix=${target}- \
	    $(HOST) --prefix=$(prefix)/lib --datadir=$(prefix)/lib/share ; \
	fi

gdb.configure:
	$(OUT)if [ -e $(DIR)/gdb/Makefile ] ; then \
          echo Configuring gdb for $(TUPLE) aready done ; \
        else  \
          echo Configuring gdb for $(TUPLE) in $(DIR)/gdb ; \
	  cd $(DIR) ; \
	  mkdir -p gdb ; \
	  cd gdb ; \
	  ../../src/gdb/configure \
	    CC=$(CC) CFLAGS="$(CFLAGS)" CXX=$(CXX) CXXFLAGS="$(CXXFLAGS)" \
	    CPP=$(CPP) \
	    --enable-64-bit-bfd --enable-targets=$(targetlist) \
	    --program-prefix=ecc- --prefix=$(prefix)/lib --bindir=$(bindir) \
	    $(HOST) --datadir=$(prefix)/lib/share --enable-gdbserver=no \
	    --enable-sim=no ; \
	fi

qemu.configure:
	$(OUT)if [ -e $(DIR)/config.status ] ; then \
          echo Configuring qemu for $(TUPLE) aready done ; \
        else  \
          echo Configuring qemu for $(TUPLE) in $(DIR) ; \
	  cd $(DIR) ; \
	  ./configure --cc="$(CC)" --host-cc="$(HCC)" --extra-cflags="" \
	    --prefix=$(prefix) \
	    --disable-strip --target-list="$(targetlist)" --disable-guest-agent \
	    --disable-tools ; \
	fi
