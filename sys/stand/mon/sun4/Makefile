#
# @(#)Makefile 1.1 92/07/30 SMI
#
FILES=

all: $(FILES)

clean:
	sccs clean

HFILES=	sunmon.h
KFILES= cpu.map.h cpu.addrs.h

install_h: $(HFILES) $(KFILES)
	@set -x; for i in $(HFILES) ; do \
		install -c -m 444 $$i $(DESTDIR)/usr/include/sun4; done
