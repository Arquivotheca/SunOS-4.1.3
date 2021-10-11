#
#       @(#)Makefile 1.1 92/07/30 SMI
#
#       Copyright 1987 Sun Microsystems 

DESTDIR=
BINS=hostrfs
CFLAGS= -O
FRC=

.KEEP_STATE:

all: $(BINS)

.c:
	$(LINK.c) -o $@ $<

install: $(BINS)
	install -d $(DESTDIR)/usr/bin
	install -s $(BINS) $(DESTDIR)/usr/bin

clean:
	rm -f .make.state $(BINS) *.o core

FRC:
