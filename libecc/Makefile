# Build the ELLCC libraries.

# RICH: TODO: Check out the warnings.
PWD := $(shell pwd)
ELLCC := $(PWD)/..
LIBECC := $(ELLCC)/libecc
Configs := $(patsubst %.notyet,,$(shell cd $(LIBECC)/mkscripts/targets; echo *))
INCLUDES := $(foreach TARGET, $(Configs), $(LIBECC)/mkscripts/targets/$(TARGET)/setup.mk)
OS := linux
CFLAGS := -g -Qunused-arguments \
          -Wno-unneeded-internal-declaration -Wno-cast-align \
	  -Wno-incompatible-pointer-types -Wno-string-plus-int \
	  -Wno-pointer-sign -Wno-array-bounds -Wno-dangling-else \
	  -Wno-int-to-pointer-cast
-include $(INCLUDES)

# Get the clang version.
VERSION := $(shell $(ELLCC)/bin/ecc -v 2>&1 | grep version 2>&1 | sed "s/.*version \([0-9]*\.[0-9]*\).*/\1/")

OBJ := $(PWD)/musl-build
install:header.install \
	$(Configs) \
	other-libs

all:

$(Configs)::
	@echo Making libecc for $@ in musl-build/$@
	mkdir -p $(OBJ)/$@
	./config.musl "$(CFLAGS.$@)" $@ $(ELLCC) || exit 1
	make -C src/musl || exit 1
	make -C src/musl install || exit 1
	make $@.musl.install
	@echo Making libcompiler-rt for $@ in src/compiler-rt
	make -C src/compiler-rt ellcc_linux-$@
	make $@.compiler-rt.install

$(Configs)::
	make $@.ncurses.configure && \
	make $@.ncurses.build && \
	make $@.ncurses.install

# RICH: NOCXX added below because of a Microblaze C++ error.
# Remove when fixed or if Microblaze is removed.

%.ncurses.configure:
	@echo Configuring ncurses for $* in ncurses-build/$*/linux
	mkdir -p ncurses-build/$*/linux
	cd ncurses-build/$*/linux ; \
	NOCXX="--without-cxx-binding" ; \
	../../../src/ncurses/configure \
	    CC="$(ELLCC)/bin/ecc $(CFLAGS.$*)" \
	    CXX="$(ELLCC)/bin/ecc++ $(CXXFLAGS.$*)" \
            AR=$(ELLCC)/bin/ecc-ar \
            RANLIB=$(ELLCC)/bin/ecc-ranlib \
            --host=$(TARGET)-$(OS) \
	    --prefix=$(ELLCC) $$NOCXX

%.ncurses.build:
	make -C ncurses-build/$*/linux

%.ncurses.install:
	@cd ncurses-build ; \
	echo Installing ncurses for $* ; \
	mkdir -p ../lib/$*/linux ; \
	cp $*/linux/lib/*.a ../lib/$*/linux ; \
	mkdir -p ../include/$*/linux ; \
	cp $*/linux/include/*.h ../include/linux ; \
	make -C $*/linux/misc install

$(Configs)::
	make $@.zlib.configure && \
	make $@.zlib.build && \
	make $@.zlib.install

%.zlib.configure:
	@echo Configuring zlib for $* in zlib-build/$*/linux
	mkdir -p zlib-build/$*/linux
	cp -fr src/zlib/* zlib-build/$*/linux
	cd zlib-build/$*/linux ; \
	CC="$(ELLCC)/bin/ecc $(CFLAGS.$*)" \
	CXX="$(ELLCC)/bin/ecc++ $(CXXFLAGS.$*)" \
        AR=$(ELLCC)/bin/ecc-ar \
        RANLIB=$(ELLCC)/bin/ecc-ranlib \
	    ./configure --static --prefix=$(ELLCC)

%.zlib.build:
	make -C zlib-build/$*/linux

%.zlib.install:
	@cd zlib-build ; \
	echo Installing zlib for $* ; \
	mkdir -p ../lib/$*/linux ; \
	cp $*/linux/*.a ../lib/$*/linux ; \
	mkdir -p ../include/$*/linux ; \
	cp $*/linux/zlib.h ../include/linux ; \
	cp $*/linux/zconf.h ../include/$*/linux

$(Configs)::
	make $@.expat.configure && \
	make $@.expat.build && \
	make $@.expat.install

%.expat.configure:
	@echo Configuring expat for $* in expat-build/$*/linux
	mkdir -p expat-build/$*/linux
	cd expat-build/$*/linux ; \
	CC="$(ELLCC)/bin/ecc $(CFLAGS.$*)" \
	CXX="$(ELLCC)/bin/ecc++ $(CXXFLAGS.$*)" \
        AR=$(ELLCC)/bin/ecc-ar \
        RANLIB=$(ELLCC)/bin/ecc-ranlib \
	    ../../../src/expat/configure \
            --host=$(TARGET)-$(OS) \
	        --prefix=$(ELLCC) --enable-shared=no

%.expat.build:
	make -C expat-build/$*/linux

%.expat.install:
	@cd expat-build ; \
	echo Installing expat for $* ; \
	mkdir -p ../lib/$*/linux ; \
	cp $*/linux/.libs/*.a ../lib/$*/linux ; \
	mkdir -p ../include/$*/linux ; \
	cp ../src/expat/lib/expat.h ../include/linux ; \
	cp ../src/expat/lib/expat_external.h ../include/linux

clean:
	make -C src/musl distclean
	make -C src/compiler-rt clean
	make -C obj clean
	rm -fr src/musl/musl.last

veryclean: clean
	make -C obj veryclean
	rm -fr $(OBJ)

install:

header.install:
	@echo Installing ecc header files for version $(VERSION) ; \
	rm -fr clang ; \
	mkdir -p clang ; \
	cp ../lib/clang/$(VERSION)/include/* clang

musl.install:
	@cd musl-build ; \
	for target in `echo *` ; do \
	  make -C .. $$target.musl.install ; \
	done

%.musl.install:
	@cd musl-build ; \
	echo Installing libecc for $* ; \
	mkdir -p ../include/linux ; \
	cp -r $*/include/* ../include/linux ; \
	mkdir -p ../include/$*/linux ; \
	rm -fr ../include/$*/linux/bits ; \
	mv ../include/linux/bits ../include/$*/linux/bits ; \
	mkdir -p ../lib/$*/linux ; \
	cp $*/lib/*.a $*/lib/*.o ../lib/$*/linux

%.compiler-rt.install:
	cd src/compiler-rt/ellcc_linux ; \
	for target in `echo *` ; do \
	  echo Installing libcompiler-rt for $* ; \
	  mkdir -p ../../../lib/$*/linux ; \
	  cp $*/libcompiler_rt.a ../../../lib/$*/linux ; \
          (cd ../../../lib/$*/linux; ln -fs libcompiler_rt.a libgcc.a); \
	done

other-libs:
	mkdir -p include/c++ ; \
	cp src/c++/libcxx/include/* include/c++ ; \
	mkdir -p include/c++/ext ; \
	cp src/c++/libcxx/include/ext/* include/c++/ext ; \
	cp clang/unwind.h include/c++
	cp src/c++/libcxxabi/include/* include/c++ ; \
	make -C obj install
