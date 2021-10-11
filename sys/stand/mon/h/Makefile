#
# @(#) Makefile 1.1@(#)
#
HFILES= bootparam.h eeprom.h idprom.h keyboard.h led.h obpdefs.h openprom.h \
	password.h sunmon.h sunromvec.h
KFILES= cpu.map.h cpu.addrs.h
KEYBRDFILE= keyboard.h

CHOWN=/etc/chown

ARCH:sh= arch -k
HDIR=$(DESTDIR)/usr/include/mon
ARCHDIRS= $(DESTDIR)/usr/include/sun4 $(DESTDIR)/usr/include/sun4c\
		$(DESTDIR)/usr/include/sun4m

all:
	@echo make install_h only

install_h: $(HFILES) $(KFILES) $(KEYBRDFILE)
	install -d -o bin -m 755 $(HDIR)
	for i in $(HFILES); do \
		install -c -m 444 $$i $(HDIR)/$$i ; \
	done
	for i in ${ARCHDIRS}; do \
		install -d -o bin -m 755 $$i; \
		install -c -m 444 $(KEYBRDFILE) $$i/$(KEYBRDFILE); \
	done
