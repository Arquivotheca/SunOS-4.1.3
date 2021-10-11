#
# Copyright (c) 1983 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile 1.1 92/07/30 SMI; from UCB 5.3 9/7/85
#
BINS= restore
OBJS= dirs.o interactive.o main.o restore.o \
	symtab.o tape.o utilities.o
DUMPOBJS= dumprmt.o
DUMPSRCS= ../dump/dumprmt.c
DOPRNT= ../../lib/libc/stdio/common
SRCS= $(OBJS:.o=.c) $(DUMPSRCS)
HDRS= restore.h

LDFLAGS= -Bstatic
CFLAGS= -O

.KEEP_STATE:

.INIT: $(HDRS)

all: $(BINS)

$(BINS): $(OBJS) $(DUMPOBJS) doprnt.o
	$(LINK.c) -o $@ $(OBJS) $(DUMPOBJS) doprnt.o

doprnt.o:	$(DOPRNT)/doprnt.c
	$(CC) $(CFLAGS) -c -DFLOAT=0 -I$(DOPRNT) $(DOPRNT)/doprnt.c

$(DUMPOBJS): $(DUMPSRCS)
	$(COMPILE.c) -o $@ $(DUMPSRCS)

install: $(BINS)
	install -d $(DESTDIR)/usr/etc
	install -s -m 4755 $(BINS) $(DESTDIR)/usr/etc
	-rm -f $(DESTDIR)/usr/etc/rrestore
	ln -s restore $(DESTDIR)/usr/etc/rrestore

clean:
	$(RM) $(BINS) $(OBJS) $(DUMPOBJS)

lint: $(SRCS)
	$(LINT.c) $(SRCS)

print:
	@pr /usr/include/protocols/dumprestore.h restore.h $(SRCS) $(DUMPSRCS)
