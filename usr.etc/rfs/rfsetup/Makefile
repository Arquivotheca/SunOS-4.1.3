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

#ident	"@(#)rfsetup:rfsetup.mk	1.9"

DESTDIR=
BINS=rfsetup
CC=/usr/5bin/cc
LDLIBS = -lns -lnsl
LOG=-DLOGGING
CFLAGS= -O -DRFS $(LOG)
RFUID = root
RFGID = staff
FRC=

.KEEP_STATE:

all: $(BINS)

.c:
	$(LINK.c) -o $@ $< $(LDLIBS)

install: $(BINS)
	install -d -m 755 ${DESTDIR}/var
	install -d -m 755 ${DESTDIR}/var/net
	install -d -m 755 ${DESTDIR}/var/net/nls
	install -d -m 755 ${DESTDIR}/var/net/servers
	install -d -m 755 ${DESTDIR}/var/net/servers/rfs
	install -m 04755 -o $(RFUID) -g $(RFGID) $(BINS) ${DESTDIR}/usr/etc
	cd ${DESTDIR}/var/net/servers/rfs; rm -rf $(BINS); ln -s ../../../../usr/etc/$(BINS) $(BINS)
	cd ${DESTDIR}/usr; rm -rf net; ln -s ../var/net net

clean:
	rm -f *.o $(BINS) core .make.state
FRC:
