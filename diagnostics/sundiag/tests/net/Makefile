
#
#       @(#)Makefile 1.1 92/07/30 SMI;
#
#       Makefile for Sundiag newtest test template.
#
# This is the default rule make will use in case all others fail
#
.DEFAULT:
	sccs get -G$@ $@

#
# Get the Sundiag specific macros
#
include ../../include/Makefile.macros

#
# Define the SunOS release number, default is 4.0.  If you want to build the
# program under a different OS release redefine this macro or do it on the
# command line.  ie. specify "OS=" if built for 3.x
#
#OS=_4.0 
#disable this since it will be used for 5.0 merge 10/11/90
#

OS=SVR4

ARCH=sun4
#This is for SVR4 only

#specify "OS=" for 4.x builds
#
# specify DBX=-g for dbx version
#
DBX=-O

#
# No destination directory for installs
#
DESTDIR =
NETTEST = nettest 

#
# The standard include files.
#
INCL= commands.h fddi_types.h $(INCDIR)/sdrtns.h $(ONLINEINC)/libonline.h

#
# Test specific include file defintion
#
NETTEST_INCL = if_trvar.h nettest.h nettest_spray.h entest.h $(INCL)

LIBS	= -lrpcsvc 

NETTEST_SRC = nettest.c nettest_probe.c nettest_stat.c nettest_spray.c nettest_udp.c entest.c
NETTEST_OBJ = $(NETTEST_SRC:.c=.o)
NETTEST_LINT = $(NETTEST_SRC:.c=.ln)

CFLAGS	= $(DBX) -D`arch` -I$(INCDIR)
SVR4CFLAGS = $(DBX) -D$(OS) -D$(ARCH) -I$(INCDIR)


LDFLAGS	= $(DBX)
LINTFLAGS= -D$(OS)

.KEEP_STATE:

##### beginning of dependency lines #####

all:	$(NETTEST_INCL) $(NETTEST) 

$(NETTEST): $(NETTEST_OBJ)
	@if [ $(OS) ]; then \
		set -x ;\
		cc $(LDFLAGS) -o $@ $(NETTEST_OBJ) $(SDLIBS) $(LIBS) ../../lib/libkvm.a -lelf; \
	else \
		set -x ;\
		cc $(LDFLAGS) -o $@ $(NETTEST_OBJ) $(SDLIBS) $(LIBS) -lkvm ; \
	fi

install: all FRC
	@if [ $(DESTDIR) ]; then \
	  set -x; \
	  install -s $(NETTEST) $(DESTDIR); \
	else \
	  set -x; \
	  install $(NETTEST) ../../bin; \
	fi
		
clean: FRC
	rm -f $(NETTEST) $(NETTEST_OBJ) $(NETTEST_LINT) core 

lint: $(NETTEST_LINT)
	lint $(LINTFLAGS) $(NETTEST_LINT)

info: FRC
	sccs info
include ../../include/Makefile.end

