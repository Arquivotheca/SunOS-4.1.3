#
# @(#)Makefile 1.1 92/07/30 SMI
#

LOCALES = C En_US En_GB
NUMERICDIR = usr/share/lib/locale/LC_NUMERIC

all: $(LOCALES)

C: mkclocale
	mkclocale >C

clean:
	rm -f mkclocale C

install: $(LOCALES)
	install -d -m 755 $(DESTDIR)/$(NUMERICDIR)
	install -m 644 $(LOCALES) $(DESTDIR)/$(NUMERICDIR)
	rm -f $(DESTDIR)/$(NUMERICDIR)/default
	ln $(DESTDIR)/$(NUMERICDIR)/C $(DESTDIR)/$(NUMERICDIR)/default
