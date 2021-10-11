#
# @(#)Makefile 1.1 92/07/30 SMI
#
HFILES = lnode.h loinfo.h

HDIR=$(DESTDIR)/usr/include/lofs

install_h: $(HFILES)
	install -d -m 755 $(HDIR)
	install -m 444 $(HFILES) $(HDIR)
