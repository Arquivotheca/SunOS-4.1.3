#
#
# @(#)Makefile 1.1 92/07/30 SMI
#
BINS= c2convert
OBJS= main.o list.o read.o make.o update.o create.o split.o chownmod.o

CFLAGS=	-O
LDFLAGS= -n

.KEEP_STATE:

all: $(BINS)

$(BINS): $(OBJS)
	$(LINK.c) -o $@ $(OBJS)

clean:
	$(RM) $(BINS) $(OBJS)

install: $(BINS)
	install -d $(DESTDIR)/usr/lib
	install -s -m 755 $(BINS) $(DESTDIR)/usr/lib
