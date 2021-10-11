#
# @(#)Makefile 1.1 92/07/30 SMI; from UCB 4.3 83/07/10
#
# Doing a make install builds /usr/5include
#
# The ``rm -rf''s used below are safe because rm doesn't
# follow symbolic links.
#
DESTDIR=
SUBDIRS=sys
STD=	assert.h \
	ctype.h \
	grp.h \
	malloc.h \
	pwd.h \
	stdio.h \
	stdlib.h \
	time.h
LINKS=	fcntl.h
CHOWN=	/etc/chown

all: $(SUBDIRS) ${STD} $(LINKS)

$(SUBDIRS): FRC
	@set -x;for i in $(SUBDIRS); do ( cd $$i; $(MAKE) ); done

$(LINKS): $(SUBDIRS)
	rm -f $@;
	ln -s sys/$@ $@

FRC:

install: all
	install -d -o bin -m 755 ${DESTDIR}/usr/5include
	install -d -o bin -m 755 ${DESTDIR}/usr/5include/sys
	@set -x;(cd sys; for j in *.h; do \
		install -c -m 444 $$j ${DESTDIR}/usr/5include/sys/$$j; \
	done)
	-for i in ${STD}; do \
		install -c -m 444 $$i ${DESTDIR}/usr/5include/$$i; \
	done
	(cd ../usr.lib/libcurses; ${MAKE} DESTDIR=${DESTDIR} install_h)
	-for i in ${LINKS}; do \
		rm -f ${DESTDIR}/usr/5include/$$i; \
		ln -s sys/$$i ${DESTDIR}/usr/5include/$$i; \
	done

clean:
