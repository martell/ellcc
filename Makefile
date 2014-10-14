# Build ELLCC.
VERSION=0.1.0

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
	  http://ellcc.org/svn/ellcc/trunk http://ellcc.org/svn/ellcc/tags/ellcc-$(VERSION)


.PHONY: clean
clean:
	$(OUT)rm -fr llvm-build* gnu/gnu-build* libecc/*-build
	$(OUT)$(MAKE) $(MFLAGS) -C gnu/src/qemu clean

install:: docs

#.PHONY: release
#release:
#	$(OUT)mkdir -p release
#	$(OUT)rm -fr ellcc-$(VERSION)
#	$(OUT)mkdir -p ellcc-$(VERSION)
#	$(OUT)$(MAKE) $(MFLAGS) $(MFLAGS) -C libecc veryclean
#	$(OUT)$(MAKE) $(MFLAGS) $(MFLAGS) -C workspace veryclean
#	$(OUT)tar --exclude "*.svn*" --exclude "*/test/*" -cvp -f- bin libecc workspace | \
#	    (cd ellcc-$(VERSION); tar xfp -)
#	$(OUT)(cd ellcc-$(VERSION); tree -T "ELLCC Release Directory Tree" -H ellcc --nolinks > ../tree.html)
#	$(OUT)tar -cvpz -frelease/ellcc-$(VERSION)-linux-x86_64.tgz ellcc-$(VERSION)

.PHONY: tagrelease
tagrelease:
	$(OUT)svn cp -m "Tag release $(VERSION)." http://ellcc.org/svn/ellcc/trunk http://ellcc.org/svn/ellcc/tags/ellcc-$(VERSION)

.PHONY: untagrelease
untagrelease:
	$(OUT)svn rm http://ellcc.org/svn/ellcc/trunk http://ellcc.org/svn/ellcc/tags/ellcc-$(VERSION)

.PHONY: docs
docs:
	$(OUT)cp -r ./lib/share/doc/ld.html \
	./lib/share/doc/gdb \
	./lib/share/doc/binutils.html \
	./lib/share/doc/as.html \
	./share/doc/qemu \
	libecc/doc

# Default to all targets.
TARGETS=
# Limit targets to keep the ecc executable smaller, e.g.:
#TARGETS=--enable-targets=arm

ifneq ($(TARGET),$(build))
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
