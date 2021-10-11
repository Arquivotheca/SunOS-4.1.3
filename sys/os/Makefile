#
# @(#)Makefile 1.1 92/07/30 SMI
#
CHOWN=/etc/chown

HDIR=$(DESTDIR)/usr/include/os

HFILES= dlyprf.h

all:

install_h: $(HFILES) FRC
	install -d -m 755 $(HDIR)
	install -m 444 $(HFILES) $(HDIR)
FRC:
