#
# @(#)Makefile 1.1 92/07/30 SMI
#
HFILES = if_ecreg.h if_en.h if_enreg.h if_iereg.h if_ievar.h \
	if_lereg.h if_levar.h if_llc.h if_mie.h if_obie.h if_tie.h \
	llcp.h

HDIR=$(DESTDIR)/usr/include/sunif

install_h: $(HFILES)
	install -d -m 755 $(HDIR)
	install -m 444 $(HFILES) $(HDIR)
