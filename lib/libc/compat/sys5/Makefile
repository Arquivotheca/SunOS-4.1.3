#
# @(#)Makefile 1.1 92/07/30 SMI
#
# Copyright (c) 1987 by Sun Microsystems, Inc.
#

all: xall

include ../../Makefile.arch
include ../../Makefile.master


STDSRC = nice.c rand.c times.c
# these should just be pitched, but in the interest of compatibility...
TRASHSRC = getpw.c
SRCS = $(STDSRC) $(TRASHSRC)
OBJS = $(SRCS:%.c=$(VARIANT)/%.o)
HDRS = epoch.h

.KEEP_STATE:

xall:  $$(LIBS)
$(LIBS) : symlink $(HDRS) $$(VARIANT) $$(OBJS)

clean: master.clean
	rm -f mkepoch epoch.h a.out

epoch.h: mkepoch
	./mkepoch >epoch.h

mkepoch: mkepoch.c
	$(CC) -USUNPRO_DEPENDENCIES -O -o mkepoch mkepoch.c

symlink:
	rm -rf strings;\
	ln -s ../../strings strings
