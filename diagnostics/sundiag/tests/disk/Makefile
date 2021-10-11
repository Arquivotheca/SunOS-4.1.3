#
#	@(#)Makefile 1.1 92/07/30 Copyright(c) Sun Microsystems, Inc.
#

.DEFAULT:
	sccs get -G$@ $@

include ../../include/Makefile.macros

OS=SVR4
#specify "OS=" for 4.x builds

ARCH=sun4
#This is for 5.0DR only since 'arch' is not available

DBX=-O
# specify DBX=-g for dbx version

DESTDIR =
PROGRAM1= fstest
PROGRAM2= rawtest
INCLUDES= fstest.h fstest_msg.h rawtest.h rawtest_msg.h\
		$(INCDIR)/sdrtns.h $(ONLINEINC)/libonline.h
LIBS	=

SOURCES1= fstest.c fstest_msg.c
SOURCES2= rawtest.c rawtest_msg.c
OBJECTS1= $(SOURCES1:.c=.o)
OBJECTS2= $(SOURCES2:.c=.o)
LINTFILES = $(SOURCES1:.c=.ln) $(SOURCES2:.c=.ln)

CFLAGS	= $(DBX) -D`arch` -I$(INCDIR)
SVR4CFLAGS= $(DBX) -D$(ARCH) -D$(OS) -I$(INCDIR)
LDFLAGS	= $(DBX) -D$(DEBUG)
LINTFLAGS= -D`arch` -I$(INCDIR)

NSL = -lnsl
#This is for 5.0DR only

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

all:	$(INCLUDES) $(PROGRAM1) $(PROGRAM2)

$(PROGRAM1): $(OBJECTS1)
	@if [ $(OS) ]; then \
	    set -x;\
	    cc $(LDFLAGS) -o $@ $(OBJECTS1) $(SDLIBS) $(LIBS) $(NSL);\
	else \
	    set -x;\
	    cc $(LDFLAGS) -o $@ $(OBJECTS1) $(SDLIBS) $(LIBS) ;\
	fi

$(PROGRAM2): $(OBJECTS2)
	@if [ $(OS) ]; then \
	    set -x;\
	    cc $(LDFLAGS) -o $@ $(OBJECTS2) $(SDLIBS) $(LIBS) $(NSL); \
	else \
	    set -x;\
	    cc $(LDFLAGS) -o $@ $(OBJECTS2) $(SDLIBS) $(LIBS); \
	fi

install: all FRC
	@if [ $(DESTDIR) ]; then \
	  set -x; \
	  install -s $(PROGRAM1) $(PROGRAM2) $(DESTDIR); \
	else \
	  set -x; \
	  install $(PROGRAM1) $(PROGRAM2) ../../bin; \
	fi
		
clean: FRC
	rm -f $(PROGRAM1) $(PROGRAM2) $(OBJECTS1) $(OBJECTS2) \
	$(OBJECTS3) $(LINTFILES) core

lint: $(LINTFILES)
	lint $(LINTFLAGS) $(LINTFILES) $(LIBS)

info: FRC
	sccs info
include ../../include/Makefile.end

