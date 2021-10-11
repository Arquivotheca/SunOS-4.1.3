#
# @(#)Makefile 1.1 92/07/30 SMI
#

DESTDIR=
CFLAGS=	-O
OBJS=	lifetool.o life.o circle.o patterns.o
LIBS= -lsuntool -lsunwindow -lpixrect
CC= /bin/cc

life: ${OBJS}
	$(CC) ${CFLAGS} -o life ${OBJS} ${LIBS}

${OBJS}: life.h

install: life
	install -s life ${DESTDIR}/usr/games/life
clean:
	rm -f ${OBJS} life
