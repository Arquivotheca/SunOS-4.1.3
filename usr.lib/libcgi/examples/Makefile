#
# @(#)Makefile 1.1 92/07/30 SMI
#
# Copyright (c) 1985, 1986, 1987, 1988, 1989 by Sun Microsystems, Inc.
# Permission to use, copy, modify, and distribute this software for any
# purpose and without fee is hereby granted, provided that the above
# copyright notice appear in all copies and that both that copyright
# notice and this permission notice are retained, and that the name
# of Sun Microsystems, Inc., not be used in advertising or publicity
# pertaining to this software without specific, written prior permission.
# Sun Microsystems, Inc., makes no representations about the suitability
# of this software or the interface defined in this software for any
# purpose. It is provided "as is" without express or implied warranty.
#
#
# ----------- Makefile for SunCGI graphics example programs -----------
#
C_DEMOS=	ex1-1 ex2-1 ex2-2 ex2-3 ex3-1.1 ex3-1.2 ex4-1 ex4-6 \
		ex5-2 ex5-3 exD-1 exD-2 exD-3 exE-4.1 exE-4.2
FTN_DEMOS=	exF-2.1 exF-2.2
OTHER_DEMOS=
DEMOS=		${C_DEMOS} ${FTN_DEMOS} ${OTHER_DEMOS}
CGIINCLUDES=	-I..
OTHERINCLUDES=	
#CGILIBS=	-lcgi
CGILIBS=	../libcgi.a
CGI77LIBS=	../../libcgi77/libcgi77.a
TOOLLIBS=	-lsuntool
WINLIBS=	-lsunwindow -lpixrect -lm
#OPTIM=		-g
OPTIM=		-O
CPPFLAGS=	
CFLAGS=		${OPTIM} ${CGIINCLUDES} ${OTHERINCLUDES}
FFLAGS=		${OPTIM} ${CGIINCLUDES} ${OTHERINCLUDES}
LDFLAGS=	${TARGET_ARCH} ${CGILIBS} ${TOOLLIBS} ${WINLIBS}

.SUFFIXES: .c .f

.KEEP_STATE:

all:	${CGILIBS} ${CGI77LIBS} ${DEMOS}

.c:
	cc -o $@ $@.c ${CFLAGS} ${CPPFLAGS} ${CGILIBS} ${TOOLLIBS} ${WINLIBS}

.f:
	f77 -o $@ $@.f ${FFLAGS} \
		${CPPFLAGS} ${CGI77LIBS} ${CGILIBS} ${TOOLLIBS} ${WINLIBS}

install:
	rm -f $$cgi/doc/ref/src/*
	cp Makefile $$cgi/doc/ref/src
	cp README $$cgi/doc/ref/src
	for i in ${C_DEMOS}; do sed 's/\\/\\e/' $$i.c > $$cgi/doc/ref/src/$$i.c ; done
	for i in ${FTN_DEMOS}; do sed 's/\\/\\e/' $$i.f > $$cgi/doc/ref/src/$$i.f ; done

clean:
	-rm -f *.o ${DEMOS} errs core
