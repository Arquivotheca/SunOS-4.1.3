#	Copyright (c) 1984 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident  "@(#)Makefile 1.1 92/07/30"	/* from SVR3.2 uucp:uucp.mk 2.24.1.4 */
#
#	CFLAGS:
#	-DSMALL can be used on small machines.
#	It reduces debugging statements in the object code.
#
.KEEP_STATE:

.DEFAULT:
	sccs get -G$@ $@

CFLAGS = -O
LDFLAGS=

#	NOTE - when UNETLIB is to be used, define UNET in parms.h
# add eio.o to PROTOCOLS below
# UNETLIB = -lunet
#	NOTE - when TCP is defined in parms.h
# add eio.o to PROTOCOLS below
#	NOTE - when DKLIB is to be used, define DATAKIT in parms.h
# add dio.o to PROTOCOLS below
# DKLIB = -ldk
# Since release 3.2 (7/87), datakit files are included in uucp directory,
# it is no longer a library. But we still define DATAKIT in parms.h and 
# add dio.o to PROTOCOLS.
# 	NOTE - some sites use -lbtl for compatibility
# BTLLIB = -lbtl 
#	NOTE - when TLI is defined in parms.h
# add eio.o to PROTOCOLS below
#TLILIB = -lnsl_s

# PROTOCOLS= dio.o	# for datakit
# PROTOCOLS =  		# if no datakit and no unet
PROTOCOLS = eio.o fio.o dio.o	# TCP, TLI, or unet, PAD, datakit

#LIBS= -lc_s ${BTLLIB} ${UNETLIB} ${TLILIB}
LIBS= ${BTLLIB} ${UNETLIB} ${TLILIB}

# some versions of unix don't have strpbrk in libc
# STRPBRK = strpbrk.o
# STRPBRKSRC = strpbrk.c
# Use my getopt until the argument gobbling is fixed!

# use this on systems that don't have getopt() in libc
# GETOPT = getopt.o
# GETOPTSRC = getopt.c

# BSD can't compile uugetty -- remove next line for BSD
#UUGETTY=uugetty

# Use this if you have BSD networking and can run the UUCP daemon
UUCPD=in.uucpd

# Use this if you can support "nttysrv", whatever it is
#NTTYSRV=nttysrv

ROOT=$(DESTDIR)
OWNER=uucp
GRP=uucp
BIN=$(ROOT)/usr/bin
USRETC=$(ROOT)/usr/etc
INSDIR=$(ROOT)/usr/lib/uucp
ETCDIR=$(ROOT)/etc/uucp
# RELINSDIR is INSDIR relative to ETCDIR
RELINSDIR=../../usr/lib/uucp
# The a.out files go to ODIR
ODIR=.

# if you change these directories, change them in uucp.h as well

SPOOL=$(ROOT)/var/spool
LOCKS=$(ROOT)/var/spool/locks
#LOCKS=$(ROOT)/var/spool/uucp
PUBDIR=$(ROOT)/var/spool/uucppublic
UUCPDIR=$(ROOT)/var/spool/uucp
#		things are linked from UUCPDIR into XQTDIR and CORRUPT
XQTDIR=	$(UUCPDIR)/.Xqtdir
#		this is optional
CORRUPT=$(UUCPDIR)/.Corrupt
#		for logfiles
LOGDIR=$(UUCPDIR)/.Log
#		for sequence number files
SEQDIR=$(UUCPDIR)/.Sequence
#		for STST files
STATDIR=$(UUCPDIR)/.Status
LOGUUCP=$(LOGDIR)/uucp
LOGUUX=$(LOGDIR)/uux
LOGUUXQT=$(LOGDIR)/uuxqt
LOGCICO=$(LOGDIR)/uucico
WORKSPACE=$(UUCPDIR)/.Workspace
ADMIN=$(UUCPDIR)/.Admin
#		for saving old log files
OLDLOG=$(UUCPDIR)/.Old
#		maximum number of uuxqts at one time in this file
MAXUUXQTS=$(ETCDIR)/Maxuuxqts

