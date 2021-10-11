#
#	@(#)Makefile 1.1 92/07/30 Copyright(c) Sun Microsystems, Inc.
#
.DEFAULT:
	sccs get -G$@ $@
	
include ../../include/Makefile.macros
DBX=-O
# specify DBX=-g for dbx version

OS=SVR4
DESTDIR =
PROGRAM = sptest
INCLUDES= sptest.h sptest_msg.h $(ONLINEINC)/libonline.h $(INCDIR)/sdrtns.h
LIBS	=
SVR4LIBS	=-lnsl

SOURCES	= sptest.c sptest_msg.c
OBJECTS	= $(SOURCES:.c=.o)
LINTFILES = $(SOURCES:.c=.ln)

CFLAGS	= $(DBX) -D`arch` -D$(OS) -I$(INCDIR) -DBSD_COMP
LDFLAGS	= $(DBX) -D$(OS)
LINTFLAGS= -D`arch` -I$(INCDIR)

.KEEP_STATE:

##### beginning of dependency lines #####

all:	$(INCLUDES) $(PROGRAM)

$(PROGRAM): $(OBJECTS)
	@if [ $(OS) ]; then \
	  set -x; \
	  cc $(LDFLAGS) -o $@ $(OBJECTS) $(SDLIBS) $(LIBS) $(SVR4LIBS) ; \
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
