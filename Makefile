# Build ELLCC.
VERSION=0.0.2

ifeq ($(VERBOSE),)
  MFLAGS=--no-print-directory
  OUT=@
else
  MFLAGS=
  OUT=
endif

# Get the names of the subdirectories.
SUBDIRS := llvm-build gnu libecc

all install clean veryclean check:: llvm-build gnu/gnu-build
	$(OUT)for dir in $(SUBDIRS) ; do \
	  echo Making $@ in $$dir ; \
	  $(MAKE) $(MFLAGS) CLANG_VENDOR="ellcc $(VERSION)svn based on" -C $$dir $@ || exit 1 ; \
	done

install:: docs

llvm-build gnu/gnu-build:
	./build

.PHONY: buildrelease
buildrelease:
	$(OUT)$(MAKE) $(MFLAGS) -C llvm-build clean
	$(OUT)$(MAKE) $(MFLAGS) CLANG_VENDOR="ellcc $(VERSION) based on" -C llvm-build install || exit 1 ; \

.PHONY: release
release:
	$(OUT)mkdir -p release
	$(OUT)rm -fr ellcc-$(VERSION)
	$(OUT)mkdir -p ellcc-$(VERSION)
	$(OUT)$(MAKE) $(MFLAGS) $(MFLAGS) -C libecc veryclean
	$(OUT)$(MAKE) $(MFLAGS) $(MFLAGS) -C workspace veryclean
	$(OUT)tar --exclude "*.svn*" --exclude "*/test/*" -cvp -f- bin libecc workspace | \
	    (cd ellcc-$(VERSION); tar xfp -)
	$(OUT)(cd ellcc-$(VERSION); tree -T "ELLCC Release Directory Tree" -H ellcc --nolinks > ../tree.html)
	$(OUT)tar -cvpz -frelease/ellcc-$(VERSION)-linux-x86_64.tgz ellcc-$(VERSION)

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
