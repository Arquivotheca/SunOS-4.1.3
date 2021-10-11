#
# @(#)Makefile 1.1 92/07/30 Copyr 1985-9 Sun Micro
#
# Copyright (c) 1985, 1986, 1987, 1988, 1989 by Sun Microsystems, Inc. 
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
#	Build SunCGI Fortran library wrapper

LIB= libcgi77.a

OBJS= bundle77.o cf77.o cgiwindow77.o char77.o charatt77.o \
	circarc77.o circle77.o devcont77.o elliparcnew77.o \
	ellipse77.o input277.o input77.o inputset77.o \
	inqatt77.o inqinput77.o lookup77.o mark77.o \
	metafile77.o metainit77.o metanew77.o negotiate77.o \
	patsubs77.o pixels277.o pixels77.o polyatt77.o \
	polygon77.o polyline77.o

LOCAL_HDRS= cf77.h cgidefs77.h
#HDRDIR= $(DESTDIR)/usr/include/f77
HDRDIR= $(DESTDIR)/usr/old
HDRS= cgidefs77.h

CFLAGS = -O -fsingle
CPPFLAGS= -I../libcgi

AR=/usr/bin/ar
RANLIB=/usr/bin/ranlib
F77=/usr/bin/f77
RM=/usr/bin/rm
CC=/usr/bin/cc
INSTALL=/usr/bin/install

.KEEP_STATE:

all: $(LOCAL_HDRS) $(LIB)

$(LIB): $(OBJS)
	$(AR) ru $@ $?
	$(RANLIB) $@

install: $(LIB)
#	$(INSTALL) -d $(DESTDIR)/usr/lib
#	$(INSTALL) -m 644 $(LIB) $(DESTDIR)/usr/lib
#	$(RANLIB) $(DESTDIR)/usr/lib/$(LIB)
	$(INSTALL) -d -m 755 $(DESTDIR)/usr/old
	$(INSTALL) -m 644 $(LIB) $(DESTDIR)/usr/old
	$(RANLIB) $(DESTDIR)/usr/old/$(LIB)

install_h: $(HDRS)
	$(INSTALL) -d -m 755 $(HDRDIR)
	$(INSTALL) -m 444 $(HDRS) $(HDRDIR)

clean:
	-$(RM) -f $(LIB) $(OBJS) tags
