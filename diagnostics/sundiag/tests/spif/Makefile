#
#	@(#)Makefile 1.1 7/30/92 Copyright 1989 Sun Microsystems Inc.
#

.DEFAULT:
	sccs get -G$@ $@

include ../../include/Makefile.macros

#REV=
# specify "REV=" if built for 4.0.X

DBX=-g
# specify DBX=-g for dbx version

DESTDIR =
PROGRAM = spiftest
RPROG	= sp_read
WPROG	= sp_write
INCLUDES= spiftest.h spif_dev.h spif_msg.h stcio.h stcreg.h \
	$(INCDIR)/sdrtns.h $(ONLINEINC)/libonline.h
LIBS	=

SOURCES	= spiftest.c spif_msg.c
RSRC 	= sp_read.c
WSRC	= sp_write.c
OBJECTS	= $(SOURCES:.c=.o)
ROBJ	= $(RSRC:.c=.o)
WOBJ	= $(WSRC:.c=.o)
LINTFILES = $(SOURCES:.c=.ln) $(RSRC:.c=.ln) $(WSRC:.c=.ln)

#CFLAGS	= $(DBX) -DHWBUG -D`arch` -I$(INCDIR)
CFLAGS	= $(DBX) -D`arch` -I$(INCDIR)
LDFLAGS	= $(DBX)
#LINTFLAGS= -D$(REV) -D`arch` -I$(INCDIR)
LINTFLAGS= -D`arch` -I$(INCDIR)

.KEEP_STATE:

##### beginning of dependency lines #####

all:	$(INCLUDES) $(PROGRAM)

$(PROGRAM): $(OBJECTS)
	cc $(LDFLAGS) -o $@ $(OBJECTS) $(SDLIBS) $(LIBS)

$(RPROG): $(ROBJ)
	cc $(LDFLAGS) -o $@ $(ROBJ) $(SDLIBS) $(LIBS)

$(WPROG): $(WOBJ)
	cc $(LDFLAGS) -o $@ $(WOBJ) $(SDLIBS) $(LIBS)

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
	rm -f $(RPROG) $(ROBJ) $(LINTFILES) core
	rm -f $(WPROG) $(WOBJ) $(LINTFILES) core

lint: $(LINTFILES)
	lint $(LINTFLAGS) $(LINTFILES) $(LIBS)

info: FRC
	sccs info

include ../../include/Makefile.end
