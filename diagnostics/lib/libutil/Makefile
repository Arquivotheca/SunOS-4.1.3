#
#      @(#)Makefile	1.1 7/30/92  SMI 
#
.DEFAULT:
	sccs get -G$@ $@

OS=SVR4
#specify 'OS=' for 4.x built

INCDIR	= ../include
INCLUDES = $(INCDIR)/libonline.h

SRCS    = online_util.c online_info.c
OBJS    = $(SRCS:.c=.o)

UTILLIB = libutil.a
DBX	= -O
# = -g for dbx version
BSDCFLAGS = $(DBX) -D`arch` -I$(INCDIR)
SVR4CFLAGS = $(DBX) -Dsun4 -D$(OS) -I$(INCDIR)

.KEEP_STATE:

##### beginning of dependency lines #####

all: $(INCLUDES) $(UTILLIB) 

.c.o:
	@if [ $(OS) ]; then \
	  set -x ; \
	  cc -c $(SVR4CFLAGS) $< ; \
	else \
	  set -x; \
	  cc -c $(BSDCFLAGS) $< ; \
	fi

$(UTILLIB): $(OBJS)
	ar cru $(UTILLIB) $(OBJS)
	@if [ $(OS) ] ; then set -x ; \
	else set -x ; ranlib $@ ; fi

install: all

clean: FRC
	rm -f $(UTILLIB) $(OBJS)

info: FRC
	sccs info

FRC:
