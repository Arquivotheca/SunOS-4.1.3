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

#ident  "@(#)mount_rfs:mount_rfs.mk 1.6"

DESTDIR=
BINS=mount_rfs
CC= /bin/cc
CFLAGS=	-O -DRFS
LDLIBS= -lns -lnsl
FRC=

.KEEP_STATE:

all: $(BINS)

.c:
	$(LINK.c) -o $@ $< $(LDLIBS)

install: $(BINS)
	install -s $(BINS) $(DESTDIR)/usr/etc
	rm -f ${DESTDIR}/etc/$(BINS)
	cd $(DESTDIR)/etc;ln -s ../usr/etc/$(BINS) $(BINS)

clean:
	$(RM) .make.state $(BINS) *.o core

FRC:
