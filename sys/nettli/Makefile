#
#       Makefile for RFS's nettli
#
#       @(#)  Makefile 1.1 92/07/30
#
#       Copyright (c) 1987 by Sun Microsystems, Inc.
#
DESTDIR=

.DEFAULT:
	sccs get $@

HFILES = timod.h tcp_tli.h tcp_tlivar.h

HDRFILE = tihdr.h

USERFILE = tiuser.h

HDIR=$(DESTDIR)/usr/include/nettli

HDRDIR=$(DESTDIR)/usr/include/sys

USERDIR=$(DESTDIR)/usr/include

install_h: $(HFILES) $(HDRFILE) $(USERFILE) FRC
	install -d -m 755 $(HDIR)
	install -d -m 755 $(HDRDIR)
	install -m 444 $(HFILES) $(HDIR)
	install -m 444 $(HDRFILE) $(HDRDIR)
	install -m 444 $(USERFILE) $(USERDIR)
	rm -f $(HDIR)/$(HDRFILE)
	cd $(HDIR); ln -s ../sys/$(HDRFILE) $(HDRFILE)
	rm -f $(HDIR)/$(USERFILE)
	cd $(HDIR); ln -s ../$(USERFILE) $(USERFILE)

FRC:
