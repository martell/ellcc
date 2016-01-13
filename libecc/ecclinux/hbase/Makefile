include config.mk
PROGS=hd install patch pgrep stty

all: $(PROGS)

.PHONY: $(PROGS)

$(PROGS):
	cd $@; $(CC) $(CPPFLAGS) $(CFLAGS) -I.. -DUCB -o $@ *.c

clean:
	for p in $(PROGS) ; do rm -f $$p/$$p ; done
