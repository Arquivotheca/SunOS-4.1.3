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

#ident	"@(#)rfuadmin:rfuadmin.mk	1.3"

#	rfuadmin make file

DESTDIR=
BINS=rfuadmin

all:	$(BINS)

$(BINS):
	sh $(BINS).sh
	chmod 755 $(BINS)

install: $(BINS)
	install -m 755 -o root -g bin $(BINS) $(DESTDIR)/usr/bin

clean:
	rm -f $(BINS)
