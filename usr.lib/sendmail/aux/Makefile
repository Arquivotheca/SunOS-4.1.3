#
# @(#)Makefile 1.1 92/07/30 SMI; from UCB 3.6 02/18/83
#
#
#  Makefile for assorted programs related (perhaps distantly) to Sendmail.
#

ALL=	mailstats	mconnect   vacation 	Makefile
SRCS=	mailstats.c	mconnect.c vacation.c

LIBS=	-ldbm
DBMLIB=	-ldbm
CONVTIME=../src/convtime.o
DESTDIR=

CHOWN=	-echo chown
CHMOD=	chmod
O=	-O
COPTS=
CCONFIG=-I../include -I../src -DVMUNIX
CFLAGS=	$O $(COPTS) $(CCONFIG)
AR=	-ar
CC=	/bin/cc
ARFLAGS=rvu
LINT=	lint
XREF=	ctags -x
CP=	cp
MV=	mv
INSTALL=install -c -s
M4=	m4
TOUCH=	touch
ABORT=	false

GET=	sccs get
DELTA=	sccs delta
WHAT=	sccs what
PRT=	sccs prt
REL=

ROOT=	root
OBJMODE=755

all: $(ALL)

mailstats: mailstats.o
	$(CC)  $(COPTS) -o $@ $@.o

logger: logger.o
	$(CC)  $(COPTS) -o $@ $@.o $(LIBS)

mconnect: mconnect.o
	$(CC)  $(COPTS) -o $@ $@.o

praliases: praliases.o
	$(CC)  $(COPTS) -o $@ $@.o

vacation: vacation.o
	$(CC)  $(COPTS) -o $@ $@.o $(CONVTIME) $(DBMLIB)

sources: $(SRCS)

$(SRCS):
	$(GET) $(REL) SCCS/s.$@

clean:
	rm -f $(ALL) core a.out make.out lint.out errs
	rm -f *.o ,*

vacation.o: ../include/userdbm.h
