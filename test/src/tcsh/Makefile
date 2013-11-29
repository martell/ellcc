-include $(prefix)/libecc/mkscripts/targets/$(TARGET)/setup.mk

ifneq ($(TARGET),$(build))
  HOST=--host=$(TARGET)-$(OS)
  BUILD=--build=$(build)-$(OS)
else
  HOST=
  BUILD=
endif

ifneq ($(CC),gcc)
  ifeq ($(haslibs),yes)
    CFLAGS=$(CFLAGS.$(TARGET))
    CXXFLAGS=$(CXXFLAGS.$(TARGET))
  endif
endif

configure:
	cd $(DIR) ; \
	../src/configure \
        CC=$(CC) CFLAGS="$(CFLAGS)" \
        CXX=$(CXX) CXXFLAGS="$(CXXFLAGS)" \
        --bindir=$(bindir) --prefix=$(prefix) \
        $(HOST) $(BUILD) $(TARGETS)

clean:
	rm -fr build-*

