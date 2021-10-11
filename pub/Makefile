#
# @(#)Makefile 1.1 92/07/30 SMI
#

.DEFAULT:
	sccs get -G$@ $@

DESTDIR=

# Files which live in the current directory and are copied to the destination.
#
FILES=	ascii eqnchar greek kbd marg8 tabclr tabs tabs4

all:	${FILES}

install:
	-mkdir -p ${DESTDIR}/usr/share/lib/pub && \
		chown bin ${DESTDIR}/usr/share/lib/pub && \
		chmod 755 ${DESTDIR}/usr/share/lib/pub
	for i in ${FILES}; do \
		(install -c -m 644 $$i ${DESTDIR}/usr/share/lib/pub/$$i); done
	-rm -rf ${DESTDIR}/usr/pub
	ln -s share/lib/pub ${DESTDIR}/usr/pub

clean:

FRC:
