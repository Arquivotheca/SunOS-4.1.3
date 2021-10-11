#
# @(#)Makefile 1.1 92/07/30 SMI
#
DESTDIR=

.DEFAULT:
	sccs get -G$@ $@

# Files which live in the current directory and are copied to the destination.
#
FILES=	ms.acc ms.cov ms.eqn ms.ref ms.tbl ms.ths ms.toc

all:	${FILES}

install:
	install -d -m 755 ${DESTDIR}/usr/share
	install -d -m 755 ${DESTDIR}/usr/share/lib
	install -d -o bin -m 755 ${DESTDIR}/usr/share/lib/ms
	rm -rf ${DESTDIR}/usr/lib/ms
	ln -s ../share/lib/ms ${DESTDIR}/usr/lib/ms
	for i in ${FILES}; do \
		(install -c -m 644 $$i ${DESTDIR}/usr/share/lib/ms/$$i); done

clean:

FRC:
