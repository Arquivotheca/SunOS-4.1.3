#	Copyright (c) 1984 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)Makefile 1.1 92/07/30 SMI"; /* from S5R3.1 acct:acct.mk 1.9.1.3 */
ROOT = $(DESTDIR)
TESTDIR = .
FRC =
INS = install
ARGS =
INSDIR = $(ROOT)/usr/lib/acct
ADMDIR = $(ROOT)/var/adm
ROOTBIN = $(ROOT)/usr/bin
WKDIR = $(ADMDIR) $(ADMDIR)/acct $(ADMDIR)/acct/nite $(ADMDIR)/acct/fiscal\
	$(ADMDIR)/acct/sum
CC = /usr/5bin/cc
CFLAGS = -O
LIB = lib/a.a
HDRS= acctdef.h ctmp.h ptmp.h tacct.h

all:	acctcms acctcom acctcon1\
	acctcon2 acctdisk acctdusg acctmerg accton\
	acctprc1 acctprc2 acctwtmp\
	diskusg fwtmp wtmpfix\
	chargefee.sh ckpacct.sh dodisk.sh lastlogin.sh\
	monacct.sh nulladm.sh prctmp.sh prdaily.sh\
	prtacct.sh remove.sh runacct.sh\
	shutacct.sh startup.sh turnacct.sh holidays \
	ptecms.awk ptelus.awk getname

$(LIB): $(HDRS)
		cd lib; make "INS=$(INS)" "CC=$(CC)"

acctcms:	$(LIB) acctcms.c $(HDRS)
		$(CC) $(CFLAGS) $(LFLAGS) $(FFLAG) \
			acctcms.c $(LIB) -o $(TESTDIR)/acctcms

acctcom:	$(LIB) acctcom.c $(HDRS)
		$(CC) $(CFLAGS) $(LFLAGS) $(FFLAG) \
			acctcom.c $(LIB) -o $(TESTDIR)/acctcom

acctcon1:	$(LIB) acctcon1.c $(HDRS)
		$(CC) $(CFLAGS) $(LFLAGS) $(FFLAG) \
			acctcon1.c $(LIB) -o $(TESTDIR)/acctcon1

acctcon2:	acctcon2.c $(HDRS)
		$(CC) $(CFLAGS) $(LFLAGS) $(FFLAG) \
			acctcon2.c -o $(TESTDIR)/acctcon2

acctdisk:	acctdisk.c $(HDRS)
		$(CC) $(CFLAGS) $(LFLAGS) $(FFLAG) \
			acctdisk.c -o $(TESTDIR)/acctdisk

acctdusg:	acctdusg.c $(HDRS)
		$(CC) $(CFLAGS) $(LFLAGS) $(FFLAG) \
			acctdusg.c -o $(TESTDIR)/acctdusg

acctmerg:	$(LIB) acctmerg.c $(HDRS)
		$(CC) $(CFLAGS) $(LFLAGS) $(FFLAG) \
			acctmerg.c $(LIB) -o $(TESTDIR)/acctmerg

accton:		accton.c $(HDRS)
		$(CC) $(CFLAGS) $(LFLAGS) $(FFLAG) \
			accton.c -o $(TESTDIR)/accton

acctprc1:	$(LIB) acctprc1.c $(HDRS)
		$(CC) $(CFLAGS) $(LFLAGS) $(FFLAG) \
			acctprc1.c $(LIB) -o $(TESTDIR)/acctprc1

acctprc2:	acctprc2.c $(HDRS)
		$(CC) $(CFLAGS) $(LFLAGS) $(FFLAG) \
			acctprc2.c -o $(TESTDIR)/acctprc2

acctwtmp:	acctwtmp.c $(HDRS)
		$(CC) $(CFLAGS) $(LFLAGS) $(FFLAG) \
			acctwtmp.c -o $(TESTDIR)/acctwtmp

diskusg:	diskusg.c $(HDRS)
		$(CC) $(CFLAGS) $(LFLAGS) $(FFLAG) \
			diskusg.c -o $(TESTDIR)/diskusg

fwtmp:		fwtmp.c $(HDRS)
		$(CC) $(CFLAGS) $(LFLAGS) $(FFLAG) \
			fwtmp.c -o $(TESTDIR)/fwtmp

wtmpfix:	wtmpfix.c $(HDRS)
		$(CC) $(CFLAGS) $(LFLAGS) $(FFLAG) \
			wtmpfix.c -o $(TESTDIR)/wtmpfix

getname:	getname.c $(HDRS)
		$(CC) $(CFLAGS) $(LFLAGS) $(FFLAG) \
			getname.c -o $(TESTDIR)/getname

clean:
		-rm -f *.o
		-rm  acctcms acctcom acctcon1 acctcon2 acctdisk diskusg\
			acctdusg acctmerg accton acctprc1 acctprc2 acctwtmp\
			fwtmp wtmpfix getname
		cd lib; make clean

$(INSDIR) $(WKDIR):
		$(INS) -d -o bin -m 755 $@

remove_acct:
		rm -fr $(ADMDIR)/acct

install:	all remove_acct $(INSDIR) $(WKDIR)
		$(INS) -s $(TESTDIR)/acctcms $(INSDIR)
		$(INS) -s $(TESTDIR)/acctcom $(ROOTBIN)
		$(INS) -s $(TESTDIR)/acctcon1 $(INSDIR)
		$(INS) -s $(TESTDIR)/acctcon2 $(INSDIR)
		$(INS) -s $(TESTDIR)/acctdisk $(INSDIR)
		$(INS) -s $(TESTDIR)/acctdusg $(INSDIR)
		$(INS) -s $(TESTDIR)/acctmerg $(INSDIR)
		$(INS) -s -o root -g staff -m 4755 $(TESTDIR)/accton $(INSDIR)
		$(INS) -s $(TESTDIR)/acctprc1 $(INSDIR)
		$(INS) -s $(TESTDIR)/acctprc2 $(INSDIR)
		$(INS) -s $(TESTDIR)/acctwtmp $(INSDIR)
		$(INS) -s $(TESTDIR)/fwtmp $(INSDIR)
		$(INS) -s $(TESTDIR)/diskusg $(INSDIR)
		$(INS) -s $(TESTDIR)/wtmpfix $(INSDIR)
		$(INS) -s $(TESTDIR)/getname $(INSDIR)
		$(INS) $(TESTDIR)/chargefee.sh $(INSDIR)/chargefee
		$(INS) $(TESTDIR)/ckpacct.sh $(INSDIR)/ckpacct
		$(INS) $(TESTDIR)/dodisk.sh $(INSDIR)/dodisk
		$(INS) $(TESTDIR)/lastlogin.sh $(INSDIR)/lastlogin
		$(INS) $(TESTDIR)/monacct.sh $(INSDIR)/monacct
		$(INS) $(TESTDIR)/nulladm.sh $(INSDIR)/nulladm
		$(INS) $(TESTDIR)/prctmp.sh $(INSDIR)/prctmp
		$(INS) $(TESTDIR)/prdaily.sh $(INSDIR)/prdaily
		$(INS) $(TESTDIR)/prtacct.sh $(INSDIR)/prtacct
		$(INS) $(TESTDIR)/remove.sh $(INSDIR)/remove
		$(INS) $(TESTDIR)/runacct.sh $(INSDIR)/runacct
		$(INS) $(TESTDIR)/shutacct.sh $(INSDIR)/shutacct
		$(INS) $(TESTDIR)/startup.sh $(INSDIR)/startup
		$(INS) $(TESTDIR)/turnacct.sh $(INSDIR)/turnacct
		$(INS) -m 644 $(TESTDIR)/holidays $(ROOT)/etc
		$(INS) -m 644 $(TESTDIR)/ptecms.awk $(INSDIR)
		$(INS) -m 644 $(TESTDIR)/ptelus.awk $(INSDIR)
