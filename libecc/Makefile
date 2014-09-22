# Build the ELLCC libraries.

# RICH: TODO: Check out the warnings.
PWD := $(shell pwd)
ELLCC := $(PWD)/..
LIBECC := $(ELLCC)/libecc
Configs := $(patsubst %.notyet,,$(shell cd $(LIBECC)/config; echo *))
CFLAGS := -g -Qunused-arguments \
          -Wno-unneeded-internal-declaration -Wno-cast-align \
	  -Wno-incompatible-pointer-types -Wno-string-plus-int \
	  -Wno-pointer-sign -Wno-array-bounds -Wno-dangling-else \
	  -Wno-int-to-pointer-cast

OBJ := $(PWD)/musl-build
install: c++-headers $(Configs) clang-headers

all:

$(Configs)::
	@echo Making libecc for $@ in musl-build/$@
	mkdir -p $(OBJ)/$@
	#RICH: ./config.musl "$(CFLAGS.$@)" $@ $(ELLCC) || exit 1
	./config.musl "-target $@ $(CFLAGS)" $@ $(ELLCC) || exit 1
	make -C src/musl || exit 1
	make -C src/musl install || exit 1
	make $@.musl.install
	@echo Making libcompiler-rt for $@ in src/compiler-rt
	make -C src/compiler-rt ellcc_linux-`./getarch $@`
	make $@.compiler-rt.install

$(Configs)::
	make $@.ncurses.configure && \
	make $@.ncurses.build && \
	make $@.ncurses.install

# RICH: NOCXX added below because of a Microblaze C++ error.
# Remove when fixed or if Microblaze is removed.

%.ncurses.configure:
	@echo Configuring ncurses for $* in ncurses-build/$*
	mkdir -p ncurses-build/$*
	cd ncurses-build/$* ; \
	NOCXX="--without-cxx-binding" ; \
	../../src/ncurses/configure \
	    CC="$(ELLCC)/bin/ecc -target $* $(CFLAGS)" \
	    CXX="$(ELLCC)/bin/ecc++ -target $* $(CXXFLAGS)" \
            AR=$(ELLCC)/bin/ecc-ar \
            RANLIB=$(ELLCC)/bin/ecc-ranlib \
            --host=`../../getarch $*`-`../../getos $*` \
	    --prefix=$(ELLCC) $$NOCXX

%.ncurses.build:
	make -C ncurses-build/$*

%.ncurses.install:
	@cd ncurses-build ; \
	echo Installing ncurses for $* ; \
	mkdir -p ../lib/$* ; \
	cp $*/lib/*.a ../lib/$* ; \
	cp $*/include/*.h ../include ; \
	make -C $*/misc install

$(Configs)::
	make $@.zlib.configure && \
	make $@.zlib.build && \
	make $@.zlib.install

%.zlib.configure:
	@echo Configuring zlib for $* in zlib-build/$*
	mkdir -p zlib-build/$*
	cp -fr src/zlib/* zlib-build/$*
	cd zlib-build/$* ; \
	CC="$(ELLCC)/bin/ecc $(CFLAGS.$*)" \
	CXX="$(ELLCC)/bin/ecc++ $(CXXFLAGS.$*)" \
        AR=$(ELLCC)/bin/ecc-ar \
        RANLIB=$(ELLCC)/bin/ecc-ranlib \
	    ./configure --static --prefix=$(ELLCC)

%.zlib.build:
	make -C zlib-build/$*

%.zlib.install:
	@cd zlib-build ; \
	echo Installing zlib for $* ; \
	mkdir -p ../lib/$* ; \
	cp $*/*.a ../lib/$* ; \
	cp $*/zlib.h ../include ; \
	cp $*/zconf.h ../include

$(Configs)::
	make $@.expat.configure && \
	make $@.expat.build && \
	make $@.expat.install

%.expat.configure:
	@echo Configuring expat for $* in expat-build/$*
	mkdir -p expat-build/$*
	cd expat-build/$* ; \
	CC="$(ELLCC)/bin/ecc $(CFLAGS.$*)" \
	CXX="$(ELLCC)/bin/ecc++ $(CXXFLAGS.$*)" \
        AR=$(ELLCC)/bin/ecc-ar \
        RANLIB=$(ELLCC)/bin/ecc-ranlib \
	    ../../src/expat/configure \
            --host=`../../getarch $*`-`../../getos $*` \
	    --prefix=$(ELLCC) --enable-shared=no

%.expat.build:
	make -C expat-build/$*

%.expat.install:
	@cd expat-build ; \
	echo Installing expat for $* ; \
	mkdir -p ../lib/$* ; \
	cp $*/.libs/*.a ../lib/$* ; \
	cp ../src/expat/lib/expat.h ../include ; \
	cp ../src/expat/lib/expat_external.h ../include

$(Configs)::
	make $@.libedit.configure && \
	make $@.libedit.build && \
	make $@.libedit.install

%.libedit.configure:
	@echo Configuring libedit for $* in libedit-build/$*
	mkdir -p libedit-build/$*
	cp -fr src/libedit/* libedit-build/$*
	cd libedit-build/$* ; \
	CC="$(ELLCC)/bin/ecc $(CFLAGS.$*)" \
	CXX="$(ELLCC)/bin/ecc++ $(CXXFLAGS.$*)" \
        AR=$(ELLCC)/bin/ecc-ar \
        RANLIB=$(ELLCC)/bin/ecc-ranlib \
	    ../../src/libedit/configure \
	    --enable-shared=no  --prefix=$(ELLCC) \
            --host=`../../getarch $*`-`../../getos $*`

%.libedit.build:
	make -C libedit-build/$*

%.libedit.install:
	@cd libedit-build ; \
	echo Installing libedit for $* ; \
	mkdir -p ../lib/$* ; \
	cp $*/src/.libs/*.a ../lib/$* ; \
	cp $*/src/histedit.h ../include ; \
	cp $*/src/editline/readline.h ../include

clean:
	make -C src/musl distclean
	make -C src/compiler-rt clean
	make -C obj veryclean
	rm -fr src/musl/musl.last *-build

veryclean: clean
	rm -fr $(OBJ)
	rm -fr libc++-build

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
	mkdir -p ../include/`../getincarch $*` ; \
	rm -fr ../include/`../getincarch $*`/bits ; \
	mv ../include/bits ../include/`../getincarch $*`/bits ; \
	(cd ../include/`../getincarch $*`; rm -rf machine; ln -sf bits machine)
	mkdir -p ../lib/$* ; \
	cp $*/lib/*.a $*/lib/*.o ../lib/$*

%.compiler-rt.install:
	cd src/compiler-rt/ellcc_linux ; \
	for target in `echo *` ; do \
	  echo Installing libcompiler-rt for $* ; \
	  mkdir -p ../../../lib/$* ; \
	  cp `../../../getarch $*`/libcompiler_rt.a ../../../lib/$* ; \
          (cd ../../../lib/$* ; \
	    ln -fs libcompiler_rt.a libgcc.a ; \
	    ln -fs libncurses.a libterminfo.a); \
	done

c++-headers:
	mkdir -p include/c++ ; \
	cp -r src/c++/libcxx/include/* include/c++ ; \
	cp -r src/c++/libcxxabi/include/* include/c++

$(Configs)::
	@echo Making libc++ for $@ in c++-build/$@
	mkdir -p c++-build/$@
	cp Makefile.lib c++-build/$@/Makefile
	make -C c++-build/$@ || exit 1
	cp c++-build/$@/libc++.a lib/$@

clang-headers:
	mkdir -p clang ; \
	cp ../llvm/tools/clang/lib/Headers/* clang
