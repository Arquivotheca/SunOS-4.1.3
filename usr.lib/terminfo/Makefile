#	Copyright (c) 1984 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#
# @(#)Makefile 1.1 92/07/30 SMI; from S5R3.1 1.5
#
#	terminfo makefile
#

TERMDIR=$(DESTDIR)/usr/share/lib/terminfo
LIBDIR=$(DESTDIR)/usr/share/lib
PARTS=	header *.ti trailer
COMPILE=../libcurses/screen/tic -v

all:
	echo "Nothing made by default; only "make install" does something."

install:	ckdir terminfo.src ckout
	TERMINFO=$(TERMDIR) 2>&1 $(COMPILE) terminfo.src > errs
	$(RM) -r $(DESTDIR)/usr/lib/terminfo
	ln -s ../share/lib/terminfo $(DESTDIR)/usr/lib/terminfo
	@chgrp -R staff $(DESTDIR)/usr/share/lib/terminfo
	@touch install
	@echo
	@sh ./ckout

terminfo.src:	$(PARTS)
	@cat $(PARTS) > terminfo.src

clean:
	rm -f terminfo.src install errs nohup.out

ckdir:
	install -d -m 755 $(DESTDIR)/usr/share
	install -d -m 755 $(LIBDIR)
	@echo "terminfo database will be built in $(TERMDIR)."
	@echo "checking for the existence of $(TERMDIR):"
	@echo
	@if [ -d $(TERMDIR) ]; \
	then \
		echo "	$(TERMDIR) exists"; \
	else  \
		echo "	building $(TERMDIR)"; \
		install -d -m 755 $(TERMDIR); \
	fi
	@echo
	@echo
	@echo
	@echo It will take quite some time to generate $(TERMDIR)/*/*.
	@echo
