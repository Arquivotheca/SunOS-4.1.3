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

#ident	"@(#)rfadmin:rfadmin.mk	1.1"

ROOT=
DESTDIR=
BINS=rfadmin
CC=/usr/5bin/cc
CFLAGS = -O
LDLIBS = -lns
FRC=

.MAKE.STATE:

all: $(BINS)

.c:
	$(LINK.c) -o $@ $< $(LDLIBS)

install: $(BINS)
	install -s $(BINS) $(DESTDIR)/usr/bin/$(BINS)

clean:
	rm -f .make.state *.o $(BINS) core

FRC:
