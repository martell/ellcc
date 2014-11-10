# Build ELLCC.
VERSION=0.1.5

ifeq ($(VERBOSE),)
  MFLAGS=--no-print-directory
  OUT=@
else
  MFLAGS=
  OUT=
endif

.PHONY: me
me:
	$(OUT)echo "Building ELLCC for this host."
	$(OUT)./build

.PHONY: all
all: me
	$(OUT)./build -a

.PHONY: release
release:
	$(OUT)echo "Building and packaging ELLCC release $(VERSION)."
	$(OUT)find llvm-build* -name Version.o | xargs rm -f
	$(OUT)svn update
	$(OUT)$(MAKE) $(MFLAGS) \
	  CLANG_VENDOR="ecc $(VERSION) based on" all || exit 1
	$(OUT)./build -p $(VERSION) || exit 1
	$(OUT)echo Enter the ellcc.org svn password
	$(OUT)svn cp -m "Tag release $(VERSION)." \
	  http://ellcc.org/svn/ellcc/trunk \
	  http://ellcc.org/svn/ellcc/tags/ellcc-$(VERSION)
	$(OUT)echo Enter the ellcc.org svn password
	$(OUT)svn cp -m "Tag current release." \
	  http://ellcc.org/svn/ellcc/tags/ellcc-$(VERSION) \
	  http://ellcc.org/svn/ellcc/tags/current
	$(OUT)echo Enter the main password
	$(OUT)scp ../README.txt ChangeLog ellcc-* main:/var/ftp/pub
	$(OUT)ssh main chmod oug+r /var/ftp/pub/\*
	$(OUT)scp ../README.txt ChangeLog ellcc-* main:web/ellcc/releases

.PHONY: macrelease
macrelease:
	$(OUT)echo "Building and packaging Mac OS X ELLCC release $(VERSION)."
	$(OUT)find llvm-build -name Version.o | xargs rm -f
	$(OUT)svn update
	$(OUT)$(MAKE) $(MFLAGS) \
	  CLANG_VENDOR="ecc $(VERSION) based on" || exit 1
	$(OUT)./macmkdist $(VERSION) || exit 1
	$(OUT)echo Enter the main.pennware.com password
	$(OUT)scp ellcc-Mac*-$(VERSION).tgz main.pennware.com:/var/ftp/pub
	$(OUT)ssh main.pennware.com chmod oug+r /var/ftp/pub/\*
	$(OUT)ssh main.pennware.com cp /var/ftp/pub/ellcc-Mac*-$(VERSION).tgz \
                                       web/ellcc/releases

.PHONY: untagrelease
untagrelease:
	$(OUT)svn rm http://ellcc.org/svn/ellcc/tags/ellcc-$(VERSION)

.PHONY: clean
clean:
	$(OUT)rm -fr llvm-build* gnu/gnu-build* libecc/*-build
	$(OUT)$(MAKE) $(MFLAGS) -C gnu/src/qemu clean

# Default to all targets.
TARGETS=
# Limit targets to keep the ecc executable smaller, e.g.:
#TARGETS=--enable-targets=arm

#ifneq ($(TARGET),$(build))
ifneq ($(CC),gcc)
  BUILD=--build=$(build)-$(HOSTOS)
  ifeq ($(MINGW),1)
    HOST=--host=$(TARGET)
    TRGT=--target=$(TARGET)
    ELLCC_ARG0=-DELLCC_ARG0=\\\"x86_64-ellcc-linux\\\"
  else
    HOST=--host=$(TARGET)-$(OS)
    TRGT=--target=$(TARGET)-$(OS)
    ELLCC_ARG0=-DELLCC_ARG0=\\\"$(TUPLE)\\\"
  endif
else
  HOST=
  BUILD=
  ELLCC_ARG0=
endif

ifneq ($(CC),gcc)
  ifeq ($(haslibs),yes)
    CFLAGS=$(ELLCC_ARG0)
    CXXFLAGS=$(ELLCC_ARG0)
    TARGETTUPLE=-target $(TUPLE)
  endif
endif

llvm.configure:
	$(OUT)if [ -e $(DIR)/Makefile ] ; then \
          echo Configuring LLVM for $(TUPLE) aready done ; \
        else  \
          echo Configuring LLVM for $(TUPLE) in $(DIR) ; \
	  cd $(DIR) ; \
	    ../llvm/configure \
	    CC="$(CC) $(TARGETTUPLE)" CFLAGS="$(CFLAGS)" \
	    CPP="$(CC) $(TARGETTUPLE) -E $(CFLAGS)" \
	    CXX="$(CXX) $(TARGETTUPLE)" CXXFLAGS="$(CXXFLAGS)" \
	    CXXCPP="$(CXX) $(TARGETTUPLE) -E $(CFLAGS)" \
	    AR=$(AR) RANLIB=$(RANLIB) \
	    --bindir=$(bindir) --prefix=$(prefix) \
	    $(HOST) $(BUILD) $(TRGT) $(TARGETS) \
	    --enable-shared=no -enable-pic=no --enable-keep-symbols \
	    --enable-optimized --program-prefix= ; \
	fi
