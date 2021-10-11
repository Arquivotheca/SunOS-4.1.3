#
# @(#)Makefile 1.1 92/07/30 SMI; from UCB
#
#
# Nifty foo monopoly maker
#	Program:  Ken Arnold
#	Makefile:  K.S.
#
DESTDIR=
DATFILES=brd.dat mon.dat prop.dat
CFILES=	monop.c cards.c execute.c getinp.c houses.c jail.c misc.c morg.c \
	print.c prop.c rent.c roll.c spec.c strcmp.c trade.c
OBJS=	monop.o cards.o execute.o getinp.o houses.o jail.o misc.o morg.o \
	print.o prop.o rent.o roll.o spec.o strcmp.o trade.o 
HEADERS=monop.h deck.h
BINDIR=	${DESTDIR}/usr/games
LIBDIR=	${DESTDIR}/usr/games/lib
LIB=
CFLAGS=	-O -w
DAT=	brd.dat monop.dat prop.dat
CC=	cc
ED=	ed
AS=	as
RM=	rm -f
CTAGS=	ctags

it:	monop cards.pck

monop:	${OBJS}
	${CC} -o monop -n ${OBJS} ${LIB}

${OBJS}: ${HEADERS} monop.ext

install: monop cards.pck
	install -s monop ${BINDIR}/monop
	install -c -m 644 cards.pck ${LIBDIR}/cards.pck

new:	clean ctags monop

monop.o: ${DATFILES} monop.def

newone:
	${CC} -o monop ${CFLAGS} ${CFILES} ${LIB}

ctags:
	${CTAGS} ${CFILES} monop.h deck.h monop.def initdeck.c

cards.pck: initdeck cards.inp
	./initdeck

initdeck: initdeck.c deck.h
	${CC} -o initdeck ${CFLAGS} initdeck.c ${LIB}

monop.tar:
	tar crvf monop.tar ${CFILES} ${DATFILES} ${HEADERS} :rofix Makefile cards.inp initdeck.c monop.def monop.ext

clean:
	${RM} ${OBJS} monop core tags x.c x.o monop.tar

printall:
	-/usr/bin/eecs/pr monop.h deck.h monop.def monop.ext ${CFILES} ${DATFILES} | /usr/bin/lpr
	-/usr/bin/lpq

lint:
	lint ${CFILES} > lint.out ;
