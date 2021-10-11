#
# @(#)Makefile 1.1 92/07/30 SMI
#

HFILES = anon.h as.h faultcode.h hat.h mp.h page.h pvn.h rm.h \
	seg.h seg_dev.h seg_map.h seg_vn.h seg_u.h swap.h vpage.h

HDIR=$(DESTDIR)/usr/include/vm

install_h: $(HFILES) FRC
	install -d -m 755 $(HDIR)
	install -m 444 $(HFILES) $(HDIR)
FRC:
