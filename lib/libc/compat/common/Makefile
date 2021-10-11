#
# @(#)Makefile 1.1 92/07/30 SMI
#
# Copyright (c) 1987 by Sun Microsystems, Inc.
#
SRCS = gtty.c lockf.c pause.c stty.c tell.c ulimit.c utime.c
OBJS = $(SRCS:%.c=$(VARIANT)/%.o)

all: xall
include ../../Makefile.master
.KEEP_STATE:

xall: $$(LIBS)

$(LIBS) : symlink $$(VARIANT) $$(OBJS)

symlink:
	rm -rf strings;\
	ln -s ../../strings strings

clean: master.clean

