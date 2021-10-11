#	Copyright (c) 1984 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#
# @(#)Makefile 1.1 92/07/30 SMI; from S5R3.1 1.1
#

#	Makefile for chrtbl

HDIR = ../../../lib/libc/gen/common
HFILES = $(HDIR)/euc.h $(HDIR)/codeset.h $(HDIR)/iso2022.h
CTYPEDIR = usr/share/lib/locale/LC_CTYPE

CTYPEFILES = C iso_8859_1

CFLAGS = -O -I$(HDIR)

.INIT: $(HFILES)

all: chrtbl $(CTYPEFILES)

C: C.ct
	chrtbl C.ct

iso_8859_1: iso_8859_1.ct
	chrtbl iso_8859_1.ct

clean:
	rm -f chrtbl $(CTYPEFILES)

install: chrtbl $(CTYPEFILES)
	install -d -m 755 $(DESTDIR)/$(CTYPEDIR)
	install -s chrtbl $(DESTDIR)/usr/etc
	for i in $(CTYPEFILES); do \
		install -m 644 $$i $(DESTDIR)/$(CTYPEDIR); \
	done
	rm -f $(DESTDIR)/$(CTYPEDIR)/default
	ln $(DESTDIR)/$(CTYPEDIR)/C $(DESTDIR)/$(CTYPEDIR)/default

chrtbl.c: $(HFILES)
