#
# @(#) Makefile 1.1 7/30/92 20:07:23  @(#)
#
SUBDIRS = fddi

all: FRC
	for i in $(SUBDIRS) ; do ( cd $$i ; $(MAKE) $(MFLAGS) all); done

clean: FRC
	for i in $(SUBDIRS) ; do ( cd $$i ; $(MAKE) $(MFLAGS) clean); done

lint: FRC
	for i in $(SUBDIRS) ; do ( cd $$i ; $(MAKE) $(MFLAGS) lint); done

HDIR = $(DESTDIR)/usr/include/sunfw

install_h: FRC
	-mkdir $(HDIR)
	set -x; for i in $(SUBDIRS) ; do \
		rm -fr $(HDIR)/$$i; \
		-mkdir $(HDIR)/$$i; \
		$(CHOWN) bin $(HDIR)/$$i; \
		( cd $$i; $(MAKE) install_h CHOWN=$(CHOWN) DESTDIR=$(DESTDIR) );
	done;

install: FRC
	-mkdir $(HDIR)
	set -x; for i in $(SUBDIRS) ; do \
		(cd $$i; $(MAKE) ${MFLAGS} DESTDIR=${DESTDIR} install); done
	done;

FRC:
