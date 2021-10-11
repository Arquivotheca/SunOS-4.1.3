#
#	@(#)Makefile 1.1 92/07/30 Copyright(c) Sun Microsystems, Inc.
#

.DEFAULT:
	sccs get -G$@ $@

include ../../include/Makefile.macros

OS=SVR4
#specify "OS=" for 4.x bbuilds

ARCH=sun4
# this is for 5.0(SVR4) only since arch is not available on 5.0

DBX=-O
# specify DBX=-g for dbx version

DESTDIR =
PROGRAM = pmem
INCLUDES= pmem.h pmem_msg.h openprom.h $(ONLINEINC)/libonline.h\
	$(INCDIR)/sdrtns.h

NETLIB = -lsocket -lnsl /usr/lib/ld.so.1

LIBS	= -lkvm	-lm
SVR4LIBS = ../../lib/libkvm.a -lelf

SOURCES	= pmem.c pmem_msg.c
OBJECTS	= $(SOURCES:.c=.o)
LINTFILES = $(SOURCES:.c=.ln)

BSDCFLAGS	= $(DBX) -D`arch` -I$(INCDIR)
SVR4CFLAGS = -dy $(DBX) -D$(OS) -D$(ARCH) -I$(INCDIR)
LDFLAGS	= $(DBX) -L$(ONLINEDIR) -L$(TESTDIR)
LINTFLAGS= -D`arch` -I$(INCDIR)

.KEEP_STATE:

##### beginning of dependency lines #####

all:	$(PROGRAM)

.c.o:
	@if [ $(OS) ]; then \
	   set -x; \
	   cc -c $(SVR4CFLAGS) $< ; \
	else \
	   set -x; \
	   cc -c $(BSDCFLAGS) $< ; \
	fi


$(PROGRAM): $(INCLUDES) $(OBJECTS)
	@if [ $(OS) ] ; then \
	    set -x ; \
	    cc $(LDFLAGS) -o $@ $(OBJECTS) $(SDLIBS) $(NETLIB) $(SVR4LIBS) ; \
	else \
	    set -x ; \
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
