#
# @(#)Makefile 1.1 92/07/30 SMI
#
HFILES = tmp.h tmpnode.h tmpdir.h

HDIR=$(DESTDIR)/usr/include/tmpfs

install_h: $(HFILES)
	install -d -m 755 $(HDIR)
	install -m 444 $(HFILES) $(HDIR)
