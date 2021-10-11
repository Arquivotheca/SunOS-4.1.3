#
# @(#)Makefile 1.1 92/07/30 SMI
#

LOCALES = C En_US En_GB
MONETARYDIR = usr/share/lib/locale/LC_MONETARY

all: $(LOCALES)

C: mkclocale
	mkclocale >C

clean:
	rm -f mkclocale C

install: $(LOCALES)
	install -d -m 755 $(DESTDIR)/$(MONETARYDIR)
	install -m 644 $(LOCALES) $(DESTDIR)/$(MONETARYDIR)
	rm -f $(DESTDIR)/$(MONETARYDIR)/default
	ln $(DESTDIR)/$(MONETARYDIR)/C $(DESTDIR)/$(MONETARYDIR)/default
