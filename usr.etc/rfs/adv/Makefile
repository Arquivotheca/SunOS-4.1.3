#
#       @(#)Makefile 1.1 92/07/30 SMI
#
#       Copyright 1987 Sun Microsystems 
#
#       Copyright (c) 1984 AT&T
#         All Rights Reserved

#       THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#       The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)adv:adv.mk	1.6"

DESTDIR=
BINS=adv
CC= /usr/5bin/cc
CFLAGS= -DNAMESERVER -O
LDLIBS= -lns
FRC=

.KEEP_STATE:

all: $(BINS)

.c:
	$(LINK.c) -o $@ $< $(LDLIBS)

install: $(BINS)
	install -d $(DESTDIR)/usr/bin
	install -s $(BINS) $(DESTDIR)/usr/bin

clean:
	$(RM) $(BINS) *.o core .make.state

FRC:
