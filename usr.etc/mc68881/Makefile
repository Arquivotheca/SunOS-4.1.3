#
# @(#)Makefile 1.1 92/07/30 SMI; Copyr 1986 Sun Micro
#
BINS= mc68881version
OBJS= mc68881version.o mc68881subroutine.o
CFLAGS=	-O

.KEEP_STATE:

all: $(BINS)

mc68881version: $(OBJS)
	$(LINK.c) -o $@ $(OBJS)

install: $(BINS)
	install -d $(DESTDIR)/usr/etc
	install -s $(BINS) $(DESTDIR)/usr/etc

clean:
	$(RM) $(BINS) $(OBJS)