#		Maximum number of uuscheds at one time
MAXUUSCHEDS=$(ETCDIR)/Maxuuscheds

CLEAN=
LINTOP=
COMMANDS=uucp uux uuxqt uucico uusched
TOOLS=uuname uucheck uustat uucleanup remote.unknown $(NTTYSRV) $(UUGETTY) \
	$(UUCPD)
OFILES=utility.o cpmv.o expfile.o gename.o getpwinfo.o \
	ulockf.o xqt.o logent.o versys.o gnamef.o systat.o \
	sysfiles.o strsave.o $(GETOPT)
LFILES=utility.c cpmv.c expfile.c gename.c getpwinfo.c \
	ulockf.c xqt.c logent.c versys.c gnamef.c systat.c \
	sysfiles.c strsave.c $(GETOPTSRC)
ODK=dkbreak.o dkdial.o dkminor.o dtnamer.o dkerr.o 
LDK=dkbreak.c dkdial.c dkminor.c dtnamer.c dkerr.c
OUUCP=uucpdefs.o uucp.o gwd.o permission.o getargs.o uucpname.o\
	gtcfile.o $(STRPBRK) chremdir.o mailst.o
LUUCP=uucpdefs.c uucp.c gwd.c permission.c getargs.c uucpname.c\
	gtcfile.c $(STRPBRKSRC) chremdir.c mailst.c
OUUX=uucpdefs.o uux.o gwd.o permission.o getargs.o getprm.o\
	uucpname.o gtcfile.o chremdir.o $(STRPBRK)
LUUX=uucpdefs.c uux.c gwd.c permission.c getargs.c getprm.c\
	uucpname.c gtcfile.c chremdir.c $(STRPBRKSRC)
OUUXQT=uucpdefs.o uuxqt.o mailst.o getprm.o uucpname.o \
	permission.o getargs.o gtcfile.o $(STRPBRK) \
	shio.o chremdir.o
LUUXQT=uucpdefs.c uuxqt.c mailst.c getprm.c uucpname.c \
	permission.c getargs.c gtcfile.c $(STRPBRKSRC) \
	shio.c chremdir.c
OUUCICO=uucpdefs.o cico.o conn.o callers.o cntrl.o pk0.o pk1.o \
	gio.o xio.o anlwrk.o permission.o getargs.o \
	gnxseq.o pkdefs.o imsg.o gtcfile.o \
	mailst.o uucpname.o line.o chremdir.o \
	interface.o statlog.o strecpy.o stoa.o \
	setmode.o \
	$(STRPBRK) $(PROTOCOLS)
LUUCICO=uucpdefs.c cico.c conn.c callers.c cntrl.c pk0.c pk1.c \
	gio.c xio.c anlwrk.c permission.c getargs.c \
	gnxseq.c pkdefs.c imsg.c gtcfile.c \
	mailst.c uucpname.c line.c chremdir.c \
	interface.c statlog.c strecpy.c stoa.c \
	setmode.c \
	$(STRPBRKSRC) eio.c dio.c
OUUNAME=uuname.o uucpname.o uucpdefs.o getpwinfo.o sysfiles.o strsave.o
LUUNAME=uuname.c uucpname.c uucpdefs.c getpwinfo.c sysfiles.c strsave.c
OUUSTAT=uustat.o gnamef.o expfile.o uucpdefs.o getpwinfo.o ulockf.o getargs.o \
	utility.o uucpname.o versys.o strsave.o sysfiles.o permission.o \
	$(STRPBRK) 
LUUSTAT=uustat.c gnamef.c expfile.c uucpdefs.c getpwinfo.c ulockf.c getargs.c \
	utility.c uucpname.c versys.c strsave.c sysfiles.c permission.c \
	$(STRPBRKSRC) 
OUUSCHED=uusched.o gnamef.o expfile.o uucpdefs.o getpwinfo.o uucpname.o \
	ulockf.o systat.o getargs.o utility.o strsave.o versys.o \
	sysfiles.o permission.o
LUUSCHED=uusched.c gnamef.c expfile.c uucpdefs.c getpwinfo.c uucpname.c \
	ulockf.c systat.c getargs.c utility.c strsave.c versys.c \
	sysfiles.c permission.c
OUUCLEANUP=uucleanup.o gnamef.o expfile.o uucpdefs.o getpwinfo.o \
	uucpname.o ulockf.o getargs.o cpmv.o utility.o strsave.o versys.o \
	sysfiles.o permission.o
LUUCLEANUP=uucleanup.c gnamef.c expfile.c uucpdefs.c getpwinfo.c \
	uucpname.c ulockf.c getargs.c cpmv.c utility.c strsave.c versys.c \
	sysfiles.c permission.c
OUUGETTY=uugetty.o ulockf.o uucpdefs.o
LUUGETTY=uugetty.c ulockf.c uucpdefs.c
OUNKNOWN=unknown.o
LUNKNOWN=unknown.c
ONTTYSRV=nttysrv.o
LNTTYSRV=nttysrv.c
OUUCPD=in.uucpd.o
LUUCPD=in.uucpd.c

UUCONFIG=Devices Dialers Permissions Poll Dialcodes Devconfig Sysfiles
UUSCRIPTS=uulog uuto uupick
UUDEMON=uudemon.hour uudemon.poll uudemon.cleanup uudemon.admin
UUCRONTAB=uudemon.crontab
UUSETUP=Uutry SetUp Cvt

INIT=init

.INIT:	dk.h parms.h pk.h sysexits.h sysfiles.h uucp.h

all:	$(INIT) $(COMMANDS) $(TOOLS) 

install:	mkdirs all cp

cp:	cpprog cplib

cpprog:	all cpuucp cpuux cpuuname cpuustat cpuuxqt cpuucico cpuusched \
	cpuucheck cpuucleanup cpuucpd

cpuucp:
	install -s -m 4111 -o $(OWNER) -g $(GRP) uucp $(BIN)

cpuux:
	install -s -m 4111 -o $(OWNER) -g $(GRP) uux $(BIN)

cpuuname:
	install -s -m 4111 -o $(OWNER) -g $(GRP) uuname $(BIN)

cpuustat:
	install -s -m 4111 -o $(OWNER) -g $(GRP) uustat $(BIN)

cpuuxqt:
	install -s -m 4111 -o $(OWNER) -g $(GRP) uuxqt $(INSDIR)

cpuucico:
	install -s -m 4111 -o $(OWNER) -g $(GRP) uucico $(INSDIR)

cpuusched:
	install -s -m 4111 -o $(OWNER) -g $(GRP) uusched $(INSDIR)

cpuucheck:
	install -s -m 110 -o $(OWNER) -g $(GRP) uucheck $(INSDIR)

cpuucleanup:
	install -s -m 110 -o $(OWNER) -g $(GRP) uucleanup $(INSDIR)

cpuucpd:
	install -s -m 500 -o $(OWNER) -g $(GRP) in.uucpd $(USRETC)

cplib:	${UUCONFIG} Systems ${UUSCRIPTS} ${UUDEMON} ${UUCRONTAB} ${UUSETUP}
	@set -x; for i in ${UUCONFIG}; do \
		install -c -m 644 -o $(OWNER) -g $(GRP) $$i $(ETCDIR); done
	install -c -m 640 -o $(OWNER) -g $(GRP) Systems $(ETCDIR)
	echo 2 >/tmp/$$$$; for i in $(MAXUUXQTS) $(MAXUUSCHEDS); do \
		install -c -m 644 -o $(OWNER) -g $(GRP) /tmp/$$$$ $$i; done; \
	$(RM) /tmp/$$$$
	@set -x; for i in ${UUSCRIPTS}; do \
		install -c -m 555 -o $(OWNER) -g $(GRP) $$i $(BIN); done
	@set -x; for i in ${UUDEMON}; do \
		install -c -m 555 -o $(OWNER) -g $(GRP) $$i $(ETCDIR); done
	install -c -m 400 -o $(OWNER) -g $(GRP) ${UUCRONTAB} $(INSDIR)
	@set -x; for i in ${UUSETUP}; do \
		install -c -m 555 -o $(OWNER) -g $(GRP) $$i $(INSDIR); done
	install -c -m 644 -o $(OWNER) -g $(GRP) /dev/null $(PUBDIR)/.hushlogin
	install -c -m 400 -o root -g $(GRP) /dev/null $(PUBDIR)/.rhosts
	install -s -m 4111 -o $(OWNER) -g $(GRP) remote.unknown $(INSDIR)
	$(RM) $(ETCDIR)/remote.unknown; \
	ln -s $(RELINSDIR)/remote.unknown $(ETCDIR)/remote.unknown

clean:
	-rm -f *.o
	-rm -f $(COMMANDS) $(TOOLS)

clobber:	clean
#	-rm -f $(COMMANDS) $(TOOLS)

cmp:	all
	cmp uucp $(BIN)/uucp
	rm uucp
	cmp uux $(BIN)/uux
	rm uux
	cmp uuxqt $(INSDIR)/uuxqt
	rm uuxqt
	cmp uucico $(INSDIR)/uucico
	rm uucico
	cmp uuname $(BIN)/uuname
	rm uuname
	cmp uucheck $(INSDIR)/uucheck
	rm uucheck
	cmp uucleanup $(INSDIR)/uucleanup
	rm uucleanup
	cmp uustat $(BIN)/uustat
	rm uustat
	cmp uusched $(INSDIR)/uusched
	rm uusched
	-cmp uugetty $(INSDIR)/uugetty
	-rm uugetty
	rm *.o

init:	anlwrk.o permission.o cpmv.o \
	dkbreak.o dkdial.o dkerr.o dkminor.o dtnamer.o expfile.o gename.o \
	getargs.o getprm.o getpwinfo.o gnamef.o \
	gnxseq.o gwd.o imsg.o logent.o \
	mailst.o shio.o \
	systat.o ulockf.o uucpname.o versys.o xqt.o

uucp:	$(OUUCP) $(OFILES) $(ODK)
	$(CC) $(CFLAGS)  $(LDFLAGS) $(OUUCP) $(OFILES) $(ODK) $(LIBS) \
		-o $(ODIR)/uucp

uux:	$(OUUX) $(OFILES) $(ODK)
	$(CC) $(CFLAGS)  $(LDFLAGS) $(OUUX) $(OFILES) $(ODK) $(LIBS) \
		-o $(ODIR)/uux

uuxqt:	$(OUUXQT) $(OFILES) $(ODK)
	$(CC) $(CFLAGS)  $(LDFLAGS) $(OUUXQT) $(OFILES) $(ODK) $(LIBS) \
		-o $(ODIR)/uuxqt

uucico:	$(OUUCICO) $(OFILES) $(ODK)
	$(CC) $(CFLAGS)  $(LDFLAGS) $(OUUCICO) $(OFILES) $(ODK) $(LIBS) \
		-o $(ODIR)/uucico

uuname:	$(OUUNAME)
	$(CC) $(CFLAGS)  $(LDFLAGS) $(OUUNAME) $(LIBS) \
		-o $(ODIR)/uuname

uustat:	$(OUUSTAT) $(GETOPT)
	$(CC) $(CFLAGS)  $(LDFLAGS) $(OUUSTAT) $(GETOPT) $(LIBS) \
		-o $(ODIR)/uustat
 
uucleanup:	$(OUUCLEANUP) $(GETOPT)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OUUCLEANUP) $(GETOPT) $(LIBS) \
		-o $(ODIR)/uucleanup
 
