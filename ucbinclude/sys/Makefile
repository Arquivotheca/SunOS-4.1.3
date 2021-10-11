#
# @(#)Makefile 1.1 92/07/30 SMI
#
HFILES = fcntl.h

all: $(HFILES)

clean:

install: $(HFILES)
	install -d -o bin -m 755 ${DESTDIR}/usr/include/sys
	install -m 444  $(HFILES) $(DESTDIR)/usr/include/sys
