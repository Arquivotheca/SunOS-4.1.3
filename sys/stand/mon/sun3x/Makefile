#
# @(#)Makefile 1.1 92/07/30 SMI
#

all:

HFILES=	keyboard.h sunmon.h
KFILES= cpu.map.h cpu.addrs.h

install_h: $(HFILES) $(KFILES)
	@set -x; for i in $(HFILES) ; do \
		install -c -m 444 $$i $(DESTDIR)/usr/include/sun3; done
