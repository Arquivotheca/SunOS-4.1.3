#
#  SENDMAIL Makefile.
#
#	Version:
#	    @(#)Makefile 1.1 92/07/30 SMI; from UCB 3.67 12/24/82
#

LIBS=	-ldbm
DESTDIR=

OBJS1=	conf.o main.o collect.o parseaddr.o alias.o \
	savemail.o err.o readcf.o stab.o headers.o recipient.o \
	remote.o stats.o usersmtp.o srvrsmtp.o queue.o \
	macro.o util.o clock.o trace.o envelope.o
NOMXOBJS= daemon.o deliver.o
OBJS2=	sysexits.o arpadate.o convtime.o
OBJS=	$(OBJS1) $(OBJS2)
SRCS1=	conf.h sendmail.h mailstats.h \
	conf.c deliver.c main.c parseaddr.c err.c alias.c savemail.c \
	sysexits.c util.c arpadate.c version.c collect.c \
	macro.c headers.c readcf.c stab.c recipient.c stats.c daemon.c \
	usersmtp.c srvrsmtp.c queue.c clock.c trace.c envelope.c
SRCS2=	TODO convtime.c
SRCS=	$(SRCS1) $(SRCS2)
ALL=	sendmail sendmail.mx

# Resolver for Arpanet-style MX records
MXLIBS= ../../libresolv/libresolv.a -ldbm
MXOBJS= daemon-mx.o domain-mx.o deliver-mx.o
MXCONFIG=-I../include -I/usr/include/arpa -DVMUNIX -DMXDOMAIN

CHOWN=	-echo chown
CHMOD=	chmod
O=	-O
COPTS= -DINTER
CCONFIG=-I../include -DVMUNIX -DBSD=43
CFLAGS=	$O $(COPTS) $(CCONFIG)
AR=	-ar
ARFLAGS=rvu
LINT=	lint -hbacz -DVMUNIX -I../include
XREF=	ctags -x
CTAGS=	ctags
CP=	cp
MV=	mv
INSTALL=install -c -s -o root -m 4551
M4=	m4
TOUCH=	touch
ABORT=	false

GET=	sccs get
DELTA=	sccs delta
WHAT=	sccs what
PRT=	sccs prt
REL=

ROOT=	root
all: $(ALL)

sendmail: $(OBJS1) $(OBJS2) $(NOMXOBJS) version.o
	$(CC) $(COPTS) -o sendmail version.o $(OBJS1) $(OBJS2) \
		 $(NOMXOBJS) $(LIBS)
	size sendmail; ls -l sendmail; $(WHAT) < version.o

daemon-mx.o: daemon.c
	$(CC) -o daemon-mx.o -c $O $(MXCONFIG) daemon.c

syslog-mx.o: syslog.c
	$(CC) -o syslog-mx.o -c $O $(MXCONFIG) syslog.c

domain-mx.o: domain.c
	$(CC) -o domain-mx.o -c $O $(MXCONFIG) domain.c

deliver-mx.o: deliver.c
	$(CC) -o deliver-mx.o -c $O $(MXCONFIG) deliver.c

sendmail.mx: $(OBJS1) $(OBJS2) $(MXOBJS) version.o 
	$(CC) $(COPTS) -o sendmail.mx version.o $(OBJS1) \
		$(OBJS2) $(MXOBJS) $(MXLIBS)

install: all
	$(INSTALL) sendmail $(DESTDIR)/usr/lib
	$(INSTALL) sendmail.mx $(DESTDIR)/usr/lib

$(OBJS1): sendmail.h
$(OBJS): conf.h

sendmail.h util.o: ../include/useful.h
sysexits.o: ../include/sysexits.h

#
#  Auxiliary support entries
#

clean:
	rm -f core sendmail rmail usersmtp uucp a.out XREF sendmail.cf
	rm -f *.o Makefile 

sources: $(SRCS)

tags:	$(SRCS1)
	$(CTAGS) $(SRCS1)

print: $(SRCS)
	@ls -l | pr -h "sendmail directory"
	@$(XREF) *.c | pr -h "cross reference listing"
	@size *.o | pr -h "object code sizes"
	@pr Makefile *.m4 *.h *.[cs]

lint:
	$(LINT) $(CCONFIG) $(SRCS1)

stats.o: mailstats.h
