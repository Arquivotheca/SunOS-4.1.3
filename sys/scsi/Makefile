#
# @(#)Makefile 1.1 92/07/30 SMI
#
HDIR	=	$(DESTDIR)/usr/include/scsi
CSRC	=	
HFILES	=	scsi.h scsi_address.h scsi_ctl.h scsi_params.h scsi_pkt.h \
		scsi_resource.h scsi_types.h
SUBDIR	=	adapters conf generic impl targets

all:	$(HFILES)

install:

install_h: all
	rm -rf $(HDIR)
	install -d -m 755 $(HDIR)
	install -m 444 $(HFILES) $(HDIR)
	@set -x; for i in $(SUBDIR) ; do \
		( cd $$i; $(MAKE) install_h \
			CHOWN=$(CHOWN) DESTDIR=$(DESTDIR) ) ; \
	done

FRC:
