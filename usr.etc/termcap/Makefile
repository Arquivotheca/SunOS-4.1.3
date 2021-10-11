#
# @(#)Makefile 1.1 92/07/30 SMI; from UCB 1.10 07/01/83
#
# reorder gives an editor command for most common terminals
# (in reverse order from n'th to 1'st most commonly used)
# to move them to the front of termcap
#
DESTDIR =

.DEFAULT:
	sccs get -G$@ $@

termcap: reorder termcap.src
	ex - termcap.src < reorder

install: termcap
	install -d -m 755 ${DESTDIR}/usr/share
	install -d -m 755 ${DESTDIR}/usr/share/lib
	install -m 664 termcap ${DESTDIR}/usr/share/lib/termcap
	-rm -rf ${DESTDIR}/etc/termcap
	-rm -rf ${DESTDIR}/usr/etc/termcap
	ln -s ../usr/share/lib/termcap ${DESTDIR}/etc/termcap
	ln -s ../share/lib/termcap ${DESTDIR}/usr/etc/termcap
	rm -fr ${DESTDIR}/usr/share/lib/tabset
	install -d -o bin -m 755 ${DESTDIR}/usr/share/lib/tabset
	cd tabset; $(MAKE) install DESTDIR=$(DESTDIR)
	-rm -fr ${DESTDIR}/usr/lib/tabset
	ln -s ../share/lib/tabset ${DESTDIR}/usr/lib/tabset

clean:
	rm -f termcap
