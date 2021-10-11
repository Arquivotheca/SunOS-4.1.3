#
#	@(#)Makefile 1.1 92/07/30 Copyright(c) Sun Microsystems, Inc.
#

.DEFAULT:
	sccs get -G$@ $@

include ../../include/Makefile.macros
DBX=-O
# specify DBX=-g for dbx version

DESTDIR =
PROGRAM = ipctest pctool-listener
INCLUDES= ipctest.h pc.bat $(INCDIR)/sdrtns.h $(ONLINEINC)/libonline.h
LIBS	=

SOURCES	= ipctest.c pctool-listener.c
OBJECTS	= $(SOURCES:.c=.o)
LINTFILES = $(SOURCES:.c=.ln)

CFLAGS	= $(DBX) -D`arch` -I$(INCDIR)
LDFLAGS	= $(DBX)
LINTFLAGS= -D`arch` -I$(INCDIR)

.KEEP_STATE:

##### beginning of dependency lines #####

all:	$(INCLUDES) $(PROGRAM)

$(PROGRAM): $$@.o
	cc $(LDFLAGS) -o $@ $@.o $(SDLIBS) $(LIBS)

install: all FRC
	@if [ $(DESTDIR) ]; then \
	  set -x; \
	  install -s $(PROGRAM) pc.bat $(DESTDIR); \
	else \
	  set -x; \
	  install $(PROGRAM) pc.bat ../../bin; \
	fi
		
clean: FRC
	rm -f $(PROGRAM) $(OBJECTS) $(LINTFILES) core

lint: $(LINTFILES)
	lint $(LINTFLAGS) $(LINTFILES) $(LIBS)

info: FRC
	sccs info
include ../../include/Makefile.end
