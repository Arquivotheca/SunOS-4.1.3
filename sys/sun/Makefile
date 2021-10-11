#
# @(#)Makefile 1.1 92/07/30 SMI
#
HFILES=	audioio.h autoconf.h consdev.h dbriio.h dkio.h dklabel.h fault.h  \
	fbio.h gpio.h isdnio.h mem.h openprom.h seg_kmem.h ndio.h sqz.h  \
	tvio.h vddrv.h

HDIR=$(DESTDIR)/usr/include/sun

install_h: $(HFILES)
	@set -x; for i in $(HFILES); do \
		install -c -m 444 $$i $(HDIR)/$$i ; \
	done;
