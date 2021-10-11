#	Copyright (c) 1984 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#
# @(#)Makefile 1.1 92/07/30 SMI; from S5R3 1.17
#
# Makefile for libsvidm
#
# Note that the object list given in FILES is in the order in which the files
# should be added to the archive when it is created.  If the files are not
# ordered correctly, linking of libm fails on the pdp11 and is slowed down
# on other machines.

DESTDIR=
SRCDIR=/usr/src
USRLIB= $(DESTDIR)/usr/5lib
CPPFLAGS=
COPTS= -O
CFLAGS= $(CPPFLAGS) $(COPTS)
INSTALL = install
VARIANT = svid
LIBNAME = lib$(VARIANT)m.a
LIBPNAME = lib$(VARIANT)m_p.a
AR= ar
FILES =\
	$(LIBNAME)(asin.o)\
	$(LIBNAME)(atan.o)\
	$(LIBNAME)(erf.o)\
	$(LIBNAME)(fabs.o)\
	$(LIBNAME)(floor.o)\
	$(LIBNAME)(fmod.o)\
	$(LIBNAME)(gamma.o)\
	$(LIBNAME)(hypot.o)\
	$(LIBNAME)(infinity.o)\
	$(LIBNAME)(jn.o)\
	$(LIBNAME)(j0.o)\
	$(LIBNAME)(j1.o)\
	$(LIBNAME)(pow.o)\
	$(LIBNAME)(log.o)\
	$(LIBNAME)(sin.o)\
	$(LIBNAME)(sinh.o)\
	$(LIBNAME)(sqrt.o)\
	$(LIBNAME)(tan.o)\
	$(LIBNAME)(tanh.o)\
	$(LIBNAME)(exp.o)\
	$(LIBNAME)(matherr.o)

all: $(LIBNAME) profiledlib

$(LIBNAME): $(FILES)
	$(CC) -c $(CFLAGS) $(?:.o=.c) ;
	$(AR) rv $@ $?
	ranlib $@
	rm -f $?

$(FILES): $(SRCDIR)/include/math.h $(SRCDIR)/include/values.h \
	$(SRCDIR)/include/errno.h $(SRCDIR)/sys/sys/errno.h

.c.a:;

profiledlib:
	$(MAKE) -e $(LIBPNAME) LIBNAME=$(LIBPNAME) \
				CFLAGS="-p $(CFLAGS)"

install: all
	$(INSTALL) -d  -o bin $(USRLIB)
	$(INSTALL) -m 644 -o root $(LIBNAME) $(USRLIB)/$(LIBNAME)
	ranlib $(USRLIB)/$(LIBNAME)
	$(INSTALL) -m 644 -o root $(LIBPNAME) $(USRLIB)/$(LIBPNAME)
	ranlib $(USRLIB)/$(LIBPNAME)

clean:
	-rm -f *.o

clobber:	clean
	-rm -f $(LIBNAME) $(LIBPNAME)

