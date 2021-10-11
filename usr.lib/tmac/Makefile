#
# @(#)Makefile 1.1 92/07/30 SMI
#
DESTDIR=

.DEFAULT:
	sccs get -G$@ $@

# Files which live in the current directory and are copied to the destination.
#
FILES=	tmac.a tmac.an tmac.cp tmac.e tmac.imagen tmac.os tmac.r \
	tmac.s tmac.scover tmac.sdisp tmac.skeep tmac.srefs tmac.vcat \
	tmac.indent

all:	${FILES}

install:
	install -d -m 755 ${DESTDIR}/usr/share
	install -d -m 755 ${DESTDIR}/usr/share/lib
	install -d -m 755 ${DESTDIR}/usr/share/lib/tmac
	rm -rf ${DESTDIR}/usr/lib/tmac
	ln -s ../share/lib/tmac ${DESTDIR}/usr/lib/tmac
	for i in ${FILES}; do \
		(install -c -m 644 $$i ${DESTDIR}/usr/share/lib/tmac/$$i); done

clean:

FRC:
