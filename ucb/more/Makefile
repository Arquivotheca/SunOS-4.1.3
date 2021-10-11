#
# @(#)Makefile 1.1 92/07/30 SMI; from UCB 4.2 83/07/03
#
CFLAGS=	-O
DESTDIR=

.DEFAULT:
	sccs get $@

more:	more.o
	${CC} -o more more.o -lcurses -ltermcap

install: more more.help
	install -s more ${DESTDIR}/usr/ucb/more
	rm -f ${DESTDIR}/usr/ucb/page
	ln ${DESTDIR}/usr/ucb/more ${DESTDIR}/usr/ucb/page
	install -c -m 644 more.help ${DESTDIR}/usr/lib/more.help

more.o:	more.c
more.o:	/usr/include/stdio.h
more.o:	/usr/include/ctype.h
more.o:	/usr/include/signal.h
more.o:	/usr/include/errno.h
more.o:	/usr/include/sgtty.h
more.o:	/usr/include/setjmp.h
more.o:	/usr/include/sys/types.h
more.o:	/usr/include/sys/stat.h

clean:
	rm -f more more.o core a.out errs
