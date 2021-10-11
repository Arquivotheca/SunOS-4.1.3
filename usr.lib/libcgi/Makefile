#
# @(#)Makefile @(#)Makefile 1.1 92/07/30 SMI
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
#	Build SunCGI graphics library

LIB= libcgi.a

OBJS= bundle.o cgipw.o cgisigi.o cgisigw.o cgiwindow.o char.o \
	charatt.o charfont1.o charfont2.o charsubs.o circarc.o \
	circarc2.o circarc3.o circarcpat.o circle.o \
	circsubs.o circtext2.o devcont.o elliparcnew.o ellipse.o \
	errlst.o gp1_attr.o gp1_cgi_control.o gp1_clip.o gp1_prims.o \
	input.o input2.o inputset.o inputsubs.o inqatt.o inqinput.o \
	lookup.o mark.o metafile.o metainit.o metanew.o metasubs.o \
	negotiate.o patsubs.o pixels.o pixels2.o polyatt.o polygon.o \
	polygonsubs.o polyline.o polysubs.o window_dep.o

LOCAL_HDRS= cgiminicon.h cgipriv.h cgipriv_arcs.h
HDRS= cgi_gp1_pwpr.h cgicbind.h cgiconstants.h cgidefs.h cgipw.h 

ALL= $(LIB) $(LOCAL_HDRS) $(HDRS) llib-lcgi.ln install_from_old

CFLAGS= -O -fsingle

RANLIB=/usr/bin/ranlib
INSTALL=/usr/bin/install

.KEEP_STATE:

all: $(LOCAL_HDRS) $(HDRS) $(ALL)

$(LIB): $(OBJS)
	$(AR) ru $@ $?
	$(RANLIB) $@

llib-lcgi.ln: llib-lcgi
	$(LINT) -z -I. llib-lcgi=.c -Ccgi

install: install_old

install_h: install_h_old

# install in /usr/lib
install_lib: $(ALL) install_h_lib
	$(INSTALL) -d -m 755 $(DESTDIR)/usr/lib
	$(INSTALL) -m 644 libcgi.a $(DESTDIR)/usr/lib
	$(RANLIB) $(DESTDIR)/usr/lib/libcgi.a
	$(INSTALL) -o bin -m 755 -d $(DESTDIR)/usr/lib/lint
	$(INSTALL) -m 644 llib-lcgi llib-lcgi.ln $(DESTDIR)/usr/lib/lint

install_h_lib: $(HDRS)
	$(INSTALL) -d -m 755 $(DESTDIR)/usr/include
	$(INSTALL) -m 444 $(HDRS) $(DESTDIR)/usr/include

# install in /usr/old
install_old: $(ALL) install_h
	$(INSTALL) -d -m 755 $(DESTDIR)/usr/old
	$(INSTALL) -m 644 libcgi.a $(DESTDIR)/usr/old
	$(RANLIB) $(DESTDIR)/usr/old/libcgi.a
	$(INSTALL) -m 644 llib-lcgi llib-lcgi.ln $(DESTDIR)/usr/old
	$(INSTALL) -m 755 install_from_old $(DESTDIR)/usr/old

install_h_old: $(HDRS)
	$(INSTALL) -d -m 755 $(DESTDIR)/usr/old
	$(INSTALL) -m 444 $(HDRS) $(DESTDIR)/usr/old

clean: FRC
	$(RM) $(LIB) $(OBJS) llib-lcgi.ln tags

FRC:
