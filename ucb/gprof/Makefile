#
# @(#)Makefile 1.1 92/07/30 SMI; from UCB 5.2 6/18/85
#
# Copyright (c) 1983 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#

DFLAGS = 
CFLAGS = -O ${DFLAGS}
LINTFLAGS = -x -a -n ${DFLAGS}

RM=	/bin/rm -f
PR=pr

.DEFAULT:
	sccs get $@

GPROFHDRS =	gprof.h vax.h m68k.h sparc.h gcrt0.h
GPROFSRCS =	gprof.c arcs.c dfn.c lookup.c calls.vax.c calls.m68k.c \
		calls.sparc.c hertz.c printgprof.c printlist.c
GPROFOBJS =	gprof.o arcs.o dfn.o lookup.o calls.o hertz.o \
		printgprof.o printlist.o

gprof: ${GPROFOBJS}
	$(CC) -o gprof ${CFLAGS} ${GPROFOBJS}

install: gprof flat.blurb callg.blurb
	install -s gprof ${DESTDIR}/usr/ucb/gprof
	install -c -m 644 flat.blurb ${DESTDIR}/usr/lib/gprof.flat.blurb
	install -c -m 644 callg.blurb ${DESTDIR}/usr/lib/gprof.callg.blurb

calls.o:
	case `mach` in \
	vax)     ${CC} -c ${CFLAGS} calls.vax.c; mv calls.vax.o calls.o;;\
	sparc)   ${CC} -c ${CFLAGS} calls.sparc.c;  mv calls.sparc.o calls.o;;\
	mc680?0) ${CC} -c ${CFLAGS} calls.m68k.c; mv calls.m68k.o calls.o;;\
	*) echo 'unknown machine' ; exit 1 ;;\
	esac

clean:
	${RM} ${GPROFOBJS} errs gprof

#
# You can't get gcrt0.h from ".../csu/..." because it doesn't exist
#
#gcrt0.h:
#	cp ${DESTDIR}/usr/src/libc/csu/gcrt0.h gcrt0.h

print:
	@ ls -l | ${PR}
	@ ${PR} makefile
	@ ${PR} gcrt0.h
	@ ${PR} ${GPROFHDRS} ${GPROFSRCS}
	@ ${PR} gprof.flat.blurb gprof.callg.blurb

lint:
	lint ${LINTFLAGS} ${DFLAGS} ${GPROFSRCS}

gprof.o: gprof.c ${GPROFHDRS}
arcs.o: arcs.c ${GPROFHDRS}
lookup.o: lookup.c ${GPROFHDRS}
dfn.o: dfn.c ${GPROFHDRS}
calls.o: calls.vax.c calls.m68k.c calls.sparc.c ${GPROFHDRS}
printgprof.o: printgprof.c ${GPROFHDRS}
printlist.o: printlist.c ${GPROFHDRS}
