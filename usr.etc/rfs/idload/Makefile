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

#ident	"@(#)idload:idload.mk	1.6"

DESTDIR=
BINS=idload
CC= /usr/5bin/cc
CFLAGS= -O -DRFS
LDLIBS= -lns
FRC=

.KEEP_STATE:

all: $(BINS)

.c:
	$(LINK.c) -o $@ $< $(LDLIBS)

install: $(BINS)
	install -d $(DESTDIR)/usr/bin
	install -s $(BINS) $(DESTDIR)/usr/bin
	chmod 0555 $(DESTDIR)/usr/bin/idload

clean:
	$(RM) .make.state $(BINS) *.o core

FRC:
