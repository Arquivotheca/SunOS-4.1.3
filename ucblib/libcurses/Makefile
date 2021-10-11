#
# @(#)Makefile 1.1 92/07/30 SMI; from UCB 5.1 85/06/07
#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
# curses package
#

LIBS= $(LIB) $(LIB_P)
LIB  = libcurses.a
LIB_P= libcurses_p.a

LIBOBJS= addch.o addstr.o box.o clear.o clrtobot.o clrtoeol.o \
	cr_put.o cr_tty.o curses.o delch.o deleteln.o delwin.o \
	endwin.o erase.o fullname.o getch.o getstr.o id_subwins.o \
	idlok.o initscr.o insch.o insertln.o longname.o move.o \
	mvprintw.o mvscanw.o mvwin.o newwin.o overlay.o overwrite.o \
	printw.o putchar.o refresh.o scanw.o scroll.o standout.o \
	toucholap.o touchwin.o tstp.o unctrl.o

SRCS= $(LIBOBJS:.o=.c)
LOCAL_HDRS= curses.ext curses.h
LIBDIR= $(DESTDIR)/usr/lib

CFLAGS=	-O
LDFLAGS= -x -r
LINTFLAGS= -hxb

$(LIB)   := OBJS= $(LIBOBJS)
$(LIB_P) := OBJS= $(LIBOBJS:%=profiled/%)
$(LIB_P) := CFLAGS += -pg

.KEEP_STATE:

.INIT: $(LOCAL_HDRS)

all: $(LIBS)

$(LIB_P): profiled

profiled:
	install -d $@

$(LIBS): $$(OBJS) 
	ar cr $@ $?
	ranlib $@

%.o profiled/%.o: %.c
	$(COMPILE.c) -o temp.o $<
	$(LD) $(LDFLAGS) -o $@ temp.o
	$(RM) temp.o

install: all install_h
	install -d -m 755 $(LIBDIR)
	install -m 644 $(LIBS) $(LIBDIR)
	ranlib $(LIBDIR)/$(LIB) $(LIBDIR)/$(LIB_P)

install_h: curses.h
	install -m 444 curses.h $(DESTDIR)/usr/include

clean:
	$(RM) $(LIBS) $(LIBOBJS) $(LIBOBJS:%=profiled/%)

tags: $(SRCS)
	cwd=`pwd`; \
	for i in $(SRCS); do \
		ctags -a -f tags $$cwd/$$i; \
	done

ctags: $(SRCS) $(LOCAL_HDRS)
	ctags $(SRCS) $(LOCAL_HDRS)

lint: $(SRCS)
	$(LINT.c) $(SRCS) -lcurses > lint.out

test: test.o $(LIB)
	$(LINK.c) -o $@ test.o $(LIB) -ltermlib

ar:
	ar crv curses.ar $(SRCS) curses.h curses.ext Makefile
