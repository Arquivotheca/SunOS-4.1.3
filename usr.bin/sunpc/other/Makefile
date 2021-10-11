#
# @(#)Makefile 1.1 92/07/30 SMI
#

DESTDIR=
CFLAGS= -O
CC=/bin/cc

ALL = unix2dos dos2unix

.KEEP_STATE:

all: ${ALL}

unix2dos: unix2dos.c
	${CC} ${CFLAGS} -o unix2dos unix2dos.c

dos2unix: dos2unix.c
	${CC} ${CFLAGS} -o dos2unix dos2unix.c

install: all
	install -s -m 755 unix2dos $(DESTDIR)/usr/bin
	install -s -m 755 dos2unix $(DESTDIR)/usr/bin

clean: FRC
	rm -f ${ALL}

FRC:

