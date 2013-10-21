# Build ELLCC.

# Get the names of the subdirectories.

SUBDIRS := gnu-build$(TARGETDIR)/binutils gnu-build/gdb \
	   gnu-build$(TARGETDIR)/binutils/arm-elf \
	   gnu-build$(TARGETDIR)/binutils/ppc64-elf \
	   gnu-build$(TARGETDIR)/binutils/i386-elf \
	   gnu-build$(TARGETDIR)/binutils/ppc-elf \
	   gnu-build$(TARGETDIR)/binutils/microblaze-elf \
	   gnu-build$(TARGETDIR)/binutils/sparc-elf \
	   gnu-build$(TARGETDIR)/binutils/mips-elf \
	   gnu-build$(TARGETDIR)/binutils/x86_64-elf \
	   src/qemu

all install clean veryclean check:: gnu-build$(TARGETDIR)
	@for dir in $(SUBDIRS) ; do \
	  echo Making $@ in $$dir ; \
	  $(MAKE) -C $$dir $@ || exit 1; \
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

gnu-build$TARGETDIR):
	./configure
