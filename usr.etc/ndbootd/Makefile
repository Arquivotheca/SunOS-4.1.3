#
#	@(#)Makefile 1.1 92/07/30 SMI
#
#	Copyright (c) 1986 by Sun Microsystems, Inc.
#
# ndbootd should compile and run on any Unix machine.
# This presumes that routines are provided to do
# Ethernet address translation (on a Sun, these are
# provided in ether_addr.c in the C library).
# It is also necessary to be able to listen for ND
# packets on a raw IP socket (this feature is broken
# in some Unix versions, e.g. Sun 3.0).  See README.
#
BINS= ndbootd
HDRS= nddefs.h
CFLAGS= -O

.KEEP_STATE:

.INIT: $(HDRS)

all: $(BINS)

install: $(BINS)
	install -d $(DESTDIR)/usr/etc
	install -s $(BINS) $(DESTDIR)/usr/etc

clean:
	$(RM) $(BINS) $(OBJS)
