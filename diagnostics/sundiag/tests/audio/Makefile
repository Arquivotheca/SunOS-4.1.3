#
#	@(#)Makefile 1.1 92/07/30 Copyright (c) Sun Microsystems, Inc.
#
.DEFAULT:
	sccs get -G$@ $@

include ../../include/Makefile.macros

OS=SVR4
#specify "OS=" for all 4.x builds

# invoke make with "REV=" under 4.0.3c, NEW implies 4.1 with new audio driver
REV = NEW

# specify DBX=-g for dbx version
DBX=-O

DESTDIR =
PROGRAM = autest
DATAFILE = autest.data
ARCH	= sun4
CFLAGS		= $(DBX) -D`arch` -D$(REV) -I$(INCDIR)
SVR4CFLAGS	= $(DBX) -D$(ARCH) -D$(OS) -D$(REV) -I$(INCDIR)
LDFLAGS	= $(DBX)
LINTFLAGS =
LIBS	=

INCLUDES= $(INCDIR)/sdrtns.h $(ONLINEDIR)/include/libonline.h \
	  audioio.h audioreg.h
SOURCES	= autest.c

OBJECTS	= $(SOURCES:.c=.o)
LINTFILES = $(SOURCES:.c=.ln)

.KEEP_STATE:

##### beginning of dependency lines #####

.c.o:
	@if [ $(OS) ]; then \
	   set -x ; \
	   cc -c $(SVR4CFLAGS) $< ; \
	else \
	   set -x ; \
	   cc -c $(CFLAGS) $< ; \
	fi

all: $(INCLUDES) $(PROGRAM) $(DATAFILE)

$(PROGRAM): $(OBJECTS) $(TESTLIB)
	cc $(LDFLAGS) -o $@ $(OBJECTS) $(SDLIBS) $(LIBS)

install: all FRC
	@if [ $(DESTDIR) ]; then \
	  set -x; \
	  install -s $(PROGRAM) $(DESTDIR); \
	  install -s $(DATAFILE) $(DESTDIR); \
	else \
	  set -x; \
	  install $(PROGRAM) ../../bin; \
	  install $(DATAFILE) ../../bin; \
	fi
		
clean: FRC
	rm -f $(PROGRAM) $(OBJECTS) $(LINTFILES) core

lint: $(LINTFILES)
	lint $(LINTFLAGS) $(LINTFILES) $(LIBS)

info: FRC
	sccs info

include ../../include/Makefile.end


