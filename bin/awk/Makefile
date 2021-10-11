#
# @(#)Makefile 1.1 92/07/30 SMI; from UCB 4.2 83/07/08
#

.KEEP_STATE:
.FRC:

# Binaries, local binaries, where to install them
BINS = awk
LOCAL_BINS = makeprctab
BINDIR = $(DESTDIR)/usr/bin

LOCAL_HDRS = awk.def y.tab.h awk.h tokendefs

SRC = awk.def awk.g.y awk.lx.l b.c lib.c main.c parse.c \
	makeprctab.c freeze.c run.c token.c tran.c

OBJS = awk.lx.o b.o main.o token.o tran.o lib.o run.o \
	parse.o proctab.o freeze.o awk.g.o
LOCAL_OBJS = makeprctab.o token.o

LINK_LIBS = -lm

CFLAGS = -O
YFLAGS = -d

all install install_h install_bins install_libs := TARGET = $@

makeprctab := OBJS = $(LOCAL_OBJS)

clean := TARGET = clean LOCAL_HDRS =

# make sure all non-local header files exist
.INIT:  $(HDRS) $(LOCAL_HDRS)

all:  $(BINS)

$(LOCAL_BINS) $(BINS): $$(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(LINK_LIBS)

install: $(BINS)
	install -d -o bin -m 755 $(BINDIR)
	install -s $(BINS) $(BINDIR)

install_h:

clean:
	echo $(TARGET) $(LOCAL_HDRS)
	-rm -rf $(LOCAL_BINS) $(BINS) $(LOCAL_OBJS) $(OBJS) \
		install_bins install_libs \
		core a.out y.tab.h awk.h tokendefs proctab.c

y.tab.h:	awk.g.o

awk.h:	y.tab.h
	cp y.tab.h awk.h

tokendefs:	y.tab.h tokenscript
	rm -f tokendefs
	ed - <tokenscript

proctab.c:	makeprctab
	makeprctab >proctab.c
