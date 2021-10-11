#
# @(#)Makefile 1.1 92/07/30 SMI; from UCB 5.2 85/09/07
#
#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#

DESTDIR=
CFLAGS= -O -DV7
OBJS= allow.o board.o check.o extra.o fancy.o init.o main.o move.o\
	odds.o one.o save.o subs.o table.o text.o message.o
TOBJS= allow.o board.o check.o data.o fancy.o init.o odds.o one.o save.o subs.o\
	table.o teach.o ttext1.o ttext2.o tutor.o
SRCS= allow.c board.c check.c data.c extra.c fancy.c init.c main.c\
	move.c odds.c one.c save.c subs.c table.c teach.c text.c ttext1.c\
	ttext2.c tutor.c message.c

.DEFAULT:
	sccs get -G $@ $@

all: backgammon teachgammon

#	Backgammon program
backgammon: $(OBJS)
	-rm -f backgammon
	$(CC) -o backgammon $(OBJS) -ltermlib

#	Backgammon rules and tutorial
teachgammon: ${TOBJS}
	-rm -f teachgammon
	$(CC) -o teachgammon $(TOBJS) -ltermlib

#	Header files back.h and tutor.h
allow.o board.o check.o extra.o fancy.o main.o move.o odds.o one.o save.o\
	subs.o table.o teach.o text.o ttext1.o ttext2.o: back.h
data.o tutor.o: back.h tutor.h

#	Update message.
message.o: FRC proto.message.c Mesgfix
	rm -f message.c
	cp proto.message.c message.c
	chmod 644 message.c
	ex - message.c < Mesgfix
	${CC} -c ${CFLAGS} message.c

FRC:

#	Installation
install: all
	install -s backgammon ${DESTDIR}/usr/games/backgammon
	install -s teachgammon ${DESTDIR}/usr/games/teachgammon

clean:
	rm -f ${OBJS} ${TOBJS} backgammon teachgammon message.c
