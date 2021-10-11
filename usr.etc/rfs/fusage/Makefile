#
#	@(#)Makefile 1.1 92/07/30 SMI
#
#	Copyright 1987 Sun Microsystems
#
#	Copyright (c) 1984 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)fusage:fusage.mk	1.3.3.1"

#	fusage make file

DESTDIR=
BIN=fusage
INC = /usr/include
GNODESDIR = ../fusage
CFLAGS = -O -c -I$(INC) -DRFS
LDFLAGS = -lkvm

.MAKE.STATE:

.INIT: fumount.h

all:	fusage

fusage: fusage.o getnodes.o
	$(CC) -o fusage $(LDFLAGS) fusage.o getnodes.o $(LDLIBS)
	chmod 755 fusage

fusage.o: fusage.c $(INC)/stdio.h $(INC)/sys/types.h \
		$(INC)/sys/stat.h $(INC)/mntent.h \
		$(INC)/rfs/fumount.h \
		$(INC)/rfs/rfsys.h $(INC)/errno.h

getnodes.o: $(GNODESDIR)/getnodes.c \
		$(INC)/nlist.h $(INC)/stdio.h $(INC)/fcntl.h \
		$(INC)/sys/types.h $(INC)/rfs/sema.h \
		$(INC)/rfs/comm.h $(INC)/rfs/cirmgr.h \
		$(INC)/sys/mount.h $(INC)/rfs/adv.h \
		$(INC)/rfs/fumount.h
	$(CC) $(CFLAGS) $(GNODESDIR)/getnodes.c 

install: $(BIN)
	install -s -m 04755 -o root -g bin $(BIN) $(DESTDIR)/usr/bin/$(BIN)

clean:
	-rm -f *.o core

clobber: clean
	rm -f fusage
