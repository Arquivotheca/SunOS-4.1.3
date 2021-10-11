#
# @(#)Makefile 1.1 92/07/30 SMI; from UCB X.X 06/12/83
#
BINS= in.routed
OBJS= af.o if.o inet.o input.o main.o output.o \
	startup.o tables.o timer.o trace.o
HDRS= defs.h trace.h interface.h table.h af.h

CFLAGS=	-O

.KEEP_STATE:

.INIT: $(HDRS)

all: $(BINS)

$(BINS): $(OBJS)
	$(LINK.c) -o $@ $(OBJS)

install: $(BINS)
	install -d $(DESTDIR)/usr/etc
	install -s $(BINS) $(DESTDIR)/usr/etc

clean:
	$(RM) $(BINS) $(OBJS)
