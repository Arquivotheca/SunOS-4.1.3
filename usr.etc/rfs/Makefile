#
#       Makefile for RFS's utils
#
#       @(#)  Makefile 1.1 92/07/30
#
#       Copyright (c) 1987 by Sun Microsystems, Inc.
#

DESTDIR=

SRC=/usr/src/usr.etc/rfs

SUBDIR= adv dname fumount fusage fuser hostrfs idload listen mount_rfs \
	nlsadmin nserve nsquery rfadmin rfpasswd rfsetup rfstart rfstop \
	rfuadmin rfudaemon unadv startup setpgrp rmntstat

FRC=

all:	${SUBDIR}
 
${SUBDIR}: FRC
	cd $@; $(MAKE) ${MFLAGS}

install: all
	install -d -m 755 ${DESTDIR}/var/net
	install -d -m 755 ${DESTDIR}/var/net/nls
	install -d -m 755 ${DESTDIR}/var/net/servers
	@set -x;for i in ${SUBDIR}; do \
		(cd ${SRC}/$$i; make ${MFLAGS} DESTDIR=${DESTDIR} install); done
	cd ${DESTDIR}/usr; rm -rf net; ln -s /var/net .

clean:
	@set -x;for i in ${SUBDIR}; do (cd ${SRC}/$$i; make clean); done

FRC:
