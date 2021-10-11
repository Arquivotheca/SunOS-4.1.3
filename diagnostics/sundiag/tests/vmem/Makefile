#
#	@(#)Makefile 1.1 92/07/30 Copyright(c) Sun Microsystems, Inc.
#

.DEFAULT:
	sccs get -G$@ $@

include ../../include/Makefile.macros

OS=SVR4
#specify "OS=" if built for 4.x

REV=NEW
# specify "REV=" if built for 4.0.X

DBX=-O
# specify DBX=-g for dbx version

DESTDIR =
ARCH = sun4
#this is only for 5.0 since 'arch' is not available

PROGRAM = vmem
INCLUDES= vmem.h vmem_msg.h $(ONLINEINC)/libonline.h $(INCDIR)/sdrtns.h

NETLIB = -lsocket -lnsl /usr/lib/ld.so.1

LIBS	= -lkvm
SVR4LIBS = ../../lib/libkvm.a -lelf

SOURCES	= vmem.c vmem_msg.c
OBJECTS	= $(SOURCES:.c=.o)
LINTFILES = $(SOURCES:.c=.ln)

BSDCFLAGS	= $(DBX) -D$(REV) -D`arch` -I$(INCDIR)
SVR4CFLAGS = -dy $(DBX) -D$(REV) -D$(OS) -D$(ARCH) -I$(INCDIR)
LDFLAGS	= $(DBX)
LINTFLAGS= -D$(REV) -D`arch` -I$(INCDIR)

.KEEP_STATE:

##### beginning of dependency lines #####

all:	$(INCLUDES) $(PROGRAM)

.c.o:
	@if [ $(OS) ]; then \
	  set -x; \
	  cc -c $(SVR4CFLAGS) $< ; \
	else \
	  set -x; \
	  cc -c $(BSDCFLAGS) $< ; \
	fi

$(PROGRAM): $(OBJECTS)
	@if [ $(OS) ]; then \
	  set -x; \
	  cc $(LDFLAGS) -o $@ $(OBJECTS) $(SDLIBS) $(NETLIB) $(SVR4LIBS) ; \
	else \
	  set -x; \
	  cc $(LDFLAGS) -o $@ $(OBJECTS) $(SDLIBS) $(LIBS) ; \
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
