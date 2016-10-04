# Build the ELLCC Linux libraries.

PWD := $(shell pwd)
ELLCC := $(PWD)/..
LIBECC := $(ELLCC)/libecc
Configs := $(shell cat $(LIBECC)/config/libraries | grep -v ^\# | sed -e 's/.*-elk*//g')
ElkConfigs := $(shell cat $(LIBECC)/config/libraries | grep -v ^\# | sed -e 's/.*-linux*//g')
CFLAGS := -g
ifeq ($(shell uname),Darwin)
  CPFLAGS := -pfr
else
  CPFLAGS := -pfru
endif

ifeq ($(VERBOSE),)
  MFLAGS=--no-print-directory
  OUT=@
else
  MFLAGS=
  OUT=
endif

OBJ := $(PWD)/musl-build
install: musl.build musl.install c++-headers clang-headers $(Configs) $(ElkConfigs)

elk: $(ElkConfigs)

all:

musl.build:
	$(OUT)for target in $(Configs) ; do \
	  $(MAKE) $(MFLAGS) $$target.musl.build ; \
	done

%.musl.build:
	@echo Making libecc for $* in musl-build/$*
	$(OUT)mkdir -p $(OBJ)/$*/musl
	$(OUT)mkdir -p $(OBJ)/$*/install
	$(OUT)cp -pr src/musl $(OBJ)/$*
	$(OUT)if [ -e $(OBJ)/$*/musl/config.mak ] ; then \
          echo Configuring musl for $(TUPLE) aready done ; \
        else  \
          echo Configuring musl for $(TUPLE) in $(OBJ)/$*/musl ; \
	  ./config.musl "-target $* $(CFLAGS)" $* $(ELLCC) $(OBJ)/$*/musl || exit 1 ; \
        fi
	$(OUT)$(MAKE) $(MFLAGS) -C $(OBJ)/$*/musl || exit 1
	$(OUT)$(MAKE) $(MFLAGS) -C $(OBJ)/$*/musl install || exit 1

musl.install: musl.build
	$(OUT)cd musl-build ; \
	for target in $(Configs) ; do \
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

$(Configs):: musl.install
	@echo Making libcompiler-rt for $@ in compiler-rt-build
	$(OUT)mkdir -p compiler-rt-build/$@
	$(OUT)cp -p Makefile.lib compiler-rt-build/$@/Makefile
	$(OUT)$(MAKE) $(MFLAGS) -C compiler-rt-build/$@ install || exit 1
	$(OUT)cd lib/$@; ln -sf libcompiler-rt.a libgcc.a

$(Configs):: musl.install
	$(OUT)$(MAKE) $(MFLAGS) $@.ncurses.configure && \
	$(MAKE) $(MFLAGS) $@.ncurses.build && \
	$(MAKE) $(MFLAGS) $@.ncurses.install

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
              --host=x86_64-macos \
	      --build=x86_64-linux \
	      --with-build-cc=cc \
	      --datarootdir=$(ELLCC)/libecc/share \
	      --prefix=$(ELLCC) $$NOCXX ; \
	fi

%.ncurses.build:
	$(OUT)$(MAKE) $(MFLAGS) -C ncurses-build/$*

%.ncurses.install:
	echo Installing ncurses for $*
	$(OUT)cp -p src/ncurses/c++/curs*.h include
	$(OUT)cd ncurses-build ; \
	mkdir -p ../lib/$* ; \
	cp $*/lib/*.a ../lib/$* ; \
	(cd ../lib/$*/; ln -s libncurses.a libterminfo.a) ; \
	cp -p $*/include/*.h ../include ; \
	rm -f ../include/config.h ; \
	(cd ../include; ln -s curses.h ncurses.h ) ; \
	$(MAKE) $(MFLAGS) -C $*/misc install

$(Configs):: musl.install
	$(OUT)$(MAKE) $(MFLAGS) $@.ncursesw.configure && \
	$(MAKE) $(MFLAGS) $@.ncursesw.build && \
	$(MAKE) $(MFLAGS) $@.ncursesw.install

%.ncursesw.configure:
	$(OUT)if [ -e ncursesw-build/$*/Makefile ] ; then \
	  echo Configuring ncursesw for $* aready done ; \
	else  \
	  echo Configuring ncursesw for $* in ncursesw-build/$* ; \
	  mkdir -p ncursesw-build/$* ; \
	  cd ncursesw-build/$* ; \
	  NOCXX="--without-cxx-binding" ; \
	  ../../src/ncurses/configure \
	      CC="$(ELLCC)/bin/ecc -target $* $(CFLAGS)" \
	      CXX="$(ELLCC)/bin/ecc++ -target $* $(CXXFLAGS)" \
	      AR=$(ELLCC)/bin/ecc-ar \
	      RANLIB=$(ELLCC)/bin/ecc-ranlib \
	      --enable-widec \
              --host=x86_64-macos \
	      --build=x86_64-linux \
	      --with-build-cc=cc \
	      --prefix=$(ELLCC) $$NOCXX ; \
	fi

%.ncursesw.build:
	$(OUT)$(MAKE) $(MFLAGS) -C ncursesw-build/$*

%.ncursesw.install:
	echo Installing ncursesw for $*
	$(OUT)cp -p src/ncurses/c++/curs*.h include
	$(OUT)cd ncursesw-build ; \
	mkdir -p ../lib/$* ; \
	cp $*/lib/*.a ../lib/$* ; \
	cp -p $*/include/*.h ../include ; \
	rm -f ../include/config.h ; \
	$(MAKE) $(MFLAGS) -C $*/misc install

$(Configs):: musl.install
	$(OUT)$(MAKE) $(MFLAGS) $@.zlib.configure && \
	$(MAKE) $(MFLAGS) $@.zlib.build && \
	$(MAKE) $(MFLAGS) $@.zlib.install

%.zlib.configure:
	$(OUT)if [ -e zlib-build/$*/Makefile ] ; then \
	  echo Configuring zlib for $* aready done ; \
	else  \
	  echo Configuring zlib for $* in zlib-build/$* ; \
	  mkdir -p zlib-build/$* ; \
	  cp $(CPFLAGS) src/zlib/* zlib-build/$* ; \
	  cd zlib-build/$* ; \
	  CC="$(ELLCC)/bin/ecc -target $* $(CFLAGS)" \
	  CXX="$(ELLCC)/bin/ecc++ -target $* $(CXXFLAGS)" \
	  AR=$(ELLCC)/bin/ecc-ar \
	  RANLIB=$(ELLCC)/bin/ecc-ranlib \
	      ./configure --static --prefix=$(ELLCC) ; \
	fi

%.zlib.build:
	$(OUT)cp $(CPFLAGS) src/zlib/* zlib-build/$*
	$(OUT)$(MAKE) $(MFLAGS) -C zlib-build/$*

%.zlib.install:
	$(OUT)cd zlib-build ; \
	echo Installing zlib for $* ; \
	mkdir -p ../lib/$* ; \
	cp $*/*.a ../lib/$* ; \
	cp -p $*/zlib.h ../include ; \
	cp -p $*/zconf.h ../include

$(Configs):: musl.install
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

$(Configs):: musl.install
	$(OUT)$(MAKE) $(MFLAGS) $@.libedit.configure && \
	$(MAKE) $(MFLAGS) $@.libedit.build && \
	$(MAKE) $(MFLAGS) $@.libedit.install

%.libedit.configure:
	$(OUT)if [ -e libedit-build/$*/Makefile ] ; then \
	  echo Configuring libedit for $* aready done ; \
	else \
	  echo Configuring libedit for $* in libedit-build/$* ; \
	  mkdir -p libedit-build/$* ; \
	  cp $(CPFLAGS) src/libedit/* libedit-build/$* ; \
	  cd libedit-build/$* ; \
	      ../../src/libedit/configure \
	      --enable-shared=no  --prefix=$(ELLCC) \
	      --host=`../../../bin/ecc-getarch $*`-`../../../bin/ecc-getos $*` \
	      CC="$(ELLCC)/bin/ecc -target $* $(CFLAGS)" \
	      CXX="$(ELLCC)/bin/ecc++ -target $* $(CXXFLAGS)" \
	      AR=$(ELLCC)/bin/ecc-ar \
	      RANLIB=$(ELLCC)/bin/ecc-ranlib ; \
	fi

%.libedit.build:
	$(OUT)cp $(CPFLAGS) src/libedit/* libedit-build/$*
	$(OUT)$(MAKE) $(MFLAGS) -C libedit-build/$*

%.libedit.install:
	$(OUT)cd libedit-build ; \
	echo Installing libedit for $* ; \
	mkdir -p ../lib/$* ; \
	cp $*/src/.libs/*.a ../lib/$* ; \
	cp -p $*/src/histedit.h ../include ; \
	cp -p $*/src/vis.h ../include ; \
	cp -p $*/src/editline/readline.h ../include

$(Configs):: musl.install
	$(OUT)$(MAKE) $(MFLAGS) $@.mbedtls.configure && \
	$(MAKE) $(MFLAGS) $@.mbedtls.build && \
	$(MAKE) $(MFLAGS) $@.mbedtls.install

%.mbedtls.configure:
	$(OUT)echo Configuring mbedtls for $* in mbedtls-build/$* ; \
	  mkdir -p mbedtls-build/$* ; \
	  cp $(CPFLAGS) src/mbedtls/* mbedtls-build/$* ; \
	  echo export CC="$(ELLCC)/bin/ecc -target $* $(CFLAGS)" \
	    > mbedtls-build/$*/Makefile ; \
	  echo export AR=$(ELLCC)/bin/ecc-ar >> mbedtls-build/$*/Makefile ; \
	  echo export RANLIB=$(ELLCC)/bin/ecc-ranlib \
	    >> mbedtls-build/$*/Makefile ; \
	  cat src/mbedtls/Makefile >> mbedtls-build/$*/Makefile

%.mbedtls.build:
	$(OUT)cp $(CPFLAGS) src/mbedtls/* mbedtls-build/$*
	$(OUT)$(MAKE) $(MFLAGS) CC="$(ELLCC)/bin/ecc -target $*" \
            AR=$(ELLCC)/bin/ecc-ar \
            RANLIB=$(ELLCC)/bin/ecc-ranlib \
            -C mbedtls-build/$*

%.mbedtls.install:
	$(OUT)cd mbedtls-build/$* ; \
	echo Installing mbedtls for $*; \
	mkdir -p ../../include/mbedtls; \
	cp -rp include/mbedtls ../../include ; \
	mkdir -p ../../lib/$* ; \
	cp -RP library/libmbedtls.*    ../../lib/$* ; \
	cp -RP library/libmbedx509.*   ../../lib/$* ; \
	cp -RP library/libmbedcrypto.* ../../lib/$*

$(Configs):: musl.install
	$(OUT)$(MAKE) $(MFLAGS) $@.c-ares.configure && \
	$(MAKE) $(MFLAGS) $@.c-ares.build && \
	$(MAKE) $(MFLAGS) $@.c-ares.install

%.c-ares.configure:
	$(OUT)if [ -e c-ares-build/$*/Makefile ] ; then \
	  echo Configuring c-ares for $* aready done ; \
	else \
	  echo Configuring c-ares for $* in c-ares-build/$* ; \
	  mkdir -p c-ares-build/$* ; \
	  cp $(CPFLAGS) src/c-ares/* c-ares-build/$* ; \
	  cd c-ares-build/$* ; \
	  CC="$(ELLCC)/bin/ecc" \
            CFLAGS="-target $* $(CFLAGS)" \
            CPP="$(ELLCC)/bin/ecc -E -target $*" \
	    AR=$(ELLCC)/bin/ecc-ar \
	    RANLIB=$(ELLCC)/bin/ecc-ranlib \
            ../../src/c-ares/configure \
	      --enable-shared=no  --prefix=$(ELLCC) \
	      --host=`../../../bin/ecc-getarch $*`-`../../../bin/ecc-getos $*`; \
	fi

%.c-ares.build:
	$(OUT)cp $(CPFLAGS) src/c-ares/* c-ares-build/$*
	$(OUT)$(MAKE) $(MFLAGS) CC="$(ELLCC)/bin/ecc -target $*" \
            AR=$(ELLCC)/bin/ecc-ar \
            RANLIB=$(ELLCC)/bin/ecc-ranlib \
            -C c-ares-build/$*

%.c-ares.install:
	$(OUT)cd c-ares-build/$* ; \
	echo Installing c-ares for $*; \
	mkdir -p ../../include ; \
	cp -p ares.h ares_version.h ares_dns.h ares_rules.h ../../include ; \
	mkdir -p ../../include/`../../../bin/ecc-getincarch $*` ; \
	cp ares_build.h ../../include/`../../../bin/ecc-getincarch $*` ; \
	mkdir -p ../../lib/$* ; \
	cp -RP .libs/libcares.a    ../../lib/$*

$(Configs):: musl.install
	$(OUT)$(MAKE) $(MFLAGS) $@.libssh2.configure && \
	$(MAKE) $(MFLAGS) $@.libssh2.build && \
	$(MAKE) $(MFLAGS) $@.libssh2.install

%.libssh2.configure:
	$(OUT)mkdir -p libssh2-build/$*
	$(OUT)cp $(CPFLAGS) src/libssh2/* libssh2-build/$*
	$(OUT)if [ -e libssh2-build/$*/Makefile ] ; then \
	  echo Configuring libssh2 for $* aready done ; \
	else \
	  echo Configuring libssh2 for $* in libssh2-build/$* ; \
	  cd libssh2-build/$* ; \
	  CC="$(ELLCC)/bin/ecc" \
          CFLAGS="-target $* $(CFLAGS)" \
            CPP="$(ELLCC)/bin/ecc -E -target $*" \
	    AR=$(ELLCC)/bin/ecc-ar \
	    RANLIB=$(ELLCC)/bin/ecc-ranlib \
              ../../src/libssh2/configure \
	        --enable-debug \
	        --enable-shared=no  --prefix=$(ELLCC) \
	        --host=`../../../bin/ecc-getarch $*`-`../../../bin/ecc-getos $*`; \
        fi

%.libssh2.build:
	$(OUT)mkdir -p libssh2-build/$*
	$(OUT)cp $(CPFLAGS) src/libssh2/* libssh2-build/$*
	$(OUT)$(MAKE) $(MFLAGS) CC="$(ELLCC)/bin/ecc -target $*" \
            AR=$(ELLCC)/bin/ecc-ar \
            RANLIB=$(ELLCC)/bin/ecc-ranlib \
            -C libssh2-build/$*

%.libssh2.install:
	$(OUT)cd libssh2-build/$* ; \
	echo Installing libssh2 for $*; \
	mkdir -p ../../include ; \
	cp -p include/*.h ../../include ; \
	mkdir -p ../../lib/$* ; \
	cp -RP ./src/.libs/libssh2.a ../../lib/$*

$(Configs):: musl.install
	$(OUT)$(MAKE) $(MFLAGS) $@.nghttp2.configure && \
	$(MAKE) $(MFLAGS) $@.nghttp2.build && \
	$(MAKE) $(MFLAGS) $@.nghttp2.install

%.nghttp2.configure:
	$(OUT)mkdir -p nghttp2-build/$*
	$(OUT)cp $(CPFLAGS) src/nghttp2/* nghttp2-build/$*
	$(OUT)if [ -e nghttp2-build/$*/Makefile ] ; then \
	  echo Configuring nghttp2 for $* aready done ; \
	else \
	  echo Configuring nghttp2 for $* in nghttp2-build/$* ; \
	  cd nghttp2-build/$* ; \
	  CC="$(ELLCC)/bin/ecc" \
            CFLAGS="-target $* $(CFLAGS)" \
            LDFLAGS="-target $*" \
	    CXX="$(ELLCC)/bin/ecc++" \
            CXXFLAGS="-target $* $(CXXFLAGS)" \
            CPP="$(ELLCC)/bin/ecc -E -target $*" \
            CXXCPP="$(ELLCC)/bin/ecc++ -E -target $*" \
	    AR=$(ELLCC)/bin/ecc-ar \
	    RANLIB=$(ELLCC)/bin/ecc-ranlib \
              ../../src/nghttp2/configure \
	        --enable-debug \
	        --enable-shared=no  --prefix=$(ELLCC) \
	        --host=`../../../bin/ecc-getarch $*`-`../../../bin/ecc-getos $*`; \
	fi

%.nghttp2.build:
	$(OUT)mkdir -p nghttp2-build/$*
	$(OUT)cp $(CPFLAGS) src/nghttp2/* nghttp2-build/$*
	$(OUT)$(MAKE) $(MFLAGS) CC="$(ELLCC)/bin/ecc -target $*" \
            AR=$(ELLCC)/bin/ecc-ar \
            RANLIB=$(ELLCC)/bin/ecc-ranlib \
            -C nghttp2-build/$*

%.nghttp2.install:
	$(OUT)cd nghttp2-build/$* ; \
	echo Installing nghttp2 for $*; \
	mkdir -p ../../include/nghttp2 ; \
	cp -p lib/includes/nghttp2/*.h ../../include/nghttp2 ; \
	mkdir -p ../../lib/$* ; \
	cp -RP ./lib/.libs/libnghttp2.a ../../lib/$*

$(Configs):: musl.install
	$(OUT)$(MAKE) $(MFLAGS) $@.libmetalink.configure && \
	$(MAKE) $(MFLAGS) $@.libmetalink.build && \
	$(MAKE) $(MFLAGS) $@.libmetalink.install

%.libmetalink.configure:
	$(OUT)mkdir -p libmetalink-build/$*
	$(OUT)cp $(CPFLAGS) src/libmetalink/* libmetalink-build/$*
	$(OUT)if [ -e libmetalink-build/$*/Makefile ] ; then \
	  echo Configuring libmetalink for $* aready done ; \
	else \
	  echo Configuring libmetalink for $* in libmetalink-build/$* ; \
	  cd libmetalink-build/$* ; \
	  CC="$(ELLCC)/bin/ecc" \
            CFLAGS="-target $* $(CFLAGS)" \
            LDFLAGS="-target $*" \
	    CXX="$(ELLCC)/bin/ecc++" \
            CXXFLAGS="-target $* $(CXXFLAGS)" \
            CPP="$(ELLCC)/bin/ecc -E -target $*" \
            CXXCPP="$(ELLCC)/bin/ecc++ -E" \
	    AR=$(ELLCC)/bin/ecc-ar \
	    RANLIB=$(ELLCC)/bin/ecc-ranlib \
              ../../src/libmetalink/configure \
	        --enable-debug --with-libexpat=yes \
	        --enable-shared=no  --prefix=$(ELLCC) \
	        --host=`../../../bin/ecc-getarch $*`-`../../../bin/ecc-getos $*`; \
	fi

