#
#	@(#)Makefile 1.1 92/07/30 Copyright(c) Sun Microsystems, Inc.
#
#	The makefile covers new environment(sunview & openlool)
#	and OS(SVR4 and BSD(4.x)). Two make flags are introduced
#	WIN and OS. Their default values are:
#		WIN= OL  (openlook)
#		OS = SVR4(5.0DR)
#
#	invoke make as followed :
#
#	---------------------------------------------------
#	      \OS|   			  |
#	window \ |   SVR4(5.0DR)	  |	4.x
#	---------+------------------------+----------------
#	sunview  |  make WIN= (error)	  | make OS= WIN=
#	---------+------------------------+----------------
#	openlook |  make (default)	  | make OS=
#	---------------------------------------------------
#
#	note: (1)`make WIN=` tries to make a sunview version of test
#		 under 5.0DR. This is an error condition.
#	      (2)`make OS= WIN=` will make sunview version of test 
#		 under 4.x. The target produced is mono.
#	      (3)`make` will make openlook version of test for 5.0DR.
#		 The target produced is mono.ol
#	      (4)`make OS=` will make openlook version of test for 4.x.
#		 The target produced is mono.ol.
#

.SUFFIXES: .ol.o .ol $(SUFFIXES)
#	.ol.o is for the .o equivalent under openlook. Under 4.x, it is 
#	possible to `make OS=` and also `make OS= WIN=`. If both make 
#	are performed, there will be two target mono and mono.ol. 
#	Mono is to run under sunview and mono.ol is to run under openlook.
#	There will also be mono.o and mono.ol.o as object files for each 
#	source(.c) files.
#
#	.ol is for target mono.ol so make would not tries to sccs get 
#	s.mono.ol
#

.DEFAULT:
	sccs get -G$@ $@

include ../../include/Makefile.macros
DBX=-O
# specify DBX=-g for dbx version

OS=SVR4
#specify "OS=" for 4.x builds

WIN=OL
#specify "WIN=" for sunview builds

ARCH = sun4
#This is for 5.0DR only since 'arch' is not available

DESTDIR =
PROGRAM = mono
MONO_OL = mono.ol
OLINCDIR = $(OPENWINHOME)/include
INCLUDES= mono_msg.h $(INCDIR)/sdrtns.h $(ONLINEINC)/libonline.h

SUNVIEWLIBS	= -lsuntool -lsunwindow -lpixrect
#Sunview library

OLLIBS   = -L/usr/local/openwin/lib -lX11
#openlook specific library

SVR4LIBS = -lnsl /usr/lib/ld.so.1
#SVR4 specific librarys

SOURCES	= mono.c mono_msg.c

OBJECTS	= $(SOURCES:.c=.o)
OLOBJECTS= $(SOURCES:.c=.ol.o)
LINTFILES = $(SOURCES:.c=.ln)

CFLAGS  = $(DBX) -D`arch` -I$(INCDIR)
SVR4CFLAGS = $(DBX) -D$(ARCH) -D$(OS) -I$(INCDIR)

LDFLAGS	= $(DBX)
LINTFLAGS= -D`arch` -I$(INCDIR)

.KEEP_STATE:

##### beginning of dependency lines #####

all : $(INCLUDES) $(PROGRAM)$(WIN:%=.ol)

$(PROGRAM) : $(OBJECTS)
	cc $(LDFLAGS) -o $@ $(OBJECTS) $(SDLIBS) $(SUNVIEWLIBS)

$(MONO_OL): $(OLOBJECTS)
	@if [ $(OS) ]; then \
	    set -x ;\
	    cc $(LDFLAGS) -o $@ $(OLOBJECTS) $(SDLIBS) $(OLLIBS) $(SVR4LIBS) ; \
	else \
	    set -x ; \
	    cc $(LDFLAGS) -o $@ $(OLOBJECTS) $(SDLIBS) $(OLLIBS) -lpixrect ; \
	fi

.c.ol.o:
	@if [ $(OS) ]; then \
	    set -x ;\
	    cc -c -o $(*).ol.o -D$(WIN) $(SVR4CFLAGS) -I$(OLINCDIR) $< ; \
	else \
	    set -x ;\
	    cc -c -o $(*).ol.o -D$(WIN) $(CFLAGS) -I$(OLINCDIR) $< ; \
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
