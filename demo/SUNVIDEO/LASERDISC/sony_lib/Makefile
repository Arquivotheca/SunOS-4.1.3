#
# @(#)Makefile 1.1 92/07/30 SMI
#
# Copyright (c) 1989 by Sun Microsystems, Inc.

BASEDIR=	..

CFLAGS=-O -I$(BASEDIR)
LIBOBJS=sony_sbyte.o sony_hshake.o sony_line.o sony_search.o sony_repeat.o \
	sony_motor.o sony_eject.o sony_chinq.o sony_romvers.o \
	sony_step.o sony_enter.o sony_trkjmp.o sony_statinq.o \
	sony_ucodeinq.o sony_addrinq.o sony_m_search.o sony_mark.o \
	sony_replay.o sony_repvar.o sony_clvdisk.o sony_itoa.o

LIBSRC=sony_sbyte.c sony_hshake.c sony_line.c sony_search.c sony_repeat.c \
	sony_motor.c sony_eject.c sony_chinq.c sony_romvers.c \
	sony_step.c sony_enter.c sony_trkjmp.c sony_statinq.c \
	sony_ucodeinq.c sony_addrinq.c sony_m_search.c sony_mark.c \
	sony_replay.c sony_repvar.c sony_clvdisk.c sony_itoa.c

LIBSONY=	$(BASEDIR)/sonylaslib.a
DEST_LIB=	$(DESTDIR)/usr/demo/SUNVIDEO/LASERDISC
DEST_SRC=	$(DESTDIR)/usr/demo/SUNVIDEO/LASERDISC/sony_lib

all:	lib

lib : ${LIBOBJS} 
	rm -f $(BASEDIR)/sonylaslib.a
	ar r $(BASEDIR)/sonylaslib.a `lorder ${LIBOBJS} | tsort`
	ranlib $(BASEDIR)/sonylaslib.a

clean :
	rm -f $(BASEDIR)/sonylaslib.a *.o core


install: $(LIBSONY)
	install -d -m 755 $(DEST_SRC); \
	install -m 444 $(LIBSRC) $(DEST_SRC); \
	install -m 444 Makefile $(DEST_SRC)/Makefile; \
	install -d -m 755 $(DEST_LIB); \
	install -m 644 $(LIBSONY) $(DEST_LIB)
#
