#	Copyright (c) 1984 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#
# @(#)Makefile 1.1 92/07/30 SMI; from S5R3 1.6
#
#	ctrace makefile
# installation directories:
BIN = /usr/bin
LIB = /usr/lib/ctrace
INS = install

# setting preprocessor symbols:
# set for UNIX/370:
# U370 = -b1,0
CFLAGS	= -O -DLIB=\"$(LIB)

YFLAGS	= -d

SOURCE	 = constants.h global.h main.c parser.y scanner.l lookup.c trace.c \
	   runtime.c ctcr
OTHER	 = Makefile
OBJECTS	 = main.o parser.o scanner.o lookup.o trace.o

ctrace:	$(OBJECTS)
	@echo '(expect 13 shift/reduce)'
	$(CC) -o ctrace $(LDFLAGS) $(OBJECTS)

install: ctrace runtime.c
	$(INS) -s ctrace $(DESTDIR)$(BIN)
	$(INS) -d -o bin -m 755 $(DESTDIR)$(LIB)
	$(INS) -m 644 -c runtime.c $(DESTDIR)$(LIB)

clean:
	-rm -f *.o y.* yacc.* y.tab.h y.output
	-rm -f ctrace

lookup.o: global.h y.tab.h constants.h
main.o: global.h constants.h
parser.y: global.h constants.h
scanner.o: global.h y.tab.h constants.h
trace.o: global.h constants.h
y.tab.h: parser.o

