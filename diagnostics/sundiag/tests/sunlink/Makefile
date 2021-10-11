#	Makefile for sunlink.
#
#	@(#)Makefile 1.1 92/07/30 SMI;
#

.DEFAULT:
	sccs get -G$@ $@
	
include ../../include/Makefile.macros
PROGRAM=	sunlink
DESTDIR=
INCLUDES = syncmode.h syncstat.h hsparam.h $(INCDIR)/sdrtns.h\
	$(ONLINEINC)/libonline.h
LIBS    =

SOURCES = sunlink.c 
OBJECTS = $(SOURCES:.c=.o)
LINTFILES = $(SOURCES:.c=.ln)

DBX=-O
# specify DBX=-g for dbx version

CFLAGS= $(DBX) -I${INCDIR} -D`arch`
LDFLAGS = $(DBX)
LINTFLAGS= -I$(INCDIR)

.KEEP_STATE:

all:	${INCLUDES} ${PROGRAM}

$(PROGRAM): $(OBJECTS)
	${CC} $(LDFLAGS) -o $@ $(OBJECTS) $(SDLIBS) $(LIBS)

install: all
	@if [ $(DESTDIR) ]; then \
	  set -x; \
	  install -s sunlink ${DESTDIR}; \
	else \
	  set -x; \
	  install sunlink ../../bin; \
	fi

clean:
	rm -f ${PROGRAM} $(OBJECTS) $(LINTFILES) core

lint: $(LINTFILES)
	lint $(LINTFLAGS) $(LINTFILES)

info: FRC
	sccs info

include ../../include/Makefile.end
