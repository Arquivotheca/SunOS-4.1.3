#
#	@(#)Makefile 1.32 91/03/11 Copyright(c) Sun Microsystems, Inc.
#
#	Makefile file for  sundiag(Sun System Diagnostics).
#	Note: "SUNPRO" version of the make* is required.
#

.DEFAULT:
	sccs get -G$@ $@

REV=NEW
##### Some code will compile only on new revs of the SunOS. specify "REV="
##### if built on older versions(before 4.0.3).

DESTDIR	=
INCDIR	= ../include
ONLINEDIR = ../../lib
ARCH	= `arch`

PROGRAM = sundiag
PROGRAM2= dispatcher
PROGRAM3= sundiagup
PROGRAM4= remote
PROGRAM5= sd_rexd
PROGRAM6= diagscrnprnt
SOURCES = sundiag.c panel.c panelprocs.c data.c cf.c tests.c perfmon.c status.c\
	  rpcprocs.c option.c opfile.c tty.c ttyprocs.c msq.c eeprom.c\
	  schedule.c cpuenable.c sundiag_msg.c batch.c
SOURCE2 = $(PROGRAM2).c
SOURCE3 = $(PROGRAM3).c
SOURCE4 = $(PROGRAM4).c
SOURCE5 = sd_rexd.c mount_nfs.c unix_login.c
SOURCE6 = $(PROGRAM6).c

INCLUDES= sundiag.h procs.h struct.h tty.h sundiag_msg.h $(INCDIR)/testnames.h\
	  $(ONLINEDIR)/include/libonline.h
ICONS   = sundiag.icon failed.icon
LIBS    = -lsuntool -lsunwindow -lpixrect -lcurses -ltermcap -lrpcsvc
ONLINELIB = $(ONLINEDIR)/libonline.a

# specify DBX=-g for dbx version
DBX	= -O

CFLAGS	= $(DBX) -D$(REV) -D$(ARCH) -I$(INCDIR)
LDFLAGS = $(DBX)
LINTFLAGS = -D$(ARCH) -I$(INCDIR)

OBJECTS = $(SOURCES:.c=.o)
LINTFILES = $(SOURCES:.c=.ln)

.KEEP_STATE:

##### beginning of dependency lines #####

all: $(INCLUDES) $(ICONS) $(PROGRAM) $(PROGRAM2) $(PROGRAM3) $(PROGRAM4) \
     $(PROGRAM5) $(PROGRAM6)

$(PROGRAM): $(OBJECTS)
	cc $(LDFLAGS) -o $@ $(OBJECTS) $(LIBS) $(ONLINELIB)
# 	$(LINK.c) -o $@ $(OBJECTS) $(LIBS) $(ONLINELIB)	# 4.0 only

$(PROGRAM2): $(PROGRAM2).o
	cc $(LDFLAGS) -o $@ $(PROGRAM2).o
#	$(LINK.c) -o $@ $(PROGRAM2).o			# 4.0 only

$(PROGRAM3): $(PROGRAM3).o
	cc $(LDFLAGS) -o $@ $(PROGRAM3).o
#	$(LINK.c) -o $@ $(PROGRAM3).o			# 4.0 only

$(PROGRAM4): $(PROGRAM4).o
	cc $(LDFLAGS) -o $@ $(PROGRAM4).o -lrpcsvc

$(PROGRAM5): $$@.o mount_nfs.o unix_login.o
	cc $(LDFLAGS) -o $@ $@.o mount_nfs.o unix_login.o -lrpcsvc

$(PROGRAM6): $(PROGRAM6).o
	cc $(LDFLAGS) -o $@ $(PROGRAM6).o
#	$(LINK.c) -o $@ $(PROGRAM6).o			# 4.0 only

src: $(INCLUDES) $(ICONS) $(SOURCES) $(SOURCE2) $(SOURCE3) $(SOURCE4) \
     $(SOURCE5)

install: all FRC
	@if [ $(DESTDIR) ]; then \
	  set -x; \
	  install -s $(PROGRAM) $(DESTDIR); \
	  install -s $(PROGRAM2) $(DESTDIR); \
	  install -s $(PROGRAM3) $(DESTDIR); \
	  install -s $(PROGRAM4) $(DESTDIR); \
	  install -s $(PROGRAM5) $(DESTDIR); \
	  install -s $(PROGRAM6) $(DESTDIR); \
	else \
	  set -x; \
	  install $(PROGRAM) ../bin; \
	  install $(PROGRAM2) ../bin; \
	  install $(PROGRAM3) ../bin; \
	  install $(PROGRAM4) ../bin; \
	  install $(PROGRAM5) ../bin; \
	  install $(PROGRAM6) ../bin; \
	fi

clean:
	rm -f $(PROGRAM) $(PROGRAM2) $(PROGRAM3) $(PROGRAM4) $(PROGRAM5) \
	      $(PROGRAM6) $(OBJECTS) \
	      $(PROGRAM2).o $(PROGRAM3).o $(PROGRAM4).o $(PROGRAM5).o \
	      $(PROGRAM6).o unix_login.o mount_nfs.o \
	      $(LINTFILES) $(PROGRAM2).ln $(PROGRAM3).ln $(PROGRAM4).ln \
	      $(PROGRAM5).ln $(PROGRAM6).ln core

lint: $(LINTFILES) $(PROGRAM2).ln $(PROGRAM3).ln $(PROGRAM4).ln $(PROGRAM5).ln \
        $(PROGRAM6).ln
	lint $(LINTFLAGS) $(LINTFILES) $(LIBS)
#	$(LINT.c) $(LINTFILES) $(LIBS)			# 4.0 only
	lint $(LINTFLAGS) $(PROGRAM2).ln
#	$(LINT.c) $(PROGRAM2).ln			# 4.0 only
	lint $(LINTFLAGS) $(PROGRAM3).ln
#	$(LINT.c) $(PROGRAM3).ln			# 4.0 only
	lint $(LINTFLAGS) $(PROGRAM4).ln
#	$(LINT.c) $(PROGRAM4).ln			# 4.0 only
	lint $(LINTFLAGS) $(PROGRAM5).ln
	lint $(LINTFLAGS) $(PROGRAM6).ln

info: FRC
	sccs info

FRC:

