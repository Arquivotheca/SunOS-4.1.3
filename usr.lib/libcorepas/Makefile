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
# @(#)Makefile @(#)Makefile 1.1 92/07/30 SMI
#
#   Build libcorepas.a -- Pascal interface routines for SunCore library

LIBS= libcorepas.a

OBJS= batchpas.o credelsegpas.o errorspas.o getvwsurfpas.o \
	init_termpas.o inprimspas.o inqinputpas.o inqprimattpas.o \
	inqsegattpas.o inqtextentpas.o inquirypas.o ndctowldpas.o \
	newframepas.o outprim2pas.o outprim3pas.o polygon2pas.o \
	polygon3pas.o rasterfiopas.o rasterprimpas.o savesegpas.o \
	segatt3pas.o segdefspas.o segmentspas.o setprimattpas.o \
	setsegattpas.o textmarkpas.o view_surfpas.o view_transpas.o \
	xformattpas.o zbufferpas.o

#LIBDIR= $(DESTDIR)/usr/lib
LIBDIR= $(DESTDIR)/usr/old

LOCAL_HDRS= pasarray.h f77strings.h

HDRS= typedefspas.h usercorepas.h sunpas.h devincpas.h
#HDRDIR= $(DESTDIR)/usr/include/pascal
HDRDIR= $(DESTDIR)/usr/old

CFLAGS= -O
PFLAGS= -c -w -g

.KEEP_STATE:

.INIT: $(LOCAL_HDRS)

all: $(LIBS)

$(LIBS): $(OBJS)
	ar rc $(LIBS) $?
	ranlib $(LIBS)

install: $(LIBS) install_h
	install -d $(LIBDIR)
	install -m 644 $(LIBS) $(LIBDIR)
	ranlib $(LIBDIR)/$(LIBS)

install_h: $(HDRS)
	install -o bin -m 755 -d $(HDRDIR)
	install -m 444 $(HDRS) $(HDRDIR)

clean:
	$(RM) $(LIBS) $(OBJS)