%.libmetalink.build:
	$(OUT)mkdir -p libmetalink-build/$*
	$(OUT)cp $(CPFLAGS) src/libmetalink/* libmetalink-build/$*
	$(OUT)$(MAKE) $(MFLAGS) CC="$(ELLCC)/bin/ecc -target $*" \
            AR=$(ELLCC)/bin/ecc-ar \
            RANLIB=$(ELLCC)/bin/ecc-ranlib \
            -C libmetalink-build/$*

%.libmetalink.install:
	$(OUT)cd libmetalink-build/$* ; \
	echo Installing libmetalink for $*; \
	mkdir -p ../../include/metalink ; \
	cp -p lib/includes/metalink/*.h ../../include/metalink ; \
	mkdir -p ../../lib/$* ; \
	cp -RP ./lib/.libs/libmetalink.a ../../lib/$*

$(Configs):: musl.install
	$(OUT)$(MAKE) $(MFLAGS) $@.openldap.configure && \
	$(MAKE) $(MFLAGS) $@.openldap.build && \
	$(MAKE) $(MFLAGS) $@.openldap.install

%.openldap.configure:
	$(OUT)mkdir -p openldap-build/$*
	$(OUT)cp $(CPFLAGS) src/openldap/* openldap-build/$*
	$(OUT)if [ -e openldap-build/$*/Makefile ] ; then \
	  echo Configuring openldap for $* aready done ; \
	else \
	  echo Configuring openldap for $* in openldap-build/$* ; \
	  cd openldap-build/$* ; \
	  CC="$(ELLCC)/bin/ecc" \
            CFLAGS="-target $* $(CFLAGS)" \
            LDFLAGS="-target $*" \
	    CXX="$(ELLCC)/bin/ecc++" \
            CXXFLAGS="-target $* $(CXXFLAGS)" \
            CPP="$(ELLCC)/bin/ecc -E -target $*" \
            CXXCPP="$(ELLCC)/bin/ecc++ -E -target $*" \
	    AR=$(ELLCC)/bin/ecc-ar \
	    RANLIB=$(ELLCC)/bin/ecc-ranlib \
              ../../src/openldap/configure \
	        --enable-debug --disable-bdb --disable-hdb \
	        --enable-shared=no  --prefix=$(ELLCC) \
		 --with-yielding_select=yes \
	         --host=`../../../bin/ecc-getarch $*`-`../../../bin/ecc-getos $*`; \
	fi

