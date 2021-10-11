#
# @(#)Makefile 1.1 92/07/30 SMI
#
# Copyright (c) 1987 by Sun Microsystems, Inc.
#

#????	vlimit is not included in the build in the original Makefile

all: xall

include ../../Makefile.arch
include ../../Makefile.master


STDSRC = ftime.c nice.c rand.c times.c vlimit.c vtimes.c
# these should just be pitched, but in the interest of compatibility...
TRASHSRC = getpw.c
SRCS = $(STDSRC) $(TRASHSRC)
OBJS = $(SRCS:%.c=$(VARIANT)/%.o)

.KEEP_STATE:

xall:  $$(LIBS)
$(LIBS) : symlink $$(VARIANT) $$(OBJS)

symlink:
	rm -rf strings;\
	ln -s ../../strings strings

clean: master.clean

