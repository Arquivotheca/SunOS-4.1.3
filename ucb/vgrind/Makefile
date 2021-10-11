#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
# @(#)Makefile 1.1 92/07/30 SMI; from UCB 5.1 (Berkeley) 6/6/85
#
DESTDIR=
CFLAGS=-O 
SOURCES=vfontedpr.c vgrindefs.c regexp.c retest.c
CMDS=vfontedpr retest
OBJS=retest.o regexp.o vfontedpr.o vgrindefs.o
TMAC=tmac.vgrind

.DEFAULT:
	sccs get $@

all:	$(CMDS) ${OBJS} ${TMAC}

vfontedpr: vfontedpr.o vgrindefs.o regexp.o
	${CC} ${CFLAGS} -o vfontedpr vfontedpr.o vgrindefs.o regexp.o
retest: retest.o regexp.o
	${CC} ${CFLAGS} -o retest retest.o regexp.o

install: all vgrind.sh vgrindefs.src
	install -s vfontedpr ${DESTDIR}/usr/lib/vfontedpr
	install -c vgrind.sh ${DESTDIR}/usr/ucb/vgrind
	install -c -m 444 vgrindefs.src ${DESTDIR}/usr/lib/vgrindefs
	install -c -m 444 tmac.vgrind ${DESTDIR}/usr/share/lib/tmac/tmac.vgrind

clean:
	rm -f ${CMDS} ${OBJS}

vprintall:
	csh /usr/ucb/vprint ${SOURCES}
