#
# @(#)Makefile 1.1 92/07/30 SMI
#
HDIR	=	$(DESTDIR)/usr/include/scsi/impl
CSRC	=	
HFILES	=	commands.h inquiry.h mode.h pkt_wrapper.h sense.h \
		services.h status.h transport.h types.h uscsi.h


SUBDIR	=

all:	$(HFILES)

install:

install_h: all
	rm -rf $(HDIR)
	install -d -m 755 $(HDIR)
	install -m 444 $(HFILES) $(HDIR)

FRC:
