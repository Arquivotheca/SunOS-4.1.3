#
#       @(#)Makefile	1.1 7/30/92  SMI
#
.DEFAULT:
	sccs get -G$@ $@

INCDIR	= ../include
ONLINEDIR = ../../lib
INCLUDES = rpcrtns.h $(INCDIR)/sdrtns.h $(ONLINEDIR)/include/libonline.h
ARCH	= sun4

OS = SVR4
#specify "OS=" for 4.x builds

SRCS	= sdrtns.c rpcrtns.c sdutil.c
OBJS	= $(SRCS:.c=.o)
LINTFILES = $(SRCS:.c=.ln)

RTNSLIB	= libtest.a
ONLINELIB = $(ONLINEDIR)/libonline.a

DBX	=
# = -g for dbx version
CFLAGS	= $(DBX) -D`arch` -I$(INCDIR) $(ONLINELIB)
SVR4CFLAGS	= $(DBX) -D$(OS) -D$(ARCH) -I$(INCDIR) $(ONLINELIB)
LDFLAGS = $(DBX)
LINTFLAGS= -D$(OS) -I$(INCDIR)

.KEEP_STATE:

##### beginning of dependency lines #####

all: $(RTNSLIB)

.c.o:
	@if [ $(OS) ] ; then \
	  set -x ; \
	  cc -c $(SVR4CFLAGS) $< ; \
	else \
	  set -x ; \
	  cc -c $(CFLAGS) $< ; \
	fi

$(RTNSLIB): $(INCLUDES) $(OBJS)
	ar cru $(RTNSLIB) $(OBJS)
	@if [ $(OS) ] ; then set -x ; \
	else set -x ; ranlib $(RTNSLIB) ; fi

install: all

library: all FRC
	echo -n 'New Version Log on ' > tmp; date >> tmp;\
	echo -n 'Modified from last ' >> tmp;\
	sccs what libversion >> tmp;\
	sccs edit libversion;\
	echo >> libversion;\
	cat tmp >> libversion;\
	echo 'Reasons for this new version:' >> libversion;\
	echo '- Please log reasons for this new version (use ^D to get out):';\
	cat >> libversion; set -x;\
	sccs delget libversion;\
	rm -f tmp

clean: FRC
	rm -f $(RTNSLIB) $(OBJS) $(LINTFILES) 

lint: $(LINTFILES)
	lint $(LINTFLAGS) $(LINTFILES) $(ONLINELIB)

info: FRC
	sccs info

FRC:
