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

#ident	"@(#)rfudaemon:rfudaemon.mk	1.6"

DESTDIR=
BINS=rfudaemon
CC= /usr/5bin/cc
CFLAGS= -O
LDLIBS= -lns
FRC=

.KEEP_STATE:

all: $(BINS)

.c:
	$(LINK.c) -o $@ $< $(LDLIBS)

install: $(BINS)
	install -m 755 -o root -g bin $(BINS) $(DESTDIR)/usr/bin

clean:
	rm -f *.o $(BINS) core .make.state

FRC:
