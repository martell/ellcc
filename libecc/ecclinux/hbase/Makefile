include config.mk
PROGS=hd patch pgrep stty

all: $(PROGS)

.PHONY: $(PROGS)

$(PROGS):
	cd $@; $(CC) $(CPPFLAGS) $(CFLAGS) -I.. -DUCB -o $@ *.c

install: all
	@for p in $(PROGS) ; do cp $$p/$$p $(PREFIX)/bin ; done
	@for p in $(PROGS) ; do \
          if [ -e $$p/$$p.1 ] ; then \
            cp $$p/$$p.1 $(MANPREFIX)/man1 ; \
	  fi ; \
        done

clean:
	for p in $(PROGS) ; do rm -f $$p/$$p ; done
