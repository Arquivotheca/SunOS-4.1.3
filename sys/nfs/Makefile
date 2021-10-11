#
# @(#)Makefile 1.1 92/07/30 SMI
#
HFILES = export.h nfs.h nfs_clnt.h rnode.h 

HDIR=$(DESTDIR)/usr/include/nfs

install_h: $(HFILES) FRC
	install -d -m 755 $(HDIR)
	install -m 444 $(HFILES) $(HDIR)

FRC:
