#
# @(#)Makefile 1.1 92/07/30 SMI
#
# Copyright (c) 1987 by Sun Microsystems, Inc.
#

all: xall

include ../../Makefile.arch
include $(ARCH)/Makefile
include ../../Makefile.master

SRC = filbuf.c fopen.c fprintf.c \
	printf.c flsbuf.c \
	setbuf.c sprintf.c \
	vfprintf.c vprintf.c vsprintf.c
OBJS = $(SRC:%.c=$(VARIANT)/%.o) $(MD_CSRC:$(ARCH)/%.c=$(VARIANT)/%.o)
.KEEP_STATE:

xall: $$(LIBS)
$(LIBS) : symlink $$(VARIANT) $$(OBJS)

symlink:
	rm -rf strings;\
	ln -s ../../strings strings

clean: master.clean
