# @(#) Makefile 1.1 92/07/30 SMI
#

.KEEP_STATE:
.FRC:

DIFFDIR=/usr/bin
DIFFHDIR=/usr/lib
PR=/usr/bin/pr

# Binaries
BINS = diff
BINDIR = $(DESTDIR)$(DIFFDIR)

# Libraries
LIBS = diffh
LIBDIR = $(DESTDIR)$(DIFFHDIR)

# Headers, local headers, where to install them
LOCAL_HDRS = diff.h

# Source files
SRC = diff.c diffdir.c diffreg.c

# Object files
OBJS = diff.o diffdir.o diffreg.o

# build flags
CFLAGS = -O -DDIFF='"$(DIFFDIR)/diff"' -DDIFFH='"$(DIFFHDIR)/diffh"' -DPR='"$(PR)"'
LDFLAGS = -n -Bdynamic

# make sure all header files exist
.INIT:  $(HDRS) $(LOCAL_HDRS)

all :  bins libs

bins : $(BINS)

libs : $(LIBS) 

$(LOCAL_BINS) $(BINS) : $$(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(LINK_LIBS)

install: $(BINS) $(LIBS)
	install -d -o bin -m 755 $(BINDIR)
	install -s $(BINS) $(BINDIR)
	install -d -m 755 $(LIBDIR)
	install $(LIBS) $(LIBDIR)

install_h:

clean:
	-rm -rf $(LOCAL_BINS) $(BINS) $(LOCAL_LIBS) $(LIBS) $(OBJS) \
		diffh.o

diffh: diffh.o
	$(LINK.c) -o $@ diffh.o $(LINK_LIBS)
