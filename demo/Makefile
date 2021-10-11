# @(#)Makefile 1.1 92/07/30 SMI

DESTDIR= 
INSDIR= $(DESTDIR)/usr/demo
SUBDIR= SUNVIEW BUTTONBOX CDROM DIALBOX SOUND XGL SUNVIDEO
OWNER= bin
INSTALL= install -o $(OWNER)
OPENWINHOME= /usr/openwin

all: Makefile subdirs

subdirs: FRC
	for i in $(SUBDIR); do \
		(cd $$i && $(MAKE) $(MFLAGS)); \
	done

clean: FRC
	for i in $(SUBDIR); do \
		(cd $$i && $(MAKE) $(MFLAGS) $@); \
	done

install: Makefile Makefile.install README FRC
	$(INSTALL) -d $(INSDIR)
	$(INSTALL) -m 644 Makefile.install $(INSDIR)
	mv $(INSDIR)/Makefile.install $(INSDIR)/Makefile
	$(INSTALL) -m 444 README $(INSDIR)
	for i in $(SUBDIR); do \
		(cd $$i && $(MAKE) $(MFLAGS) $@ \
			DESTDIR=$(DESTDIR) OWNER=$(OWNER)); \
	done

FRC:
