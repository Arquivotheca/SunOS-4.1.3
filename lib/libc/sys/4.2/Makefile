#
# @(#)Makefile 1.1 92/07/30 SMI
#
# Copyright (c) 1987 by Sun Microsystems, Inc.
#

all: xall

include ../../Makefile.arch
include $(ARCH)/Makefile
include ../../Makefile.master

OBJS = $(MD_PSEUDO_OBJS:%.o=$(VARIANT)/%.o)

.KEEP_STATE:

xall: $$(LIBS)
$(LIBS) : symlink $$(VARIANT) $$(OBJS)

symlink:
	rm -rf strings;\
	ln -s ../../strings strings

clean: master.clean

.INIT: $(MD_HDRS)
