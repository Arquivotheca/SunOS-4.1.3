#
#	@(#)Makefile 1.1 92/07/30 Copyright(c) Sun Microsystems, Inc.
#

#       The makefile covers new environment(sunview & openlool)
#       and OS(SVR4 and BSD(4.x)). Two make flags are introduced
#       WIN and OS. Their default values are:
#               WIN= OL  (openlook)
#               OS = SVR4(5.0DR)
#
#       invoke make as followed :
#
#       ---------------------------------------------------
#             \OS|                        |
#       window \ |   SVR4(5.0DR)          |     4.x
#       ---------+------------------------+----------------
#       sunview  |  make WIN= (error)     | make OS= WIN=
#       ---------+------------------------+----------------
#       openlook |  make (default)        | make OS=
#       ---------------------------------------------------
#
#       note: (1)`make WIN=` tries to make a sunview version of test
#                under 5.0DR. This is an error condition.
#             (2)`make OS= WIN=` will make sunview version of test
#                under 4.x. The target produced is mono.
#             (3)`make` will make openlook version of test for 5.0DR.
#                The target produced is mono.ol
#             (4)`make OS=` will make openlook version of test for 4.x.
#                The target produced is mono.ol.
#

.SUFFIXES: .ol.o .ol $(SUFFIXES)

.DEFAULT:
	sccs get -G$@ $@

include ../../include/Makefile.macros

DBX=-g
# specify DBX=-g for dbx version

OS=SVR4
#specify "OS=" for 4.x builds

WIN=OL
#specify "WIN=" for sunview test builds

ARCH=sun4
#This is for 5.0DR only since 'arch' is not available

DESTDIR =
PROGRAM = color
COLOR_OL= color.ol

OLINCDIR= $(OPENWINHOME)/include
INCLUDES= $(INCDIR)/sdrtns.h $(ONLINEINC)/libonline.h
UCBLIB  = /usr/ucblib
4XLIBS	= -lsuntool -lsunwindow -lpixrect
SVR4LIBS= -lsocket $(UCBLIB)/librpcsoc.so -lX11 -lnsl /usr/lib/ld.so.1
OLLIBS = -L$(OPENWINHOME)/lib -lpixrect -sun4 -lX11

SOURCES	= color.c
OBJECTS	= $(SOURCES:.c=.o)
OLOBJECTS= $(SOURCES:.c=.ol.o)
LINTFILES = $(SOURCES:.c=.ln)

CFLAGS	= $(DBX) -D$(OS) -D`arch` -I$(INCDIR)
SVR4CFLAGS = $(DBX) -D$(ARCH) -D$(OS) -I$(INCDIR)

LDFLAGS	= $(DBX)
LINTFLAGS= -D`arch` -I$(INCDIR)

.KEEP_STATE:

##### beginning of dependency lines #####

.c.o:
	@if [ $(OS) ] ; then \
	    set -x ; \
	    echo "error, no sunview test under SVR4"; \
	    exit ; \
	else \
	    set -x ;\
	    cc -c $(CFLAGS) -I$(INCDIR) $< ; \
	fi

.c.ol.o:
	@if [ $(OS) ]; then \
	    set -x ;\
	    cc -c -o $(*).ol.o -D$(WIN) $(SVR4CFLAGS) -I$(OLINCDIR) $< ; \
	else \
	    set -x ;\
	    cc -c -o $(*).ol.o -D$(WIN) $(CFLAGS) -I$(OLINCDIR) $< ; \
	fi


all:	$(INCLUDES) $(PROGRAM)$(WIN:%=.ol)

$(PROGRAM): $(OBJECTS)
	@if [ $(OS) ]; then \
	    echo "error, no sunview under 5.0DR"; \
	    exit ; \
	else \
	    set -x ; \
	    cc $(LDFLAGS) -o $@ $(OBJECTS) $(SDLIBS) $(4XLIBS); \
	fi

$(COLOR_OL): $(OLOBJECTS)
	@if [ $(OS) ]; then \
		cc $(LDFLAGS) -o $@ $(OLOBJECTS) $(SDLIBS) $(SVR4LIBS); \
	else \
		cc $(LDFLAGS) -o $@ $(OLOBJECTS) $(SDLIBS) $(OLLIBS); \
	fi


install: all FRC
	@if [ $(DESTDIR) ]; then \
	  set -x; \
	  install -s $(PROGRAM) $(DESTDIR); \
	else \
	  set -x; \
	  install $(PROGRAM) ../../bin; \
	fi
		
clean: FRC
	rm -f $(PROGRAM) $(OBJECTS) $(OLOBJECTS) $(LINTFILES) core

lint: $(LINTFILES)
	lint $(LINTFLAGS) $(LINTFILES) $(LIBS)

info: FRC
	sccs info

include ../../include/Makefile.end
