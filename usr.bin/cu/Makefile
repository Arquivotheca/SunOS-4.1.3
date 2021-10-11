#	Copyright (c) 1984 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident  "@(#)Makefile 1.1 92/07/30"	/* from SVR3.2 cu:cu.mk 2.12 */
#
.KEEP_STATE:

OWNER=uucp
GRP=uucp
ROOT=$(DESTDIR)
BIN=$(ROOT)/usr/bin
UUDIR=../uucp
LDFLAGS =

CUDIR = .
#CFLAGS = -O -DSTANDALONE -DSMALL -I$(UUDIR)	#for smaller a.outs
CFLAGS = -O -DSTANDALONE -I$(UUDIR)
#TLILIB = -lnsl_s

# DKLIB is no longer used, datakit files are included in uucp
#LIBS= -lc_s ${DKLIB} ${TLILIB} 

#LIBS= -lc_s  ${TLILIB} 
# OLD=-o to save old files upon install
#INS=cpset
# If you system does not have "cpset"
#  use the one in the uucp source directory and use the
#  next INS=../uucp/Cpset line instead of the INS=cpset line above.
INS=../uucp/Cpset

UUOBJS = callers.o conn.o uucpdefs.o ulockf.o getargs.o interface.o strsave.o \
	 sysfiles.o strecpy.o stoa.o uucpname.o anlwrk.o cpmv.o gnamef.o \
	 systat.o utility.o setmode.o
UUSRC = $(UUOBJS:%.o=$(UUDIR)/%.c)
UUHDRS=	$(UUDIR)/uucp.h $(UUDIR)/dk.h $(UUDIR)/parms.h $(UUDIR)/sysfiles.h \
	$(UUDIR)/sysexits.h

CUOBJS =  cu.o altconn.o culine.o $(UUOBJS)
CUSRC =  cu.c altconn.c culine.c $(UUSRC)

ODK=dkbreak.o dkdial.o dkminor.o dtnamer.o dkerr.o 
LDK=$(ODK:%.o=$(UUDIR)/%.c)

.INIT:	$(UUHDRS)

cu:	$(CUOBJS) $(ODK)
	$(CC) $(CFLAGS) $(CUOBJS) $(ODK) $(LIBS) $(LDFLAGS) -o cu

all:	 cu

install:	all
	install -s -o $(OWNER) -g $(GRP) -m 4111 cu $(BIN)

clean:
	-rm -f *.o cu

clobber: clean
	rm -f cu

%.o:	$(UUDIR)/%.c
	$(CC) $(CFLAGS) -c $<
