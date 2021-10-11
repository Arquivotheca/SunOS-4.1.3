#	Copyright (c) 1984 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident "@(#)Makefile 1.1 92/07/30 SMI"; /* from S5R3.1 1.8 */

TESTDIR = .
LIB = a.a
CFLAGS = -O -I$(INCDIR)
INCDIR = ..

FILES= \
	pnpsplit.o \
	copyn.o \
	devtolin.o \
	expand.o \
	lintodev.o \
	namtouid.o \
	tmless.o \
	tmsecs.o \
	uidtonam.o \
	substr.o \
	regex.o \
	regcmp.o

all:	$(LIB) 

$(LIB):	$(FILES)
	ar rv $@ $(FILES)
	ranlib $@
	@echo lib $(LIB) is now up to date

clean:
	-rm -f *.o

clobber:	clean
	-rm -f $(LIB)
