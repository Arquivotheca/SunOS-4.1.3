#	@(#)Makefile 1.1 92/07/30 SMI
#
#	Copyright (c) 1987 Sun Microsystems

#	Copyright (c) 1984 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)rmntstat:rmntstat.mk	1.3"
DESTDIR =
BIN=rmntstat
GNODESDIR = ../fusage
INC = /usr/include
CFLAGS = -O -c -DRFS
LDFLAGS = -lkvm
FRC =

.MAKE.STATE:

.INIT: fumount.h

all: $(BIN)

rmntstat: rmntstat.o getnodes.o
	$(CC) -o rmntstat $(LDFLAGS) rmntstat.o getnodes.o
	chmod 755 rmntstat

rmntstat.o : rmntstat.c \
	$(INC)/stdio.h $(INC)/fcntl.h $(INC)/sys/types.h \
	$(INC)/rfs/rfs_misc.h $(INC)/rfs/adv.h \
	$(INC)/rfs/nserve.h $(INC)/sys/stat.h \
	$(INC)/rfs/rfs_mnt.h $(INC)/rfs/fumount.h
	$(CC) -I$(INC) -c $(CFLAGS) rmntstat.c

getnodes.o : $(GNODESDIR)/getnodes.c \
	$(INC)/nlist.h $(INC)/stdio.h $(INC)/fcntl.h \
	$(INC)/sys/types.h $(INC)/rfs/sema.h \
	$(INC)/rfs/comm.h \
	$(INC)/rfs/cirmgr.h $(INC)/rfs/idtab.h \
	$(INC)/sys/mount.h $(INC)/rfs/adv.h $(INC)/rfs/fumount.h
	$(CC) -c $(CFLAGS) $(GNODESDIR)/getnodes.c

install: $(BIN)
	install -s $(BIN) $(DESTDIR)/usr/bin

clean:
	rm -f *.o $(BIN)

clobber: clean
	rm -f rmntstat
FRC:
