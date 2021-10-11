#	@(#)Makefile 1.1 92/07/30 SMI
HFILES = dumprestore.h routed.h rwhod.h timed.h

CHOWN=/etc/chown

HDIR=$(DESTDIR)/usr/include/protocols

all:

install: $(HFILES)
	install -d -o bin -m 755 $(HDIR)
	install -m 444 $(HFILES) $(HDIR)
