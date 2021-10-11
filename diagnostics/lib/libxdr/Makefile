#
#	@(#)Makefile 1.1 92/07/30 SMI;
#

.DEFAULT:
	sccs get -G$@ $@

INCDIR	= ../include
INCLUDES = $(INCDIR)/tcm_dtm.h $(INCDIR)/dtm_tcm.h $(INCDIR)/probe_sundiag.h

XSRCS	= xdr_probe.c tcm_dtm.c dtm_tcm.c
XOBJS	= $(XSRCS:.c=.o)

XDRLIB	= libxdr.a

DBX	= -O
# = -g for dbx version
CFLAGS	= $(DBX) -I$(INCDIR)
LDFLAGS = $(DBX)

.KEEP_STATE:

##### beginning of dependency lines #####

all: $(XDRLIB)

$(XDRLIB): $(INCLUDES) $(XOBJS)
	ar cru $(XDRLIB) $(XOBJS)
	ranlib $(XDRLIB)

install: all

clean: FRC
	rm -f $(XDRLIB) *.o

lint:

info: FRC
	sccs info

FRC:
