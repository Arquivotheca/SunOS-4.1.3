#	Copyright (c) 1984 AT&T
#	  All Rights Reserved
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.
#
# @(#)Makefile 1.1 92/07/30 SMI; from S5R3 1.16
#

ARCH= $(TARGET_ARCH:-%=%)
SPOOLDIR=	$(DESTDIR)/var/spool/cron
CRONSPOOLDIR=	$(SPOOLDIR)/crontabs
ATSPOOLDIR=	$(SPOOLDIR)/atjobs

SRCS= cron.c crontab.c at.c atq.c atrm.c funcs.c atfuncs.c permit.c elm.c
ALLOBJS= $(SRCS:.c=.o)
OBJS= funcs.o atfuncs.o permit.o
HDRS= cron.h
FILES= batch.sh .proto queuedefs crontab.root
BINS= cron crontab at atq atrm

CFLAGS= -O

.KEEP_STATE:

all:	$(BINS)

install: mkdir $(BINS) $(FILES) crontab.root.$(ARCH)
	install -s cron $(DESTDIR)/usr/etc
	install -s -m 4755 crontab at atq atrm $(DESTDIR)/usr/bin
	install -m 755 batch.sh $(DESTDIR)/usr/bin/batch
	install -m 644 .proto queuedefs $(SPOOLDIR)
	@set -x; for i in $(SPOOLDIR)/at.deny $(SPOOLDIR)/cron.deny; do \
		cat /dev/null > $$i && chown root $$i && chmod 644 $$i; \
	done
	install -m 400 crontab.root.$(ARCH) $(CRONSPOOLDIR)/root

mkdir:
	install -d $(DESTDIR)/usr/bin
	install -d $(DESTDIR)/usr/etc
	install -d -o root -m 755 $(SPOOLDIR)
	install -d -o root -m 755 $(CRONSPOOLDIR)
	install -d -o root -m 755 $(ATSPOOLDIR)

crontab.root.%   : crontab.root
	cp $<  $@
crontab.root.sun4: crontab.root
	sed -e "/fparel/d" $? > $@

libelm.a: elm.o
	ar cr $@ elm.o
	ranlib $@

cron:	$$@.o funcs.o libelm.a
	$(LINK.c) -o $@ $@.o funcs.o libelm.a

crontab: $$@.o permit.o funcs.o
	$(LINK.c) -o $@ $@.o permit.o funcs.o

$(ALLOBJS) : $(HDRS)

at atq atrm : $$@.o $(OBJS)
	$(LINK.c) -o $@ $@.o $(OBJS)

tags: $(HDRS) $(SRCS)
	ctags -tw $(HDRS) $(SRCS)

clean:
	$(RM) $(BINS) $(ALLOBJS) $(FILES) \
		libelm.a tags crontab.root.$(ARCH)
