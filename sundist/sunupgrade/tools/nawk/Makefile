#	Copyright (c) 1984 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#
# @(#)Makefile 1.2 89/09/29 SMI; from S5R3.1 2.6
#

CC=/usr/bin/cc
CFLAGS += -O
YFLAGS=-d
LIBS=	-lm -ll
LDFLAGS=-Bstatic
LIST = lpr
FILES=	awk.g.o       awk.lx.o b.o lib.o main.o parse.o \
	proctab.o run.o tran.o

#SOURCE=	awk.g.y awk.h awk.lx.l b.c lib.c main.c parse.c \
#	proctab.c run.c tran.c

CFILES=b.c lib.c main.c parse.c run.c tran.c maketab.c
HFILES=awk.h 
YFILES=awk.g.y awk.lx.l

CLEANFILES= awk* awk.g.c awk.g.o awk.g.y awk.h awk.lx.l awk.lx.o \
	b.c b.o lib.c lib.o main.c main.o maketab* maketab.c parse.c \
	parse.o prevy.tab.h proctab.c proctab.o run.c run.o \
	tran.c tran.o y.tab.h

all:  awk

awk:	$(FILES)
	$(CC) $(CFLAGS) $(LDFLAGS) $(FILES) $(LIBS) -o awk

${CFILES}:
	cp ../../../../usr.bin/nawk/$@ .

${HFILES}:
	cp ../../../../usr.bin/nawk/$@ .

${YFILES}:
	cp ../../../../usr.bin/nawk/$@ .

$(FILES):	awk.h prevy.tab.h

awk.g.o:	awk.h awk.g.c

y.tab.h awk.g.c:	awk.g.y
		$(YACC) $(YFLAGS) awk.g.y
		mv y.tab.c awk.g.c

prevy.tab.h:	y.tab.h
	-cmp -s y.tab.h prevy.tab.h || (cp y.tab.h prevy.tab.h; echo change maketab)

proctab.c:	maketab
	./maketab >proctab.c

maketab:	prevy.tab.h maketab.c
	cc maketab.c -o maketab

lint:
	lint -pu b.c main.c token.c tran.c run.c lib.c parse.c -lm |\
		egrep -v '^(error|free|malloc)'

install: 
		install -s -m 755 awk ${DESTDIR}/toolkit/nawk

clean:
	for i in ${CLEANFILES}; do \
		rm -f $$i;done


