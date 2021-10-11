#
# Copyright (c) 1983 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
# @(#)Makefile 1.1 92/07/30 SMI; from UCB 5.3 5/7/86
#
# makefile for line printer spooling system
#
# Ownerships {see note at the top of lpd.c}
# ROOT		a user that can access any and all files
# DAEMON	someone special
# SPGRP		the group id of the spooling programs
#

.KEEP_STATE:

CFLAGS	= -O
LINTFLAGS = -hv
DESTDIR	=
LIBDIR	= ${DESTDIR}/usr/lib
BINDIR	= ${DESTDIR}/usr/ucb
SYSVBINDIR = ${DESTDIR}/usr/bin
SPOOLD	= ${DESTDIR}/var/spool
LINT	= /usr/bin/lint
SPOOLDIRS = lpd
CXREF	= /usr/ucb/ctags -x
ROOT	= root
DAEMON	= daemon
SPGRP	= daemon

COMMONSRC = startdaemon.c printcap.c common.c 
LPRSRC	= lpr.c startdaemon.c printcap.c
LPROBJ	= ${LPRSRC:.c=.o}
LPDSRC	= lpd.c printjob.c recvjob.c displayq.c rmjob.c lpdchar.c ${COMMONSRC}
LPDOBJ	= ${LPDSRC:.c=.o}
LPQSRC  = lpq.c displayq.c printcap.c common.c
LPQOBJ	= ${LPQSRC:.c=.o}
LPCSRC	= lpc.c cmds.c cmdtab.c ${COMMONSRC}
LPCOBJ	= ${LPCSRC:.c=.o}
LPRMSRC	= lprm.c rmjob.c ${COMMONSRC}
LPRMOBJ	= ${LPRMSRC:.c=.o}
LPTESTSRC = lptest.c
LPTESTOBJ = ${LPTESTSRC:.c=.o}
PACSRC	= pac.c printcap.c
PACOBJ	= ${PACSRC:.c=.o}
CANCELSRC = cancel.c rmjob.c ${COMMONSRC}
CANCELOBJ = ${CANCELSRC:.c=.o}
LPSTATSRC = lpstat.c displayq.c printcap.c common.c
LPSTATOBJ = ${LPSTATSRC:.c=.o}
SRCS	= lpd.c lpr.c lpq.c lprm.c pac.c lpd.c lpc.c lptest.c cmds.c cmdtab.c \
	printjob.c recvjob.c displayq.c rmjob.c \
	startdaemon.c common.c printcap.c lpdchar.c lpstat.c cancel.c
OBJS	= ${SRCS:.c=.o}
HDRS	= lp.h lp.local.h

ALL	= lpd lpr lpq lprm lpc lptest pac lpstat cancel

all:	${ALL} FILTERS

lpd:	${HDRS} ${LPDOBJ}
	${CC} -o lpd ${LPDOBJ}

lpr:	${HDRS} ${LPROBJ}
	${CC} -o lpr ${LPROBJ}

lpq:	${HDRS} ${LPQOBJ}
	${CC} -o lpq ${LPQOBJ} -ltermcap

lprm:	${HDRS} ${LPRMOBJ}
	${CC} -o lprm ${LPRMOBJ}

lpc:	${HDRS} lpc.h ${LPCOBJ}
	${CC} -o lpc ${LPCOBJ}

lptest:	${HDRS} ${LPTESTOBJ}
	${CC} -o lptest ${LPTESTOBJ}

pac:	${HDRS} ${PACOBJ}
	${CC} -o pac ${PACOBJ}

cancel:	${HDRS} ${CANCELOBJ}
	${CC} -o cancel ${CANCELOBJ}

lpstat:	${HDRS} ${LPSTATOBJ}
	${CC} -o lpstat ${LPSTATOBJ}

FILTERS:
	cd filters; make ${MFLAGS}

install: all printcap.sample
	install -d -o bin -m 755 ${SPOOLD}
	install -s -o ${ROOT} -g ${SPGRP} -m 6711 lpd ${LIBDIR}/lpd
	install -s -o ${ROOT} -g ${SPGRP} -m 6711 lpr ${BINDIR}/lpr
	rm -f ${DESTDIR}/usr/bin/lp; ln -s ../ucb/lpr ${SYSVBINDIR}/lp

	install -s -o ${ROOT} -g ${SPGRP} -m 6711 lpq ${BINDIR}/lpq
	install -s -o ${ROOT} -g ${SPGRP} -m 6711 lprm ${BINDIR}/lprm
	install -s -o ${ROOT} -g ${SPGRP} -m 6711 cancel ${SYSVBINDIR}/cancel
	install -s -o ${ROOT} -g ${SPGRP} -m 6711 lpstat ${SYSVBINDIR}/lpstat

	install -s -g ${SPGRP} -m 2711 lpc ${DESTDIR}/usr/etc/lpc
	install -s lptest ${BINDIR}/lptest
	install -s pac ${DESTDIR}/usr/etc/pac
	install -c -m 644 printcap.sample ${DESTDIR}/etc/printcap
	set -x; for i in ${SPOOLDIRS}; do \
		install -d -o ${DAEMON} -g ${SPGRP} -m 775 ${SPOOLD}/$$i ; \
	done
	cd filters; make ${MFLAGS} DESTDIR=${DESTDIR} install

lint:
	$(LINT) ${LINTFLAGS} ${LPRSRC}
	$(LINT) ${LINTFLAGS} ${LPDSRC}
	$(LINT) ${LINTFLAGS} ${LPQSRC}
	$(LINT) ${LINTFLAGS} ${LPCSRC}
	$(LINT) ${LINTFLAGS} ${LPRMSRC}

clean:
	rm -f ${ALL} ${OBJS}
	cd filters; make ${MFLAGS} clean
