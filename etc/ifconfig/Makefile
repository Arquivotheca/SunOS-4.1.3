#
# @(#)Makefile 1.1 92/07/30 SMI
#
PROG =		ifconfig

CFLAGS =	

LDFLAGS +=	-Bstatic

OBJS=		ifconfig.o revarp.o

.KEEP_STATE:

all:		$(PROG)

$(PROG):	$(OBJS)
	$(CC) $(LDFLAGS) -o $(PROG) $(OBJS)

clean:
	-rm -f $(OBJS)

clobber:	clean
	-rm -f $(PROG)

install:	$(PROG)
	install -s $(PROG) $(DESTDIR)/usr/etc
	ln -s ../usr/etc/$(PROG) $(DESTDIR)/etc/$(PROG)
