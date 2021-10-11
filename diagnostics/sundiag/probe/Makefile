#
#	@(#)Makefile 1.1 92/07/30 SMI;
#

.DEFAULT:
	sccs get -G$@ $@

REV=NEW
##### Some code will compile only on new revs of the 4.0 SunOS. specify "REV="
##### if built on older versions(before 4.0.3).

DESTDIR =
##### The next line is a temporary SVR4 specific include directory.
INC = 
##### Specify "INC=/usr/src/uts/sun4c" for SVR4 version.
##### Should be deleted after the include directory bugfix.

INCDIR	= ../include
ONLINEDIR  = ../../lib
OS=
ARCH=`arch`

INCLUDES= probe.h openprom.h $(INCDIR)/sdrtns.h \
	$(ONLINEDIR)/include/libonline.h $(ONLINEDIR)/include/probe_sundiag.h

SRCS	= probe.c probe_nlist.c probe_devs.c probe_disks.c probe_nets.c \
	probe_ports.c probe_tapes.c probe_fbs.c probe_check.c
OBJS	= $(SRCS:.c=.o)
LINTFILES = $(SRCS:.c=.ln)

SDLIBS = ../lib/libtest.a
ONLINELIB = $(ONLINEDIR)/libonline.a
LIBS    = -lpixrect -lkvm

#SVR4 specific defines
NETLIB = -lsocket -lnsl
KVMLIB = -lkvm -lelf
SVR4LIBS    = $(NETLIB) $(KVMLIB)
#end SVR4 specific defines

PROGS	= probe

DBX	=-O
# specify DBX=-g for dbx version
CFLAGS	= $(DBX) -D$(OS) -D$(REV) -D$(ARCH) -I$(INCDIR) -I$(INC)
LDFLAGS = $(DBX)
LINTFLAGS = -D$(ARCH) -I$(INCDIR)

.KEEP_STATE:

##### beginning of dependency lines #####

all: $(INCLUDES) $(PROGS)

$(PROGS): $(OBJS)
	@if [ $(OS) ]; then \
	  set -x; \
	  cc -o $@ $(OBJS) $(SDLIBS) $(ONLINELIB) $(SVR4LIBS) ; \
	else \
	  set -x; \
	$(CC) -o probe $(OBJS) $(SDLIBS) $(ONLINELIB) $(LIBS) ; \
	fi

install: all FRC
	@if [ $(DESTDIR) ]; then \
	  set -x; install -s $(PROGS) $(DESTDIR); \
	else \
	  set -x; install $(PROGS) ../bin; \
	fi

clean: FRC
	rm -f $(PROGS) $(OBJS) $(LINTFILES)

lint: $(LINTFILES)
	lint $(LINTFLAGS) $(LINTFILES) $(LIBS)

info: FRC
	sccs info

FRC:

