#
# Copyright (c) 1986, 1987, 1988, 1989 by Sun Microsystems, Inc.
# Permission to use, copy, modify, and distribute this software for any
# purpose and without fee is hereby granted, provided that the above
# copyright notice appear in all copies and that both that copyright
# notice and this permission notice are retained, and that the name
# of Sun Microsystems, Inc., not be used in advertising or publicity
# pertaining to this software without specific, written prior permission.
# Sun Microsystems, Inc., makes no representations about the suitability
# of this software or the interface defined in this software for any
# purpose. It is provided "as is" without express or implied warranty.
#
#
# @(#)Makefile 1.1 92/07/30 SMI
#
#   Build libcore77.a -- Fortran 77 interface routines for SunCore library

LIBS= libcore77.a

OBJS= \
	batch77.o credelseg77.o errors77.o get_view_surface77.o \
	init_termin77.o inprims77.o inqinput77.o inqprimatt77.o \
	inqsegatt77.o inqtextent77.o inquiry77.o ndctowld77.o \
	newframe77.o outprim277.o outprim377.o polygon277.o \
	polygon377.o rasterfileio77.o rasterprim77.o \
	savesegment77.o segatt377.o segdefaults77.o segments77.o \
	setprimatt77.o setsegatt77.o textmark77.o view_surface77.o \
	view_trans77.o xformattrib77.o

LOCAL_HDRS= f77strings.h \
	../libI77/fio.h ../libI77/fiodefs.h ../libI77/f_errno.h
#HDRDIR= $(DESTDIR)/usr/include/f77
HDRDIR= $(DESTDIR)/usr/old
HDRS= usercore77.h

CFLAGS= -O

.KEEP_STATE:

.INIT: $(LOCAL_HDRS)

all: $(LIBS)

$(LIBS): $(OBJS)
	ar ru $@ $?
	ranlib $@

install: $(LIBS) install_h
#	install -d -m 755 $(DESTDIR)/usr/lib
#	install -m 644 $(LIBS) $(DESTDIR)/usr/lib
#	ranlib $(DESTDIR)/usr/lib/$(LIBS)
	install -d -m 755 $(DESTDIR)/usr/old
	install -m 644 $(LIBS) $(DESTDIR)/usr/old
	ranlib $(DESTDIR)/usr/old/$(LIBS)

install_h: $(HDRS)
	install -o bin -m 755 -d $(HDRDIR)
	install -m 444 $(HDRS) $(HDRDIR)

clean:
	$(RM) $(LIBS) $(OBJS)
