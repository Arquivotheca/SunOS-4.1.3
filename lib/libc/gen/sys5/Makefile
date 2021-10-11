#
# @(#)Makefile 1.1 92/07/30 SMI
#
# Copyright (c) 1987 by Sun Microsystems, Inc.
#

SRCS = assert.c nlist.c setgid.c setuid.c sighold.c signal.c sgetl.c \
	sleep.c sputl.c ttyslot.c
OBJS = $(SRCS:%.c=$(VARIANT)/%.o)
include ../../Makefile.arch
include ../../Makefile.master
.KEEP_STATE:

all: xall

xall: $$(LIBS)
$(LIBS) : symlink $$(VARIANT) $$(OBJS)

clean: master.clean

symlink:
	rm -rf strings;\
	ln -s ../../strings strings