uusched:	$(OUUSCHED) $(GETOPT)
	$(CC) $(CFLAGS)  $(LDFLAGS) $(OUUSCHED) $(GETOPT) $(LIBS) \
		-o $(ODIR)/uusched
 
uucheck:	uucheck.o uucpname.o $(GETOPT)
	$(CC) $(CFLAGS)  $(LDFLAGS) uucheck.o uucpname.o $(GETOPT) $(LIBS) \
		-o $(ODIR)/uucheck
 
uugetty:	$(OUUGETTY)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OUUGETTY) \
		-o $(ODIR)/uugetty
 
remote.unknown:	$(OUNKNOWN)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OUNKNOWN) \
		-o $(ODIR)/remote.unknown
 
nttysrv:	$(ONTTYSRV)
	$(CC) $(CFLAGS) $(LDFLAGS) $(ONTTYSRV) \
		-o $(ODIR)/nttysrv
 
# uucheck.o:	permission.c
# 
# gio.o pk0.o pk1.o pkdefs.o:	pk.h
# 
# sysfiles.o:	sysfiles.h
# 
# utility.o permission.o cico.o conn.o callers.o cpmv.o\
# 	anlwrk.o cntrl.o dkbreak.o dkdial.o dkerr.o dkminor.o dtnamer.o \
#         expfile.o gename.o\
# 	getpwinfo.o gio.o xio.o gnamef.o gnxseq.o gwd.o imsg.o ioctl.o\
# 	logent.o mailst.o sdmail.o line.o shio.o\
# 	systat.o ulockf.o uucpdefs.o uucpname.o uuname.o\
# 	uucleanup.o uucheck.o uusched.o  uucp.o uustat.o dio.o \
# 	uux.o uuxqt.o versys.o xqt.o interface.o statlog.o \
# 	strecpy.o:	uucp.h parms.h dk.h sysexits.h

mkdirs:
	install -d -o root -g bin -m 755 $(SPOOL)
	install -d -o $(OWNER) -g $(GRP) -m 755 $(INSDIR)
	install -d -o $(OWNER) -g $(GRP) -m 755 $(ETCDIR)
	install -d -o $(OWNER) -g $(GRP) -m 755 $(UUCPDIR)
	install -d -o $(OWNER) -g $(GRP) -m 1777 $(PUBDIR)
	install -d -o $(OWNER) -g $(GRP) -m 755 $(XQTDIR)
	install -d -o $(OWNER) -g $(GRP) -m 755 $(CORRUPT)
	install -d -o $(OWNER) -g $(GRP) -m 755 $(LOGDIR)
	install -d -o $(OWNER) -g $(GRP) -m 755 $(SEQDIR)
	install -d -o $(OWNER) -g $(GRP) -m 755 $(STATDIR)
	install -d -o $(OWNER) -g $(GRP) -m 755 $(LOGUUCP)
	install -d -o $(OWNER) -g $(GRP) -m 755 $(LOGUUX)
	install -d -o $(OWNER) -g $(GRP) -m 755 $(LOGUUXQT)
	install -d -o $(OWNER) -g $(GRP) -m 755 $(LOGCICO)
	install -d -o $(OWNER) -g $(GRP) -m 755 $(WORKSPACE)
	install -d -o $(OWNER) -g $(GRP) -m 755 $(LOCKS)
	install -d -o $(OWNER) -g $(GRP) -m 755 $(ADMIN)
	install -d -o $(OWNER) -g $(GRP) -m 755 $(OLDLOG)

#  lint procedures

lint:	lintuucp lintuucico lintuux lintuuxqt \
	lintuuname

lintuucp:
	lint $(LINTOP) $(LUUCP) $(LFILES)

lintuucico:
	lint $(LINTOP) $(LUUCICO) $(LFILES)

lintuux:
	lint $(LINTOP) $(LUUX) $(LFILES)

lintuuxqt:
	lint $(LINTOP) $(LUUXQT) $(LFILES)

lintuuname:
	lint $(LINTOP) $(LUUNAME)