%.openldap.build:
	$(OUT)mkdir -p openldap-build/$*
	$(OUT)cp $(CPFLAGS) src/openldap/* openldap-build/$*
	$(OUT)$(MAKE) $(MFLAGS) CC="$(ELLCC)/bin/ecc -target $*" \
            AR=$(ELLCC)/bin/ecc-ar \
            RANLIB=$(ELLCC)/bin/ecc-ranlib \
            -C openldap-build/$*

%.openldap.install:
	$(OUT)cd openldap-build/$* ; \
	echo Installing openldap for $*; \
	mkdir -p ../../include ; \
	cp -p include/lber.h include/lber_types.h include/ldap.h \
	  include/ldap_cdefs.h include/ldap_schema.h include/ldap_utf8.h \
	  include/slapi-plugin.h include/ldap_features.h include/ldif.h \
	  ../../include ; \
	mkdir -p ../../lib/$* ; \
	cp -RP ./libraries/libldap_r/.libs/libldap_r.a ../../lib/$* ; \
	cp -RP ./libraries/libldap/.libs/libldap.a ../../lib/$* ; \
	cp -RP ./libraries/liblber/.libs/liblber.a ../../lib/$* ; \

$(Configs):: musl.install
	$(OUT)$(MAKE) $(MFLAGS) $@.curl.configure && \
	$(MAKE) $(MFLAGS) $@.curl.build && \
	$(MAKE) $(MFLAGS) $@.curl.install

%.curl.configure:
	$(OUT)mkdir -p curl-build/$*
	$(OUT)cp $(CPFLAGS) src/curl/* curl-build/$*
	$(OUT)if [ -e curl-build/$*/src/Makefile ] ; then \
	  echo Configuring curl for $* aready done ; \
	else \
	  echo Configuring curl for $* in curl-build/$* ; \
	  cd curl-build/$* ; \
	  CC="$(ELLCC)/bin/ecc" \
            CFLAGS="-target $* $(CFLAGS)" \
            LDFLAGS="-target $*" \
	    CXX="$(ELLCC)/bin/ecc++" \
            CXXFLAGS="-target $* $(CXXFLAGS)" \
            CPP="$(ELLCC)/bin/ecc -E -target $*" \
            CXXCPP="$(ELLCC)/bin/ecc++ -E -target $*" \
	    AR=$(ELLCC)/bin/ecc-ar \
	    RANLIB=$(ELLCC)/bin/ecc-ranlib \
              ../../src/curl/configure \
	        --prefix=$(ELLCC) \
		--with-mbedtls --with-libssh2 --enable-ares \
		--with-libmetalink=yes \
                --enable-shared=no --with-nghttp2=yes --enable-ipv6 \
	        --host=`../../../bin/ecc-getarch $*`-`../../../bin/ecc-getos $*`; \
	fi

