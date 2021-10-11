#
# @(#)Makefile 1.1 92/07/30 SMI
#

FILES= build nightly sccs_clean
DESTDIR= /var/adm/build

all: $(FILES)

install: $(FILES)
	install -d -m 775 -o root -g staff $(DESTDIR)
	install $(FILES) $(DESTDIR)
