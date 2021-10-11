#
# @(#)Makefile 1.1 92/07/30 SMI
#

LOCALES = C En_US En_GB
TIMEDIR = usr/share/lib/locale/LC_TIME

all: $(LOCALES)

C: mkclocale
	mkclocale >C

clean:
	rm -f mkclocale C

install: $(LOCALES)
	install -d -m 755 $(DESTDIR)/$(TIMEDIR)
	install -m 644 $(LOCALES) $(DESTDIR)/$(TIMEDIR)
	rm -f $(DESTDIR)/$(TIMEDIR)/default
	ln $(DESTDIR)/$(TIMEDIR)/C $(DESTDIR)/$(TIMEDIR)/default
