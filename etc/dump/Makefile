#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile 1.1 92/07/30 SMI; from UCB 5.4 5/28/86
#
#	dump.h			header file
#	dumpitime.c		reads /etc/dumpdates
#	dumpmain.c		driver
#	dumpoptr.c		operator interface
#	dumptape.c		handles the mag tape and opening/closing
#	dumptraverse.c		traverses the file system
#	lftw.c			fixed version of ftw(3)
#	partial.c		partial filesystem dump support
#	unctime.c		undo ctime
#
# CPPFLAGS:
#	DEBUG			use local directory to find ddate and dumpdates
#	PARTIAL			enable partial filesystem dump support
#	TDEBUG			trace out the process forking
#
BINS= dump
PARTOBJ= partial.o lftw.o
OBJS= dumpitime.o dumpmain.o dumpoptr.o dumprmt.o dumptape.o \
	dumptraverse.o unctime.o $(PARTOBJ)
SRCS= $(OBJS:.o=.c)
HDRS= dump.h

FLAGS= -DPARTIAL
OPTIM= -O
CFLAGS= $(FLAGS) $(OPTIM)

.KEEP_STATE:

.INIT: $(HDRS)

all: $(BINS)

$(BINS): $(OBJS)
	$(LINK.c) -o $@ $(OBJS)

install: $(BINS)
	install -d $(DESTDIR)/usr/etc
	install -s -m 6755 -g tty $(BINS) $(DESTDIR)/usr/etc
	$(RM) $(DESTDIR)/usr/etc/rdump
	ln -s dump $(DESTDIR)/usr/etc/rdump
	install -d $(DESTDIR)/etc
	install -m 664 -g operator /dev/null $(DESTDIR)/etc/dumpdates

clean:
	$(RM) $(BINS) $(OBJS)

lint: $(SRCS)
	$(LINT.c) $(SRCS)

print: $(HDRS) $(SRCS)
	pr /usr/include/protocols/dumprestore.h $(HDRS) $(SRCS) | lpr
