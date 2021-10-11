#
# @(#)Makefile 1.1 92/07/30 SMI 
#

MESSAGEDIR = usr/share/lib/locale/LC_MESSAGES

all: installtxt gencat $(CTYPEFILES)
	
clean:
	echo "Nothing to clean here"

install: installtxt gencat $(CTYPEFILES)
	install -d -m 755 ${DESTDIR}/${MESSAGEDIR}
	install -d -m 755 ${DESTDIR}/${MESSAGEDIR}/C
	rm -f $(DESTDIR)/$(MESSAGEDIR)/default
	ln -s ./C $(DESTDIR)/$(MESSAGEDIR)/default
	install -s installtxt gencat $(DESTDIR)/usr/etc


HDIR = ../../../xpginclude
HFILES = $(HDIR)/gettext.h $(HDIR)/nl_types.h
CTYPEDIR = usr/share/lib/locale/LC_CTYPE

CFLAGS = -O -I$(HDIR)

.INIT: $(HFILES)


