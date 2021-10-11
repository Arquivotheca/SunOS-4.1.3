#
# @(#)Makefile 1.1 92/07/30 SMI
#
DESTDIR=
ALL=	vfont font fontinfo
CHOWN=/etc/chown

.DEFAULT:
	sccs get $@

# should make font width tables in font/* from fonts in vfont/*
all: Makefile.master
	@set -x;for i in $(ALL);do (cd $$i;$(MAKE) $(MFLAGS) ); done

install: all FRC
	@set -x;for i in $(ALL);do \
		(cd $$i;$(MAKE) $(MFLAGS) install DESTDIR=$(DESTDIR)); done

clean: Makefile.master
	@set -x;for i in $(ALL);do (cd $$i;$(MAKE) $(MFLAGS) clean ); done

FRC:
