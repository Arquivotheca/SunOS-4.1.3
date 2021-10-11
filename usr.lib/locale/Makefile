#
# @(#)Makefile 1.1 92/07/30 SMI
#

CATEGORIES = LC_CTYPE LC_NUMERIC LC_TIME LC_MONETARY LC_MESSAGES LC_COLLATE LANGINFO

all:	${CATEGORIES}

${CATEGORIES}: FRC
	cd $@; $(MAKE) ${MFLAGS} CC="${CC}"

install:
	install -d -m 755 ${DESTDIR}/usr/share
	install -d -m 755 ${DESTDIR}/usr/share/lib
	install -d -m 755 ${DESTDIR}/usr/share/lib/locale
	-for i in ${CATEGORIES}; do \
	    (cd $$i; $(MAKE) ${MFLAGS} CC=${CC} DESTDIR=${DESTDIR} install); \
	 done

clean:
	@set -x; for i in ${CATEGORIES}; do (cd $$i; $(MAKE) ${MFLAGS} clean ); done

FRC:
