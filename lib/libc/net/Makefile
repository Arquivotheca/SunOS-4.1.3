#
# @(#)Makefile 1.1 92/07/30 SMI
#
# Copyright (c) 1987 by Sun Microsystems, Inc.
#

MID_SRC = ether_addr.c bindresvport.c gethostent.c getnetent.c getnetgrent.c \
	getprotoent.c getrpcent.c getrpcport.c getservent.c innetgr.c rcmd.c \
	rexec.c ruserpass.c
OBJS = $(MID_SRC:%.c=$(VARIANT)/%.o) $(MD_CSRC:$(ARCH)/%.c=$(VARIANT)/%.o)

all: xall
include ../Makefile.arch
include $(ARCH)/Makefile
include ../Makefile.master
#CFLAGS += -R
LDFLAGS += -assert pure-text

xall: $$(LIBS) 
$(LIBS) : symlink $$(VARIANT) $$(OBJS)

clean : master.clean

symlink:
	rm -rf strings;\
	ln -s ../strings strings
