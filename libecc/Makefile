# Build the ELLCC Linux libraries.

# RICH: TODO: Check out the warnings.
PWD := $(shell pwd)
ELLCC := $(PWD)/..
LIBECC := $(ELLCC)/libecc
Configs := $(patsubst %.notyet,,$(shell cd $(LIBECC)/config; echo *-linux-*))
ElkConfigs := $(patsubst *-elk-*,,$(shell cd $(LIBECC)/config; echo *-elk*))
CFLAGS := -g -Qunused-arguments \
          -Wno-unneeded-internal-declaration -Wno-cast-align \
	  -Wno-incompatible-pointer-types -Wno-string-plus-int \
	  -Wno-pointer-sign -Wno-array-bounds -Wno-dangling-else \
	  -Wno-int-to-pointer-cast

ifeq ($(VERBOSE),)
  MFLAGS=--no-print-directory
  OUT=@
else
  MFLAGS=
  OUT=
endif

OBJ := $(PWD)/musl-build
install: c++-headers clang-headers $(Configs) $(ElkConfigs)

elk: $(ElkConfigs)

all:

$(Configs)::
	@echo Making libecc for $@ in musl-build/$@
	$(OUT)mkdir -p $(OBJ)/$@/musl
	$(OUT)mkdir -p $(OBJ)/$@/install
	$(OUT)cp -pr src/musl $(OBJ)/$@
	$(OUT)if [ -e $(OBJ)/$@/musl/config.mak ] ; then \
          echo Configuring musl for $(TUPLE) aready done ; \
        else  \
          echo Configuring musl for $(TUPLE) in $(OBJ)/$@/musl ; \
	  ./config.musl "-target $@ $(CFLAGS)" $@ $(ELLCC) $(OBJ)/$@/musl || exit 1 ; \
        fi
	$(OUT)$(MAKE) $(MFLAGS) -C $(OBJ)/$@/musl || exit 1
	$(OUT)$(MAKE) $(MFLAGS) -C $(OBJ)/$@/musl install || exit 1
	$(OUT)$(MAKE) $(MFLAGS) $@.musl.install

$(Configs)::
	@echo Making libcompiler-rt for $@ in compiler-rt-build
	$(OUT)mkdir -p compiler-rt-build/$@
	$(OUT)cp -p Makefile.lib compiler-rt-build/$@/Makefile
	$(OUT)$(MAKE) $(MFLAGS) -C compiler-rt-build/$@ install || exit 1


$(Configs)::
	$(OUT)$(MAKE) $(MFLAGS) $@.ncurses.configure && \
	$(MAKE) $(MFLAGS) $@.ncurses.build && \
	$(MAKE) $(MFLAGS) $@.ncurses.install

# RICH: NOCXX added below because of a Microblaze C++ error.
# Remove when fixed or if Microblaze is removed.

%.ncurses.configure:
	$(OUT)if [ -e ncurses-build/$*/Makefile ] ; then \
	  echo Configuring ncurses for $* aready done ; \
	else  \
	  echo Configuring ncurses for $* in ncurses-build/$* ; \
	  mkdir -p ncurses-build/$* ; \
	  cd ncurses-build/$* ; \
	  NOCXX="--without-cxx-binding" ; \
	  ../../src/ncurses/configure \
	      CC="$(ELLCC)/bin/ecc -target $* $(CFLAGS)" \
	      CXX="$(ELLCC)/bin/ecc++ -target $* $(CXXFLAGS)" \
	      AR=$(ELLCC)/bin/ecc-ar \
	      RANLIB=$(ELLCC)/bin/ecc-ranlib \
	      --host=`../../../bin/ecc-getarch $*`-`../../../bin/ecc-getos $*` \
	      --build=x86_64-linux \
	      --with-build-cc=$(ELLCC)/bin/ecc \
	      --prefix=$(ELLCC) $$NOCXX ; \
	fi

%.ncurses.build:
	$(OUT)$(MAKE) $(MFLAGS) -C ncurses-build/$*

