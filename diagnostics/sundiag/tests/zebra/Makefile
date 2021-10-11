#
#	@(#)Makefile 1.1 92/07/30 Copyright(c) Sun Microsystems, Inc.
#

.DEFAULT:
	sccs get -G$@ $@

include ../../include/Makefile.macros

DBX=-g
# specify DBX=-g for dbx version

DESTDIR =
PROGRAM1= lpvitest
PROGRAM2= bpptest
INCLUDES= zebra.h zebra_msg.h bpp_io.h lpviio.h\
		$(INCDIR)/sdrtns.h $(ONLINEINC)/libonline.h
LIBS	=-lpixrect

SOURCES1= lpvi.c zebra_msg.c
SOURCES2= bpp.c zebra_msg.c
OBJECTS1= $(SOURCES1:.c=.o)
OBJECTS2= $(SOURCES2:.c=.o)
LINTFILES = $(SOURCES1:.c=.ln) $(SOURCES2:.c=.ln)

CFLAGS	= $(DBX) -D`arch` -I$(INCDIR)
LDFLAGS	= $(DBX)
LINTFLAGS= -D`arch` -I$(INCDIR)

IMAGE1= 57fonts.300
IMAGE2= 57fonts.400
RAS_IMAGES= $(IMAGE1) $(IMAGE2)

.KEEP_STATE:

##### beginning of dependency lines #####

all:	$(INCLUDES) $(PROGRAM1) $(PROGRAM2) 

$(PROGRAM1): $(OBJECTS1)
	cc $(LDFLAGS) -o $@ $(OBJECTS1) $(SDLIBS) $(LIBS)
$(PROGRAM2): $(OBJECTS2)
	cc $(LDFLAGS) -o $@ $(OBJECTS2) $(SDLIBS) $(LIBS)

install: all $(RAS_IMAGES) FRC
	@if [ $(DESTDIR) ]; then \
	  set -x; \
	  install -s $(PROGRAM1) $(PROGRAM2) $(DESTDIR); \
  	  install $(RAS_IMAGES) $(DESTDIR); \
	else \
	  set -x; \
	  install $(PROGRAM1) $(PROGRAM2) $(RAS_IMAGES) ../../bin; \
	fi
		
clean: FRC
	rm -f $(PROGRAM1) $(PROGRAM2) $(OBJECTS1) $(OBJECTS2) \
	$(LINTFILES) core

lint: $(LINTFILES)
	lint $(LINTFLAGS) $(LINTFILES) $(LIBS)

info: FRC
	sccs info
include ../../include/Makefile.end

