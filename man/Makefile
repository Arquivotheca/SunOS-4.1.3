#
# @(#)Makefile 1.41 89/10/05 SMI;
#

.INIT:
	-@( sccs get -s Makefile Makefile.master \
		Printfile Printfile.master Prepfile Interim > /dev/null 2>&1 )

DESTDIR=
DIRS=	man1 man2 man3 man4 man5 man6 man7 man8 manl mann
ALL=	man1 man2 man3 man4 man5 man6 man7 man8

CHOWN=	/etc/chown

all: Makefile.master FRC
	@set -x; for i in $(ALL); do ( cd $$i; $(MAKE) ${MFLAGS}); done

extract: Makefile.master FRC
	@set -x; for i in $(ALL); do ( cd $$i; $(MAKE) ${MFLAGS} $@); done

install: all
	install -d -o bin -m 755 ${DESTDIR}/usr/share
	install -d -o bin -m 755 ${DESTDIR}/usr/share/man
	rm -rf ${DESTDIR}/usr/man
	ln -s share/man ${DESTDIR}/usr/man
	@set -x;for i in ${DIRS}; do \
		install -d -o bin -m 755 ${DESTDIR}/usr/share/man/$$i; \
		done
	@set -x;for i in $(ALL); do \
		( cd $$i; $(MAKE) ${MFLAGS} install DESTDIR=$(DESTDIR)); \
		done

px showprint check: Makefile.master
	@set -x;for i in $(ALL); do ( cd $$i; $(MAKE) ${MFLAGS} $@ ); done

clean: Makefile.master
	-@set -x;for i in $(ALL); do ( cd $$i; $(MAKE) ${MFLAGS} $@ ); done
#	-rm -f badpaths badpgrefs titles no.ix badnames badhf spell badtroff

FRC:

prep: FRC
	-$(MAKE) -f Prepfile

update clone pinstall:
	-$(MAKE) -f Printfile $@
