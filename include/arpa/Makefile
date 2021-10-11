#	@(#)Makefile 1.1 92/07/30 SMI
HFILES=	ftp.h inet.h nameser.h telnet.h tftp.h


all: $(HFILES)

clean:

install: $(HFILES)
	install -d -o bin -m 755 ${DESTDIR}/usr/include/arpa
	install -m 444  $(HFILES) $(DESTDIR)/usr/include/arpa
