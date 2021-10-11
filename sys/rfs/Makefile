#
#       Makefile for RFS's sys/rfs
#
#       @(#)  Makefile 1.1 92/07/30
#
#       Copyright (c) 1987 by Sun Microsystems, Inc.
#
DESTDIR=

.DEFAULT:
	sccs get $@

HFILES = adv.h cirmgr.h comm.h fumount.h hetero.h idtab.h message.h ns_xdr.h \
	 nsaddr.h nserve.h pn.h que.h rdebug.h recover.h rfs_misc.h rfs_mnt.h \
	 rfs_node.h rfs_serve.h rfs_xdr.h rfsys.h sema.h

HDIR=$(DESTDIR)/usr/include/rfs

install_h: $(HFILES) FRC
	install -m 444 $(HFILES) $(HDIR)

install: $(HFILES) FRC
	install -d -m 755 $(HDIR)
	install -m 444 $(HFILES) $(HDIR)

FRC:
