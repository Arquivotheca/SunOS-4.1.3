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

DBX=-g
# specify DBX=-g for dbx version

DESTDIR =
PROGRAM = mptest
INCLUDES= mptest.h mptest_msg.h $(ONLINEINC)/libonline.h\
	$(INCDIR)/sdrtns.h

NETLIB = -lsocket -lnsl /usr/lib/ld.so.1

LIBS	= -lkvm	-lm
SVR4LIBS = ../../lib/libkvm.a -lelf

FPUOBJS=S.fpu.o D.fpu.o
SOURCES	= mptest.c mptest_msg.c mputil.c dataio.c cache.c mplock.c
OBJECTS	= $(SOURCES:.c=.o) 
LINTFILES = $(SOURCES:.c=.ln)

BSDCFLAGS	= $(DBX) -D`arch` -I$(INCDIR)
SVR4CFLAGS = -dy $(DBX) -D$(OS) -D$(ARCH) -I$(INCDIR)
LDFLAGS	= $(DBX) -L$(ONLINEDIR) -L$(TESTDIR)
LINTFLAGS= -D`arch` -I$(INCDIR)

.KEEP_STATE:

##### beginning of dependency lines #####

all:	$(PROGRAM)

ldstublock.o : ldstublock.s
	as -o ldstublock.o ldstublock.s

S.fpu.o: fpu.c
	${CC} ${BSDCFLAGS} -c fpu.c;
	mv fpu.o S.fpu.o

D.fpu.o: fpu.c
	${CC} ${BSDCFLAGS} -DDP -c fpu.c;
	mv fpu.o D.fpu.o

.c.o:
	@if [ $(OS) ]; then \
	   set -x; \
	   cc -c $(SVR4CFLAGS) $< ; \
	else \
	   set -x; \
	   cc -c $(BSDCFLAGS) $< ; \
	fi


$(PROGRAM): $(INCLUDES) $(OBJECTS) $(FPUOBJS) ldstublock.o
	@if [ $(OS) ] ; then \
	    set -x ; \
	    cc $(LDFLAGS) -o $@ $(OBJECTS) $(FPUOBJS) ldstublock.o $(SDLIBS) $(NETLIB) $(SVR4LIBS) ; \
	else \
	    set -x ; \
	    cc $(LDFLAGS) -o $@ $(OBJECTS) $(FPUOBJS) ldstublock.o $(SDLIBS) $(LIBS) ; \
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
	rm -f $(PROGRAM) $(OBJECTS) $(LINTFILES) ldstublock.o core S.fpu.o D.fpu.o

lint: $(LINTFILES)
	lint $(LINTFLAGS) $(LINTFILES) $(LIBS)

info: FRC
	sccs info

include ../../include/Makefile.end