%.ncurses.install:
	$(OUT)cd ncurses-build ; \
	echo Installing ncurses for $* ; \
	mkdir -p ../lib/$* ; \
	cp $*/lib/*.a ../lib/$* ; \
	cp -p $*/include/*.h ../include ; \
	$(MAKE) $(MFLAGS) -C $*/misc install

$(Configs)::
	$(OUT)$(MAKE) $(MFLAGS) $@.zlib.configure && \
	$(MAKE) $(MFLAGS) $@.zlib.build && \
	$(MAKE) $(MFLAGS) $@.zlib.install

%.zlib.configure:
	$(OUT)if [ -e zlib-build/$*/Makefile ] ; then \
	  echo Configuring zlib for $* aready done ; \
	else  \
	  echo Configuring zlib for $* in zlib-build/$* ; \
	  mkdir -p zlib-build/$* ; \
	  cp -pfr src/zlib/* zlib-build/$* ; \
	  cd zlib-build/$* ; \
	  CC="$(ELLCC)/bin/ecc -target $* $(CFLAGS)" \
	  CXX="$(ELLCC)/bin/ecc++ -target $* $(CXXFLAGS)" \
	  AR=$(ELLCC)/bin/ecc-ar \
	  RANLIB=$(ELLCC)/bin/ecc-ranlib \
	      ./configure --static --prefix=$(ELLCC) ; \
	fi

%.zlib.build:
	$(OUT)$(MAKE) $(MFLAGS) -C zlib-build/$*

%.zlib.install:
	$(OUT)cd zlib-build ; \
	echo Installing zlib for $* ; \
	mkdir -p ../lib/$* ; \
	cp $*/*.a ../lib/$* ; \
	cp -p $*/zlib.h ../include ; \
	cp -p $*/zconf.h ../include

$(Configs)::
	$(OUT)$(MAKE) $(MFLAGS) $@.expat.configure && \
	$(MAKE) $(MFLAGS) $@.expat.build && \
	$(MAKE) $(MFLAGS) $@.expat.install

%.expat.configure:
	$(OUT)if [ -e expat-build/$*/Makefile ] ; then \
	  echo Configuring expat for $* aready done ; \
	else \
	  echo Configuring expat for $* in expat-build/$* ; \
	  mkdir -p expat-build/$* ; \
	  cd expat-build/$* ; \
	  CC="$(ELLCC)/bin/ecc -target $* $(CFLAGS)" \
	  CXX="$(ELLCC)/bin/ecc++ -target $* $(CXXFLAGS)" \
	  AR=$(ELLCC)/bin/ecc-ar \
	  RANLIB=$(ELLCC)/bin/ecc-ranlib \
	      ../../src/expat/configure \
	      --host=`../../../bin/ecc-getarch $*`-`../../../bin/ecc-getos $*` \
	      --prefix=$(ELLCC) --enable-shared=no ; \
	fi

%.expat.build:
	$(OUT)$(MAKE) $(MFLAGS) -C expat-build/$*

%.expat.install:
	$(OUT)cd expat-build ; \
	echo Installing expat for $* ; \
	mkdir -p ../lib/$* ; \
	cp $*/.libs/*.a ../lib/$* ; \
	cp -p ../src/expat/lib/expat.h ../include ; \
	cp -p ../src/expat/lib/expat_external.h ../include

$(Configs)::
	$(OUT)$(MAKE) $(MFLAGS) $@.libedit.configure && \
	$(MAKE) $(MFLAGS) $@.libedit.build && \
	$(MAKE) $(MFLAGS) $@.libedit.install

%.libedit.configure:
	$(OUT)if [ -e libedit-build/$*/Makefile ] ; then \
	  echo Configuring libedit for $* aready done ; \
	else \
	  echo Configuring libedit for $* in libedit-build/$* ; \
	  mkdir -p libedit-build/$* ; \
	  cp -pfr src/libedit/* libedit-build/$* ; \
	  cd libedit-build/$* ; \
	  RANLIB=$(ELLCC)/bin/ecc-ranlib \
	  CC="$(ELLCC)/bin/ecc -target $* $(CFLAGS)" \
	  CXX="$(ELLCC)/bin/ecc++ -target $* $(CXXFLAGS)" \
	  AR=$(ELLCC)/bin/ecc-ar \
	  RANLIB=$(ELLCC)/bin/ecc-ranlib \
	      ../../src/libedit/configure \
	      --enable-shared=no  --prefix=$(ELLCC) \
	      --host=`../../../bin/ecc-getarch $*`-`../../../bin/ecc-getos $*` ; \
	fi

%.libedit.build:
	$(OUT)$(MAKE) $(MFLAGS) -C libedit-build/$*

%.libedit.install:
	$(OUT)cd libedit-build ; \
	echo Installing libedit for $* ; \
	mkdir -p ../lib/$* ; \
	cp $*/src/.libs/*.a ../lib/$* ; \
	cp -p $*/src/histedit.h ../include ; \
	cp -p $*/src/editline/readline.h ../include

