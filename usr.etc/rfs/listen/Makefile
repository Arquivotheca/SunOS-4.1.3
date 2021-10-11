#
#       @(#)Makefile 1.1 92/07/30 SMI
#
#       Copyright 1987 Sun Microsystems 
#
#       Copyright (c) 1984 AT&T
#         All Rights Reserved

#       THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#       The copyright notice above does not evidence any
#       actual or intended publication of such source code.

#ident	"@(#)listen:listen.mk	1.14.2.1"

#
# listen.mk:
# makefile for network listener - System V release 3.0
#

DESTDIR=
CC = /usr/5bin/cc
CFLAGS= -O
LDFLAGS= $(LLDFLAGS)
LDLIBS = -lnsl
LIBNLS = libnls.a
INC=.
LSTINC=.

.MAKE.STATE:

.INIT: listen.h lserror.h lsnlsmsg.h lssmbmsg.h print.h lsdbf.h lsfiles.h \
       lsparam.h   nlsenv.h

# change the next two lines to compile with -g
# OPT	= -g
LLDFLAGS= -s

CHKDIR = $(DESTDIR)/var/net $(DESTDIR)/var/net/nls
INSDIR = $(DESTDIR)/var/net/nls
LIBDIR = $(DESTDIR)/usr/lib

LSUID	= daemon
LSGID	= daemon
LIBID	= bin

LLDLIBS	= \
	$(LIBNLS)


# The DEBUG module can always be included...
# if DEBUGMODE is undefined, no code gets compiled.
# doprnt.o is from system V rel 5.0.5.

DBGOBJ	= doprnt.o
DBGSRC	= doprnt.c

SRC	= \
	listen.c \
	lsdata.c \
	lsdbf.c \
	lslog.c \
	lssmb.c \
	nlsaddr.c \
	nlsdata.c $(DBGSRC)

LIBSRC = \
	nlsenv.c \
	nlsrequest.c

S4SRC = \
	lsnames.c \
	nlsconnect.c \
	nlsname.c \
	nlsestab.c \
	putenv.c

PRODUCT	= $(INSDIR)/listen

LSTINCS = \
	$(LSTINC)/lsparam.h \
	$(LSTINC)/lsfiles.h \
	$(LSTINC)/lserror.h \
	$(LSTINC)/lsnlsmsg.h \
	$(LSTINC)/lssmbmsg.h \
	$(LSTINC)/lsdbf.h

DBGINCS	= \
	$(LSTINC)/lsparam.h \
	$(LSTINC)/print.h

SMBINCS = \
	$(LSTINC)/lssmbmsg.h \
	$(LSTINC)/lsparam.h \
	$(LSTINC)/lsdbf.h

CONINCS = \
	$(LSTINC)/listen.h

ALLINCS = $(LSTINCS) $(DBGINCS) $(SMBINCS) $(CONINCS)

LSOBJS	= \
	listen.o \
	lslog.o \
	lsdbf.o \
	lssmb.o \
	lsdata.o \
	nlsdata.o \
	nlsaddr.o

NLSOBJS = \
	$(LIBNLS)(nlsenv.o) \
	$(LIBNLS)(nlsrequest.o)

S4OBJS = \
	lsnames.o \
	nlsconnect.o \
	nlsname.o \
	nlsestab.o \
	putenv.o

all:	listen libnls

# 
# SHAREDLIB version
#

listen:		$(LSOBJS) $(DBGOBJ) $(LLDLIBS) 
		$(CC) $(CFLAGS) -o listen $(LSOBJS) $(DBGOBJ) \
			$(LDFLAGS) $(LLDLIBS) $(LDLIBS) $(SYSCALLS)

libnls:	$(LIBNLS)

$(LIBNLS):	$(NLSOBJS)
		ranlib $(LIBNLS)

.PRECIOUS:	$(LIBNLS)

s4stuff:	$(S4OBJS)
		$(CC) -c $(S4OBJS)

listen.o:	$(LSTINCS)
lsdbf.o:	$(LSTINCS)
lslog.o:	$(LSTINCS)
lssmb.o:	$(LSTINC)/lssmbmsg.h \
		$(LSTINC)/lsparam.h $(LSTINC)/lsdbf.h
lsdata.o:	$(LSTINCS)
doprnt.o:	$(DBGINCS)

# listener library routines and /usr/include headers:
# putenv.o should only be included on an S4. (not included in s4's libc.a)
# In addition, nlsconnect.o, nlsestab.o, and nlsname.o are only on the S4.

$(LIBNLS)(nlsenv.o):	$(LSTINC)/nlsenv.h
$(LIBNLS)(nlsrequest.o):	$(LSTINC)/listen.h

# S4 stuff only

nlsconnect.o:	$(CONINCS)
nlsname.o:	$(INC)/string.h $(INC)/ctype.h
nlsestab.o:	$(INC)/stdio.h $(INC)/ctype.h $(INC)/fcntl.h \
		$(INC)/errno.h $(INC)/sys/utsname.h \
		$(INC)/sys/tiuser.h $(LSTINC)/listen.h
putenv.o:


install: all
	-rm -f $(LIBDIR)/$(LIBNLS)
	install -d -o $(LIBID) -g $(LIBID) -m 755 $(LIBDIR)
	install -o $(LIBID) -g $(LIBID) -m 644 $(LIBNLS) $(LIBDIR)
	install -o $(LSUID) -g $(LSGID) listen $(DESTDIR)/usr/etc
	(cd $(DESTDIR)/var/net/nls; \
	rm -f listen; \
	ln -s ../../../../usr/etc/listen listen)

$(CHKDIR):
		if test ! -d $@; then \
			mkdir $@; \
			/usr/etc/chown $(LSUID)	$@; \
			/usr/bin/chgrp $(LSGID)	$@; \
		fi

clean:
	$(RM) $(BINS) libnls.a *.o
