#
#	@(#)Makefile 1.1 92/07/30 Copyright(c) Sun Microsystems, Inc.
#
#       Makefile for Sundiag newtest test template.
#
.DEFAULT:
	sccs get -G$@ $@

include ../../include/Makefile.macros
REV=NEW
# specify "REV=" if built for 4.0.X

DBX=-O
# specify DBX=-g for dbx version

DESTDIR =
PROGRAM = tapetest 
INCDIR	= ../../include
INCLUDES= tapetest.h tapetest_msg.h $(INCDIR)/sdrtns.h $(ONLINEINC)/libonline.h
LIBS	=

SOURCES	= tapetest.c recon.c tapetest_msg.c
OBJECTS	= $(SOURCES:.c=.o)
LINTFILES = $(SOURCES:.c=.ln)

CFLAGS	= $(DBX) -D$(REV) -D`arch` -I$(INCDIR)
LDFLAGS	= $(DBX)
LINTFLAGS= -D$(REV) -I$(INCDIR)

.KEEP_STATE:

##### beginning of dependency lines #####

all:	$(INCLUDES) $(PROGRAM)

$(PROGRAM): $(OBJECTS)
	cc $(LDFLAGS) -o $@ $(OBJECTS) $(SDLIBS) $(LIBS)

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
	lint $(LINTFLAGS) $(LINTFILES)

info: FRC
	sccs info

include ../../include/Makefile.end
