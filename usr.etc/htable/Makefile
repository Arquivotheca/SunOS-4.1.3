#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile 1.1 92/07/30 SMI from 5.2 9/18/85
#
BINS=	htable
OBJS=   htable.o parse.o scan.o
HDRS=   htable.h y.tab.h

CFLAGS=	-O

.KEEP_STATE:

all:	$(BINS)

$(BINS) : $(HDRS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS)

y.tab.h: parse.y
	$(YACC) -d parse.y
	$(RM) y.tab.c

clean:
	$(RM) $(BINS) $(OBJS) y.tab.h

lint: parse.c scan.c htable.c
	lint parse.c scan.c htable.c

install: $(BINS)
	install -d $(DESTDIR)/usr/etc
	install -s $(BINS) $(DESTDIR)/usr/etc
