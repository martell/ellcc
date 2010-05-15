# Makefile for toplevel ELLCC distribution

# Just do the obvious recursive thing.
all:
	$(MAKE) -C ecc

check:
	$(MAKE) -C ecc check

clean:
	$(MAKE) -C ecc clean

distclean:
	$(MAKE) -C ecc distclean

doc:
	$(MAKE) -C ecc doc
