#
# @(#)Makefile 1.1 92/07/30 SMI
#
# Copyright (c) 1987 by Sun Microsystems, Inc.
#
SRCS = inet_addr.c inet_network.c \
	inet_netof.c inet_ntoa.c inet_lnaof.c \
	inet_makeaddr.c
OBJS = $(SRCS:%.c=$(VARIANT)/%.o)

all: xall
include ../Makefile.arch
include ../Makefile.master

.KEEP_STATE:

xall: $$(LIBS)
$(LIBS): symlink $(HDRS) $$(VARIANT) $$(OBJS)

symlink:
	rm -rf strings;\
	ln -s ../strings strings

clean: master.clean
