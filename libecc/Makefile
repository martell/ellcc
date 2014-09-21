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

OBJ := $(PWD)/musl-build
install: $(Configs) other-libs clang-headers

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
	cp $*/linux/include/*.h ../include ; \
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
	cp $*/linux/zlib.h ../include ; \
	cp $*/linux/zconf.h ../include

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
	cp ../src/expat/lib/expat.h ../include ; \
	cp ../src/expat/lib/expat_external.h ../include

$(Configs)::
	make $@.libedit.configure && \
	make $@.libedit.build && \
	make $@.libedit.install

%.libedit.configure:
	@echo Configuring libedit for $* in libedit-build/$*/linux
	mkdir -p libedit-build/$*/linux
	cp -fr src/libedit/* libedit-build/$*/linux
	cd libedit-build/$*/linux ; \
	CC="$(ELLCC)/bin/ecc $(CFLAGS.$*)" \
	CXX="$(ELLCC)/bin/ecc++ $(CXXFLAGS.$*)" \
        AR=$(ELLCC)/bin/ecc-ar \
        RANLIB=$(ELLCC)/bin/ecc-ranlib \
	    ../../../src/libedit/configure \
	    --enable-shared=no  --prefix=$(ELLCC) \
            --host=$(TARGET)-$(OS)

%.libedit.build:
	make -C libedit-build/$*/linux

%.libedit.install:
	@cd libedit-build ; \
	echo Installing libedit for $* ; \
	mkdir -p ../lib/$*/linux ; \
	cp $*/linux/src/.libs/*.a ../lib/$*/linux ; \
	cp $*/linux/src/histedit.h ../include ; \
	cp $*/linux/src/editline/readline.h ../include

clean:
	make -C src/musl distclean
	make -C src/compiler-rt clean
	make -C obj veryclean
	rm -fr src/musl/musl.last *-build

veryclean: clean
	make -C obj veryclean
	rm -fr $(OBJ)

install:

musl.install:
	@cd musl-build ; \
	for target in `echo *` ; do \
	  make -C .. $$target.musl.install ; \
	done

%.musl.install:
	@cd musl-build ; \
	echo Installing libecc for $* ; \
	mkdir -p ../include ; \
	cp -r $*/include/* ../include ; \
	ln -sf ../endian.h ../include/sys ; \
	mkdir -p ../include/`../getarch $*` ; \
	rm -fr ../include/`../getarch $*`/bits ; \
	mv ../include/bits ../include/`../getarch $*`/bits ; \
	(cd ../include/`../getarch $*`; rm -rf machine; ln -sf bits machine) ; \
	mkdir -p ../lib/$*/linux ; \
	cp $*/lib/*.a $*/lib/*.o ../lib/$*/linux

%.compiler-rt.install:
	cd src/compiler-rt/ellcc_linux ; \
	for target in `echo *` ; do \
	  echo Installing libcompiler-rt for $* ; \
	  mkdir -p ../../../lib/$*/linux ; \
	  cp $*/libcompiler_rt.a ../../../lib/$*/linux ; \
          (cd ../../../lib/$*/linux ; \
	    ln -fs libcompiler_rt.a libgcc.a ; \
	    ln -fs libncurses.a libterminfo.a); \
	done

other-libs:
	mkdir -p include/c++ ; \
	cp src/c++/libcxx/include/* include/c++ ; \
	mkdir -p include/c++/ext ; \
	cp src/c++/libcxx/include/ext/* include/c++/ext ; \
	cp src/c++/libcxxabi/include/* include/c++ ; \
	make -C obj install

clang-headers:
	mkdir -p clang ; \
	cp ../llvm/tools/clang/lib/Headers/* clang
