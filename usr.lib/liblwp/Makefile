#
#	Copyright (C) 1987 Sun Microsystems, Inc.
#       @(#) Makefile 1.1 92/07/30 Copyr 1987 Sun Micro

MAKE= /bin/make
MKDIR= /bin/mkdir
INSTALL= /usr/bin/install
RM= /bin/rm
CHMOD= /bin/chmod
CHOWN= /usr/etc/chown
d-mc68020=m68k
d-mc68010=m68k
d-sparc=sparc
MACH= $(d$(TARGET_MACH))

all: FRC
	cd $(MACH)/user; $(MAKE) liblwp.a
	cd nbio/$(MACH); $(MAKE) libnbio.a

install: install_h FRC
	cd $(MACH)/user; $(MAKE) install
	cd nbio/$(MACH); $(MAKE) install

install_h: mch.h stk.h lwp.h chk.h lwperror.h FRC
	test -d lwp || ln -s . lwp
	test -d $(DESTDIR)/usr/include/lwp || $(MKDIR) $(DESTDIR)/usr/include/lwp
	-$(CHOWN) bin $(DESTDIR)/usr/include/lwp
	-$(CHMOD) 755 $(DESTDIR)/usr/include/lwp
	$(INSTALL) -c -m 444 lwp.h ${DESTDIR}/usr/include/lwp
	$(INSTALL) -c -m 444 lwperror.h $(DESTDIR)/usr/include/lwp
	$(INSTALL) -c -m 444 stk.h $(DESTDIR)/usr/include/lwp/stackdep.h
	$(INSTALL) -c -m 444 mch.h $(DESTDIR)/usr/include/lwp/lwpmachdep.h
	$(INSTALL) -c -m 444 chk.h $(DESTDIR)/usr/include/lwp/check.h
	cd $(MACH)/user; $(MAKE) install_h

FRC:

clean:
	cd $(MACH)/user; $(MAKE) clean
	cd nbio/$(MACH); $(MAKE) clean
