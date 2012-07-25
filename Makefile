# Build ELLCC.
VERSION=0.0.1

# Get the names of the subdirectories.
SUBDIRS := llvm-build gnu libecc

all install clean veryclean check:: llvm-build gnu/gnu-build
	@for dir in $(SUBDIRS) ; do \
	  echo Making $@ in $$dir ; \
	  $(MAKE) CLANG_VENDOR="ellcc $(VERSION)svn based on" -C $$dir $@ || exit 1 ; \
	done

install:: docs

llvm-build gnu/gnu-build:
	./configure

.PHONY: buildrelease
buildrelease:
	$(MAKE) -C llvm-build clean
	$(MAKE) CLANG_VENDOR="ellcc $(VERSION) based on" -C llvm-build install || exit 1 ; \

.PHONY: release
release:
	mkdir -p release
	rm -fr ellcc-$(VERSION)
	mkdir -p ellcc-$(VERSION)
	make -C libecc veryclean
	make -C workspace veryclean
	tar --exclude "*.svn*" --exclude "*/test/*" -cvp -f- bin libecc workspace | \
	    (cd ellcc-$(VERSION); tar xfp -)
	(cd ellcc-$(VERSION); tree -T "ELLCC Release Directory Tree" -H ellcc --nolinks > ../tree.html)
	tar -cvpz -frelease/ellcc-$(VERSION)-linux-x86_64.tgz ellcc-$(VERSION)

.PHONY: tagrelease
tagrelease:
	svn cp http://ellcc.org/svn/ellcc/trunk http://ellcc.org/svn/ellcc/tags/ellcc-$(VERSION)

.PHONY: untagrelease
untagrelease:
	svn rm http://ellcc.org/svn/ellcc/trunk http://ellcc.org/svn/ellcc/tags/ellcc-$(VERSION)

.PHONY: docs
docs:
	cp -r ./lib/share/doc/ld.html \
	./lib/share/doc/gdb \
	./lib/share/doc/binutils.html \
	./lib/share/doc/as.html \
	./share/doc/qemu \
	libecc/doc
