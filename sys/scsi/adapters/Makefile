#
# @(#)Makefile 1.1 92/07/30 SMI
#
HDIR	=	$(DESTDIR)/usr/include/scsi/adapters
CSRC	=	
HFILES	=	espreg.h screg.h espvar.h

all:	$(HFILES)

install:

install_h: all
	rm -rf $(HDIR)
	install -d -m 755 $(HDIR)
	install -m 444 $(HFILES) $(HDIR)

FRC:
