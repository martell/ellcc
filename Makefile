# Build ELLCC.
VERSION=0.1.30
# Debug Release RelWithDebInfo
BUILD_TYPE=Debug

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
	$(OUT)svn cp -m "Tag release $(VERSION)." \
	  http://ellcc.org/svn/ellcc/trunk \
	  http://ellcc.org/svn/ellcc/tags/ellcc-$(VERSION)
	$(OUT)echo Run '"make sendrelease"' to update download sites

.PHONY: sendrelease
sendrelease:
	$(OUT)echo Enter the ellcc.org svn password
	$(OUT)svn rm -m "Remove current." \
	  http://ellcc.org/svn/ellcc/tags/current
	$(OUT)svn cp -m "Tag current release." \
	  http://ellcc.org/svn/ellcc/tags/ellcc-$(VERSION) \
	  http://ellcc.org/svn/ellcc/tags/current
	$(OUT)echo Enter the ellcc.org password
	$(OUT)scp ../README.txt ChangeLog ellcc-*-$(VERSION).tgz ellcc.org:/var/ftp/pub
	$(OUT)ssh ellcc.org chmod oug+r /var/ftp/pub/\*
	$(OUT)ssh ellcc.org cp  /var/ftp/pub/README.txt \
          /var/ftp/pub/ChangeLog \
	  /var/ftp/pub/ellcc-*-$(VERSION).tgz web/ellcc/releases

.PHONY: macrelease
macrelease:
	$(OUT)echo "Building and packaging Mac OS X ELLCC release $(VERSION)."
	$(OUT)find llvm-build -name Version.o | xargs rm -f
	$(OUT)svn update
	$(OUT)$(MAKE) $(MFLAGS) \
	  CLANG_VENDOR="ecc $(VERSION) based on" || exit 1
	$(OUT)./macmkdist $(VERSION) || exit 1
	$(OUT)echo Run '"make sendmacrelease"' to update download sites

.PHONY: sendmacrelease
sendmacrelease:
	$(OUT)echo Enter the ellcc.org password
	$(OUT)scp ellcc-Mac*-$(VERSION).tgz ellcc.org:/var/ftp/pub
	$(OUT)ssh ellcc.org chmod oug+r /var/ftp/pub/\*
	$(OUT)ssh ellcc.org cp /var/ftp/pub/ellcc-Mac*-$(VERSION).tgz \
                                       web/ellcc/releases
.PHONY: tagrelease
tagrelease:
	$(OUT)svn cp -m "Tag release $(VERSION)." \
	  http://ellcc.org/svn/ellcc/trunk \
	  http://ellcc.org/svn/ellcc/tags/ellcc-$(VERSION)

.PHONY: untagrelease
untagrelease:
	-$(OUT)svn rm -m "Revert." http://ellcc.org/svn/ellcc/tags/ellcc-$(VERSION)
	-$(OUT)svn rm -m "Revert." http://ellcc.org/svn/ellcc/tags/current

.PHONY: clean
clean:
	$(OUT)rm -fr llvm-build* gnu/gnu-build* libecc/*-build
	$(OUT)$(MAKE) $(MFLAGS) -C gnu/src/qemu clean

# Default to all targets.
TARGETS=
# Limit targets to keep the ecc executable smaller, e.g.:
#TARGETS=--enable-targets=arm

ifneq ($(CC),gcc)
  BUILD=--build=$(build)-$(HOSTOS)
  HOST=--host=$(TARGET)-$(OS)
  TRGT=--target=$(TARGET)-$(OS)
  ELLCC_ARG0=-DELLCC_ARG0=\\\"$(TUPLE)\\\"
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
	    CC="$(CC) $(TARGETTUPLE)" \
	    CXX="$(CXX) $(TARGETTUPLE)" \
	    cmake ../llvm \
	      $(cross_cmake_flags) \
	      -DCMAKE_AR=$(AR) -DCMAKE_RANLIB=$(RANLIB) \
	      -DCMAKE_C_FLAGS="$(CFLAGS)" \
	      -DCMAKE_CXX_FLAGS="$(CXXFLAGS)" \
	      -DLLVM_ENABLE_PIC=False \
	      -DCMAKE_BUILD_TYPE:STRING=$(BUILD_TYPE) \
	      -DCMAKE_INSTALL_PREFIX=$(install) ; \
	fi