%.curl.build:
	$(OUT)cp $(CPFLAGS) src/curl/* curl-build/$*
	$(OUT)$(MAKE) $(MFLAGS) -C curl-build/$*/lib
	$(OUT)$(MAKE) $(MFLAGS) -C curl-build/$*/src

%.curl.install:
	$(OUT)cd curl-build/$* ; \
	echo Installing curl for $*; \
	mkdir -p ../../include/curl ; \
	cp -p include/curl/*.h ../../include/curl ; \
	mkdir -p ../../include/`../../../bin/ecc-getincarch $*`/curl ; \
	mv ../../include/curl/curlbuild.h ../../include/`../../../bin/ecc-getincarch $*`/curl ; \
	mkdir -p ../../lib/$* ; \
	cp -RP ./lib/.libs/libcurl.a ../../lib/$*

$(Configs):: musl.install
	$(OUT)$(MAKE) $(MFLAGS) $@.nanomsg.configure && \
	$(MAKE) $(MFLAGS) $@.nanomsg.build && \
	$(MAKE) $(MFLAGS) $@.nanomsg.install

%.nanomsg.configure:
	$(OUT)if [ -e nanomsg-build/$*/Makefile ] ; then \
	  echo Configuring nanomsg for $* aready done ; \
	else \
	  echo Configuring nanomsg for $* in nanomsg-build/$* ; \
	  mkdir -p nanomsg-build/$* ; \
	  cp $(CPFLAGS) src/nanomsg/* nanomsg-build/$* ; \
	  cd nanomsg-build/$* ; \
	  CC="$(ELLCC)/bin/ecc" \
            CFLAGS="-target $* $(CFLAGS)" \
            CPP="$(ELLCC)/bin/ecc -E -target $*" \
	    AR=$(ELLCC)/bin/ecc-ar \
	    RANLIB=$(ELLCC)/bin/ecc-ranlib \
              ../../src/nanomsg/configure \
	        --enable-shared=no  --prefix=$(ELLCC) \
	        --host=`../../../bin/ecc-getarch $*`-`../../../bin/ecc-getos $*`; \
	fi

%.nanomsg.build:
	$(OUT)cp $(CPFLAGS) src/nanomsg/* nanomsg-build/$*
	$(OUT)$(MAKE) $(MFLAGS) CC="$(ELLCC)/bin/ecc -target $*" \
            AR=$(ELLCC)/bin/ecc-ar \
            RANLIB=$(ELLCC)/bin/ecc-ranlib \
            -C nanomsg-build/$*

%.nanomsg.install:
	$(OUT)cd nanomsg-build/$* ; \
	echo Installing nanomsg for $*; \
	mkdir -p ../../include/nanomsg ; \
	cp -p src/bus.h src/inproc.h src/ipc.h src/nn.h src/pair.h \
              src/pipeline.h src/pubsub.h src/reqrep.h src/survey.h \
              src/tcp.h src/tcpmux.h src/ws.h ../../include/nanomsg ; \
	mkdir -p ../../lib/$* ; \
	cp -RP .libs/libnanomsg.a ../../lib/$*

$(Configs):: musl.install
	$(OUT)$(MAKE) $(MFLAGS) $@.libevent.configure && \
	$(MAKE) $(MFLAGS) $@.libevent.build && \
	$(MAKE) $(MFLAGS) $@.libevent.install

%.libevent.configure:
	$(OUT)if [ -e libevent-build/$*/Makefile ] ; then \
	  echo Configuring libevent for $* aready done ; \
	else \
	  echo Configuring libevent for $* in libevent-build/$* ; \
	  mkdir -p libevent-build/$* ; \
	  cp $(CPFLAGS) src/libevent/* libevent-build/$* ; \
	  cd libevent-build/$* ; \
	  CC="$(ELLCC)/bin/ecc" \
            CFLAGS="-target $* $(CFLAGS)" \
            CPP="$(ELLCC)/bin/ecc -E -target $*" \
	    AR=$(ELLCC)/bin/ecc-ar \
	    RANLIB=$(ELLCC)/bin/ecc-ranlib \
              ../../src/libevent/configure \
	        --enable-shared=no  --prefix=$(ELLCC) \
	        --host=`../../../bin/ecc-getarch $*`-`../../../bin/ecc-getos $*`; \
      fi

%.libevent.build:
	$(OUT)cp $(CPFLAGS) src/libevent/* libevent-build/$*
	$(OUT)$(MAKE) $(MFLAGS) CC="$(ELLCC)/bin/ecc -target $*" \
            AR=$(ELLCC)/bin/ecc-ar \
            RANLIB=$(ELLCC)/bin/ecc-ranlib \
            -C libevent-build/$*

%.libevent.install:
	$(OUT)cd libevent-build/$* ; \
	echo Installing libevent for $*; \
	mkdir -p ../../include/event2 ; \
	cp -p include/event2/*.h ../../include/event2 ; \
	cp evdns.h event.h evhttp.h evrpc.h evutil.h ../../include ; \
	mkdir -p ../../lib/$* ; \
	cp -RP .libs/libevent.a ../../lib/$*

clean:
	$(OUT)rm -fr *-build

install:

c++-headers:
	$(OUT)echo Installing C++ headers ; \
	mkdir -p include/c++ ; \
	cp -pr src/c++/libcxx/include/* include/c++ ; \
	cp -pr src/c++/libcxxabi/include/* include/c++
	cp -pr src/c++/libunwind/include/unwind.h include/
	cp -pr src/c++/libunwind/include/__libunwind_config.h include/
	mv include/c++/__cxxabi_config.h include/

$(Configs):: musl.install
	$(OUT)echo Making libc++ for $@ in c++-build/$@
	$(OUT)mkdir -p c++-build/$@
	$(OUT)cp Makefile.lib c++-build/$@/Makefile
	$(OUT)$(MAKE) $(MFLAGS) -C c++-build/$@ || exit 1
	$(OUT)cp c++-build/$@/libc++.a lib/$@

VDIRS=$(shell cd ../lib/clang/; echo *)
VER=$(word $(words $(VDIRS)), $(VDIRS))
clang-headers:
	$(OUT)echo Installing clang headers
	$(OUT)if [ -e  ../lib/clang/$(VER)/include ] ; then \
	  mkdir -p clang ; \
	    cp -p -r ../lib/clang/$(VER)/include/* clang ; \
	    cp -p clang/arm_neon.h include ; \
	    cp -p clang/altivec.h include ; \
	    cp -p clang/emmintrin.h include ; \
	    cp -p clang/mmintrin.h include ; \
	    cp -p clang/mm_malloc.h include ; \
	    cp -p clang/xmmintrin.h include ; \
	    cp -p clang/f16cintrin.h include ; \
	    cp -p clang/stdatomic.h include ; \
	    cp -p clang/cpuid.h include ; \
	  fi

$(ElkConfigs):: musl.install
	$(OUT)echo Making libelk for $@ in elk-build/$@
	$(OUT)mkdir -p elk-build/$@
	$(OUT)mkdir -p elk-build/$@/bin
	$(OUT)cp Makefile.lib elk-build/$@/Makefile
	$(OUT)cp -pr src/elk/bin/* elk-build/$@/bin
	$(OUT)$(MAKE) $(MFLAGS) -C elk-build/$@ || exit 1
	$(OUT)$(MAKE) $(MFLAGS) -C elk-build/$@/bin || exit 1
	$(OUT)$(MAKE) $(MFLAGS) LIBDIR=`pwd`/lib/$@ -C elk-build/$@ install || exit 1
