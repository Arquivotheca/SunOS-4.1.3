#
# @(#)Makefile 1.1 92/07/30 SMI; from UCB 1.2 05/19/83
#
# make file for cribbage
#
DESTDIR=
HDRS=	cribbage.h deck.h cribcur.h
OBJS=	extern.o crib.o support.o cards.o score.o io.o
CFILES=	extern.c crib.c support.c cards.c score.c io.c
TOBJS=	test.o cards.o score.o io.o
CFLAGS=	-O

all: cribbage crib.instr

cribbage:	${OBJS}
	${CC} ${CFLAGS} -o cribbage ${OBJS} -lcurses -ltermlib

crib.o io.o support.o: cribcur.h

test:		${TOBJS}
	${CC} ${CFLAGS} -o test ${TOBJS}

crib.instr: cribbage.n macro
	nroff cribbage.n > crib.instr

tags: ${HDRS} ${CFILES}
	ctags -u $?
	ed - tags < :ctfix
	sort tags -o tags

clean:
	rm -f ${OBJS} ? a.out core crib.instr cribbage

crib.o:		deck.h	cribbage.h	cribcur.h
extern.o:	deck.h	cribbage.h	
support.o:	deck.h	cribbage.h	cribcur.h
test.o:		deck.h
cards.o:	deck.h
score.o:	deck.h
io.o:		deck.h			cribcur.h

install: cribbage crib.instr
	install -c -s cribbage ${DESTDIR}/usr/games/
	install -c -m 644 crib.instr ${DESTDIR}/usr/games/lib/
