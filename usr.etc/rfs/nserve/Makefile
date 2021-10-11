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

#ident	"@(#)nserve:nserve.mk	1.11"

ROOT =
DESTDIR =
CC = /usr/5bin/cc
NSLIB = -lns
NSL = nsl
LIB = $(NSLIB) -l$(NSL)
LLIB = $(ROOT)/usr/src/lib/libns/llib-lns.ln
LOG=-DLOGGING -DLOGMALLOC
PROFILE=
DEBUG=
CFLAGS=-O -DRFS $(DEBUG) $(LOG) $(PROFILE)
EXECS=nserve
SOURCE=nserve.c nsrec.c nsfunc.c nsdb.c
OBJECTS=nserve.o nsrec.o nsfunc.o nsdb.o
FRC=

.MAKE.STATE:

.INIT: nsdb.h nslog.h nsports.h nsrec.h stdns.h

all:	$(EXECS)
nserve: $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) $(LIB) -o nserve $(LDLIBS)
debug:
	make -f nserve.mk DEBUG="-g -DLOGGING -DLOGMALLOC" all
dashg:
	make -f nserve.mk NSL=nsl NSLIB=-lnsdb DEBUG="-g -DLOGGING -DLOGMALLOC" all
lint:
	lint -pua $(SOURCE) $(LLIB)

install: $(EXECS)
	install -d -m 755 ${DESTDIR}/etc/nserve
	install -d -m 755 ${DESTDIR}/etc/nserve/auth.info
	install -s nserve $(DESTDIR)/usr/etc;
	cd ${DESTDIR}/etc/nserve; rm -f nserve; ln -s ../../usr/etc/nserve nserve
	cd ${DESTDIR}/usr; rm -rf nserve; ln -s ../etc/nserve nserve

uninstall:
	-rm -f $(DESTDIR)/usr/etc/nserve
	-rm -rf $(DESTDIR)/etc/nserve $(DESTDIR)/usr/nserve

clean:
	-rm -f *.o core $(EXECS)

FRC:
