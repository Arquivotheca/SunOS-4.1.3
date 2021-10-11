#
# @(#)Makefile 1.1 92/07/30 SMI
#

# Files which should exist but be empty in the destination
#
EMPTY=	wtmp messages lastlog msgbuf usracct lpd-errs

all:

install:
	install -d -o bin -m 755 ${DESTDIR}/var/adm
	for i in ${EMPTY}; do (touch ${DESTDIR}/var/adm/$$i); done
	-rm -f ${DESTDIR}/usr/adm
	-rm -f ${DESTDIR}/etc/adm
	-ln -s ../var/adm ${DESTDIR}/usr/adm
	-ln -s ../var/adm ${DESTDIR}/etc/adm

clean:
