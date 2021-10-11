#
#       @(#)Makefile 1.1 92/07/30 SMI
#
#       Copyright 1987 Sun Microsystems 
#
#       Copyright (c) 1984 AT&T
#         All Rights Reserved

#       THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#       The copyright notice above does not evidence any
#       actual or intended publication of such source code.

#ident	"@(#)fuser:fuser.mk	1.1.1.1"

ARCH:sh = arch -k
DESTDIR=
BINS=fuser
CC= /usr/5bin/cc
CFLAGS= -O -D${ARCH}
LDLIBS=-lkvm
FRC=

.KEEP_STATE:

all: $(BINS)

.c:
	$(LINK.c) -o $@ $< $(LDLIBS)

install: $(BINS)
	install -d $(DESTDIR)/usr/kvm ; \
	install -s $(BINS) $(DESTDIR)/usr/kvm ; \
	rm -f $(DESTDIR)/usr/etc/$(BINS) ; \
	install -d $(DESTDIR)/usr/etc; \
	ln -s ../kvm/$(BINS) $(DESTDIR)/usr/etc/$(BINS) ; \
	rm -f $(DESTDIR)/etc/$(BINS) ; \
	install -d $(DESTDIR)/etc ; \
	ln -s ../usr/etc/$(BINS) $(DESTDIR)/etc/$(BINS) ;

clean:
	$(RM) $(BINS) core *.o .make.state

FRC:
