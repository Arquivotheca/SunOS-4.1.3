#
#	@(#)Makefile 1.1 92/07/30 SMI;
#
# Doing a make install builds /usr/xpg2include
#
# The ``rm -rf''s used below are safe because rm doesn't
# follow symbolic links.
#
DESTDIR=
SUBDIRS=sys
STD=dirent.h limits.h math.h stdio.h utmp.h gettext.h nl_types.h malloc.h
BSDLINK=grp.h pwd.h
LINKS=	dirent.h
CHOWN=	/etc/chown

all: $(SUBDIRS) ${STD} $(LINKS)

$(SUBDIRS): FRC
	@set -x;for i in $(SUBDIRS); do ( cd $$i; $(MAKE) ); done

$(LINKS): $(SUBDIRS)
	rm -f $@;
	ln -s sys/$@ $@

FRC:

install: all
	install -d -o bin -m 755 ${DESTDIR}/usr/xpg2include
	rm -rf ${DESTDIR}/usr/xpg2include/sys
	install -d -o bin -m 755 ${DESTDIR}/usr/xpg2include/sys
	@set -x;(cd sys; for j in *.h; do \
		install -c -m 444 $$j ${DESTDIR}/usr/xpg2include/sys/$$j; \
	done)
	-for i in ${STD}; do \
		install -c -m 444 $$i ${DESTDIR}/usr/xpg2include/$$i; \
	done
	-for i in ${LINKS}; do \
		rm -f ${DESTDIR}/usr/xpg2include/$$i; \
		ln -s sys/$$i ${DESTDIR}/usr/xpg2include/$$i; \
	done
	-for i in ${BSDLINK}; do \
		rm -f ${DESTDIR}/usr/xpg2include/$$i; \
		ln -s ../include/$$i ${DESTDIR}/usr/xpg2include/$$i; \
	done

clean:
