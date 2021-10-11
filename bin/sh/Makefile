# @(#)Makefile 1.1 92/07/30 SMI; from S5R2 1.8
#

.KEEP_STATE:

BINS = sh
BINDIR = $(DESTDIR)/usr/bin
DOPRNT = ../../lib/libc/stdio/common
LINKDIR = ../bin

LOCAL_HDRS = brkincr.h ctype.h defs.h hash.h mac.h mode.h name.h \
	stak.h sym.h timeout.h

OBJS = setbrk.o blok.o stak.o cmd.o fault.o main.o word.o string.o\
	name.o args.o xec.o service.o error.o io.o print.o macro.o expand.o\
	ctype.o msg.o test.o defs.o echo.o hash.o hashserv.o pwd.o func.o\
	doprnt.o

CFLAGS = -O -DRES
LDFLAGS = -Bstatic

ctype.o := CFLAGS += -R

.INIT:  $(HDRS) $(LOCAL_HDRS)

bins: $(BINS)

$(BINS): $$(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(LINK_LIBS)

doprnt.o:	$(DOPRNT)/doprnt.c
	$(CC) $(CFLAGS) -c -I$(DOPRNT) -DFLOAT=0 $(DOPRNT)/doprnt.c

install: $(BINS)
	install -d -m 755 $(BINDIR)
	install -s $(BINS) $(BINDIR)
	install -d -o bin -m 755 $(DESTDIR)/usr/kvm/stand
	install -s $(BINS) $(DESTDIR)/usr/kvm/stand
	/bin/rm -f $(DESTDIR)/usr/lib/rsh; \
		/bin/ln -s $(LINKDIR)/sh $(DESTDIR)/usr/lib/rsh

install_h:

clean:
	-rm -rf $(LOCAL_BINS) $(BINS) $(LOCAL_LIBS) $(LIBS) $(OBJS) \
		core a.out install_bins
