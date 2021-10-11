#
#       Makefile for High Sierra (CD-ROM) File System  sys/hsfs
#
#       @(#)  Makefile 1.1 92/07/30 
#
#       Copyright (c) 1989 by Sun Microsystems, Inc.
#
DESTDIR=

.DEFAULT:
	sccs get $@

HFILES = hsfs.h hsfs_private.h hsfs_spec.h hsfs_node.h hsfs_isospec.h \
	 hsfs_susp.h hsfs_rrip.h

HDIR=$(DESTDIR)/usr/include/hsfs

install_h: $(HFILES) FRC
	install -m 444 $(HFILES) $(HDIR)

install: $(HFILES) FRC
	install -d -m 755 $(HDIR)
	install -m 444 $(HFILES) $(HDIR)

FRC:
