#
# @(#)Makefile 1.1 92/07/30 Copyright (C) 1986 by Sun Microsystems, Inc.
#
BINS= keyserv keyenvoy
CFLAGS= -O 

OBJS= keyserv.o setkey.o detach.o

#
# Do not use .KEEP_STATE. It screws up recursive makes.
#.KEEP_STATE:
#

all: $(BINS)

keyserv: $(OBJS)
	$(LINK.c) -o $@ $(OBJS) -lrpcsvc -lmp

clean:
	$(RM) $(BINS) $(OBJS)

install: $(BINS)
	install -d $(DESTDIR)/usr/etc
	install -s -m  755 keyserv  $(DESTDIR)/usr/etc
	install -s -m 4755 keyenvoy $(DESTDIR)/usr/etc

international:
	-rm -f keyserv $(OBJS)
	(LD_LIBRARY_PATH=$(DESTDIR)/usr/lib; export LD_LIBRARY_PATH; \
	$(MAKE) $(MFLAGS) "CFLAGS=$(CFLAGS) $(IFLAGS)" keyserv)
	-install -s -m 755 keyserv $(DESTDIR)/usr/etc
	-rm -f keyserv $(OBJS)