clean:
	$(OUT)rm -fr *-build

install:

musl.install:
	$(OUT)cd musl-build ; \
	for target in `echo *` ; do \
	  $(MAKE) $(MFLAGS) -C .. $$target.musl.install ; \
	done

%.musl.install:
	$(OUT)cd musl-build ; \
	echo Installing libecc for $* ; \
	mkdir -p ../include ; \
	cp -pr $*/install/include/* ../include ; \
	ln -sf ../endian.h ../include/sys ; \
	mkdir -p ../include/`../../bin/ecc-getincarch $*` ; \
	rm -fr ../include/`../../bin/ecc-getincarch $*`/bits ; \
	mv ../include/bits ../include/`../../bin/ecc-getincarch $*`/bits ; \
	(cd ../include/`../../bin/ecc-getincarch $*`; rm -rf machine; ln -sf bits machine) ;\
	mkdir -p ../lib/$* ; \
	cp $*/install/lib/*.a $*/install/lib/*.o ../lib/$*

c++-headers:
	$(OUT)echo Installing C++ headers ; \
	mkdir -p include/c++ ; \
	cp -pr src/c++/libcxx/include/* include/c++ ; \
	cp -pr src/c++/libcxxabi/include/* include/c++
	mv include/c++/unwind.h include/
	mv include/c++/__cxxabi_config.h include/

$(Configs)::
	$(OUT)echo Making libc++ for $@ in c++-build/$@
	$(OUT)mkdir -p c++-build/$@
	$(OUT)cp Makefile.lib c++-build/$@/Makefile
	$(OUT)$(MAKE) $(MFLAGS) -C c++-build/$@ || exit 1
	$(OUT)cp c++-build/$@/libc++.a lib/$@

clang-headers:
	$(OUT)echo Installing clang headers
	$(OUT)mkdir -p clang
	$(OUT)cp -p ../llvm/tools/clang/lib/Headers/* clang
	$(OUT)cp -p ../llvm/tools/clang/lib/Headers/altivec.h include
	$(OUT)cp -p ../llvm/tools/clang/lib/Headers/emmintrin.h include
	$(OUT)cp -p ../llvm/tools/clang/lib/Headers/mmintrin.h include
	$(OUT)cp -p ../llvm/tools/clang/lib/Headers/mm_malloc.h include
	$(OUT)cp -p ../llvm/tools/clang/lib/Headers/xmmintrin.h include

$(ElkConfigs)::
	$(OUT)echo Making libelk for $@ in elk-build/$@
	$(OUT)mkdir -p elk-build/$@
	$(OUT)mkdir -p elk-build/$@/bin
	$(OUT)cp Makefile.lib elk-build/$@/Makefile
	$(OUT)cp -pr src/elk/bin/* elk-build/$@/bin
	$(OUT)$(MAKE) $(MFLAGS) -C elk-build/$@ || exit 1
	$(OUT)$(MAKE) $(MFLAGS) -C elk-build/$@/bin || exit 1
	$(OUT)$(MAKE) $(MFLAGS) LIBDIR=`pwd`/lib/$@ -C elk-build/$@ install || exit 1
