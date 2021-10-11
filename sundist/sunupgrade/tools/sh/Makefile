# @(#)Makefile 1.22 89/10/05 SMI; from S5R2 1.8
#
# Makefile for sh for upgrade toolkit

.KEEP_STATE:

BINS = sh
BINDIR = $(DESTDIR)/usr/bin
DOPRNT = ../../../../lib/libc/stdio/common
LINKDIR = ../bin

LOCAL_HDRS = brkincr.h ctype.h defs.h hash.h mac.h mode.h name.h \
	stak.h sym.h timeout.h

HDRS=doprnt.h stdiom.h

OBJS = setbrk.o blok.o stak.o cmd.o fault.o main.o word.o string.o\
	name.o args.o xec.o service.o error.o io.o print.o macro.o expand.o\
	ctype.o msg.o test.o defs.o echo.o hash.o hashserv.o pwd.o func.o\
	doprnt.o

CFILES= setbrk.c blok.c stak.c cmd.c fault.c main.c word.c string.c\
	name.c args.c xec.c service.c error.c io.c print.c macro.c expand.c\
	ctype.c msg.c test.c defs.c echo.c hash.c hashserv.c pwd.c func.c

CLEANFILES= args.c args.o blok.c blok.o brkincr.h cmd.c cmd.o ctype.c \
	ctype.h ctype.o defs.c defs.h defs.o doprnt.c doprnt.h  doprnt.o \
	echo.c echo.o error.c error.o expand.c expand.o fault.c fault.o \
	func.c func.o hash.c hash.h hash.o hashserv.c hashserv.o io.c \
	io.o mac.h macro.c macro.o main.c main.o mode.h msg.c msg.o \
	name.c name.h name.o print.c print.o pwd.c pwd.o service.c \
	service.o setbrk.c setbrk.o sh stak.c stak.h stak.o stdiom.h \
	string.c string.o sym.h test.c test.o timeout.h word.c word.o \
	xec.c xec.o

CFLAGS = -O -DRES
LDFLAGS = -Bstatic

ctype.o := CFLAGS += -R

# .INIT:  $(HDRS) $(LOCAL_HDRS)

all:	${HDRS} ${LOCAL_HDRS} bins

bins: $(BINS)

$(BINS): $$(OBJS)
	$(LINK.c) $(LDFLAGS)  -o $@ $(OBJS) $(LINK_LIBS)

doprnt.o:	doprnt.c
	$(CC) $(CFLAGS) -c -I. -DFLOAT=0 doprnt.c


${LOCAL_HDRS}:
	cp ../../../../bin/sh/$@ .

${HDRS}:
	cp ${DOPRNT}/$@ .

${CFILES}:
	cp ../../../../bin/sh/$@ .

doprnt.c:
	cp $(DOPRNT)/doprnt.c .

install:
		install -s -m 755 sh ${DESTDIR}/toolkit/sh

clean:
	for i in ${CLEANFILES}; do \
		rm -f $$i; done


