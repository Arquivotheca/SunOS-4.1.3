#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
# @(#)Makefile 1.1 92/07/30 SMI; from UCB 6.1 5/21/86
#
#  Makefile for -me macros
#
#	This makefile strips comments from the source files and
#	installs them in the expected directories.
#
DESTDIR=
LIB =	/usr/share/lib/me

SRCS =	tmac.e acm.me chars.me deltext.me eqn.me \
	float.me footnote.me index.me local.me null.me \
	refer.me sh.me tbl.me thesis.me
GET =	sccs get

.DEFAULT:
	sccs get -G$@ $@

all: revisions $(SRCS)

install: revisions $(SRCS)
	install -d -m 755 ${DESTDIR}/usr/share
	install -d -m 755 ${DESTDIR}/usr/share/lib
	if [ ! -d ${DESTDIR}${LIB} ]; then \
		rm -f ${DESTDIR}${LIB}; \
		install -d -m 755 ${DESTDIR}${LIB}; \
	fi;
	rm -rf ${DESTDIR}/usr/lib/me
	ln -s ../share/lib/me ${DESTDIR}/usr/lib/me
	


	for i in ${SRCS}; do \
		sed -e '/%beginstrip%/{' \
		    -e 'h' \
		    -e 's/.*/.\\" This version has had comments stripped./p' \
		    -e 'g' \
		    -e '}' \
		    -e '/%beginstrip%/,$$s/[. 	][ 	]*\\".*//' \
		    -e '/^$$/d' \
		    -e '/\\n@/d' $$i > _mac_tmp_; \
		install -m 644 _mac_tmp_ ${DESTDIR}${LIB}/$$i; \
		rm _mac_tmp_; \
	done
	install -d -m 755 ${DESTDIR}/usr/share/lib/tmac
	mv ${DESTDIR}${LIB}/tmac.e ${DESTDIR}/usr/share/lib/tmac/tmac.e
	install -c -m 644 revisions ${DESTDIR}${LIB}/revisions

clean:
	rm -f _mac_tmp_

FRC:

