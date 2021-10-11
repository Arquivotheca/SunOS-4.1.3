#
#	@(#)Makefile 1.1 92/07/30 Copyright (c) Sun Microsystems, Inc.
#
.DEFAULT:
	sccs get -G$@ $@

include ../../include/Makefile.macros

OS=SVR4
#specify "OS=" for 4.x builds

REV=NEW
# specify "REV=" if built for 4.0.X

# specify DBX=-g for dbx version
DBX=-O

DESTDIR =
PROGRAM = cdtest
ARCH	= sun4
CFLAGS		= $(DBX) -D`arch` -D$(REV) -I$(INCDIR)
SVR4CFLAGS	= $(DBX) -D$(OS) -D$(ARCH) -D$(REV) -I$(INCDIR)
LDFLAGS	= $(DBX)
LINTFLAGS = -D$(REV)
LIBS	=
SVR4LIBS = -lnsl

INCLUDES= cd.h srdef.h $(INCDIR)/sdrtns.h $(ONLINEDIR)/include/libonline.h
SOURCES	= cdtest.c

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

all: $(INCLUDES) $(PROGRAM)

$(PROGRAM): $(OBJECTS) $(TESTLIB)
	@if [ $(OS) ]; then \
	    set -x ; \
	    cc $(LDFLAGS) -o $@ $(OBJECTS) $(SDLIBS) $(SVR4LIBS); \
	else \
	    set -x ; \
	    cc $(LDFLAGS) -o $@ $(OBJECTS) $(SDLIBS) $(LIBS); \
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
	rm -f $(PROGRAM) $(OBJECTS) $(LINTFILES) core

lint: $(LINTFILES)
	lint $(LINTFLAGS) $(LINTFILES) $(LIBS)

info: FRC
	sccs info

include ../../include/Makefile.end


