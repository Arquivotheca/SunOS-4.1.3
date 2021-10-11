#
# @(#)Makefile 1.1 92/07/30 SMI
#
HFILES = wintree.h

HDIR=$(DESTDIR)/usr/include/sunwindowdev

install_h: $(HFILES)
	@set -x; for i in $(HFILES); do \
		install -c -m 444 $$i $(HDIR)/$$i ; \
	done;
