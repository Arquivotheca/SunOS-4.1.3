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

#ident	"@(#)fumount:fumount.mk	1.4.2.1"

#	fumount make file

DESTDIR=
BIN=fumount
INC = /usr/include
GNODESDIR = ../fusage
CFLAGS = -O -c -DRFS
LDFLAGS = -lkvm

.MAKE.STATE:

.INIT: fumount.h

all:	fumount

fumount: fumount.o getnodes.o sndmes.o
	$(CC) -o fumount $(LDFLAGS) fumount.o getnodes.o sndmes.o
	chmod 755 fumount

fumount.o: fumount.c $(INC)/stdio.h $(INC)/sys/types.h \
		$(INC)/rfs/rfs_misc.h $(INC)/rfs/message.h \
		$(INC)/rfs/rfsys.h $(INC)/rfs/nserve.h \
		$(INC)/rfs/fumount.h
	$(CC) $(CFLAGS) fumount.c

getnodes.o: $(GNODESDIR)/getnodes.c \
		$(INC)/nlist.h $(INC)/stdio.h $(INC)/fcntl.h \
		$(INC)/sys/types.h $(INC)/rfs/sema.h \
		$(INC)/rfs/comm.h $(INC)/rfs/cirmgr.h \
		$(INC)/sys/mount.h $(INC)/rfs/adv.h \
		$(INC)/rfs/fumount.h
	$(CC) $(CFLAGS) $(GNODESDIR)/getnodes.c

sndmes.o: sndmes.c $(INC)/stdio.h
	$(CC) $(CFLAGS) sndmes.c

debug.o: debug.c
	$(CC) $(CFLAGS) debug.c

install: $(BIN)
	install -s -m 755 -o root -g bin $(BIN) $(DESTDIR)/usr/bin/$(BIN)

clean:
	-rm -f *.o core $(BIN)

clobber: clean
	rm -f fumount
