#	Copyright (c) 1984 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#
# @(#)Makefile 1.1 92/07/30 SMI; from S5R3.1 2.6
#

CC=/usr/bin/cc
CFLAGS += -O
YFLAGS=-d
LIBS=	-lm -ll
LIST = lpr
FILES=	awk.g.o       awk.lx.o b.o lib.o main.o parse.o \
	proctab.o run.o tran.o
SOURCE=	awk.g.y awk.h awk.lx.l b.c lib.c main.c parse.c \
	proctab.c run.c tran.c

all:  awk

awk:	$(FILES)
	$(CC) $(CFLAGS) $(LDFLAGS) $(FILES) $(LIBS) -o awk

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

clean:
	rm -f a.out *.o t.* *temp* *.out *junk* y.tab.* ./maketab proctab.c yacc.*
	-rm -f awk.g.c prevy.tab.*
	-rm  -f awk

install: awk
	install -s awk $(DESTDIR)/usr/bin/nawk

listing:
	pr awk.mk $(SOURCE) | $(LIST)
