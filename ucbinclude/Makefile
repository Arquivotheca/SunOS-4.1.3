#
# @(#)Makefile 1.1 92/07/30 SMI; from UCB 4.3 83/07/10
#
# Doing a make install builds /usr/ucbinclude
#
# The ``rm -rf''s used below are safe because rm doesn't
# follow symbolic links.
#
DESTDIR=
CHOWN=/usr/etc/chown
SUBDIRS=sys
#
STD=	assert.h ctype.h malloc.h pwd.h stdio.h stdlib.h time.h
LINKS=	fcntl.h
LIBDIRS=curses

all: ${STD}

install: ${STD}
	install -d -o bin -m 755 ${DESTDIR}/usr/include
	install -m 444 ${STD} ${DESTDIR}/usr/include
	for i in ${SUBDIRS}; do \
		( cd $$i; \
		$(MAKE) ${MFLAGS} install DESTDIR=$(DESTDIR) CHOWN=$(CHOWN) ); \
	done
	for i in ${LIBDIRS}; do \
		( cd ../ucblib/lib$$i; \
		$(MAKE) ${MFLAGS} install_h DESTDIR=$(DESTDIR) CHOWN=$(CHOWN));\
	done
	for i in ${LINKS}; do \
		rm -f ${DESTDIR}/usr/include/$$i; \
		ln -s sys/$$i ${DESTDIR}/usr/include/$$i; \
	done
	/bin/rm -f ${DESTDIR}/usr/ucbinclude
	/bin/ln -s ./include	${DESTDIR}/usr/ucbinclude

clean:
