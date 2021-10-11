#
# @(#)Makefile 1.1 92/07/30 SMI
#
# Copyright (c) 1987 by Sun Microsystems, Inc.
#
all: xall

include ../Makefile.arch 
include $(ARCH)/Makefile
include ../Makefile.master 

OBJS = $(MD_ASSRC:$(ARCH)/%.s=$(VARIANT)/%.o) $(MD_CSRC:$(ARCH)/%.c=$(VARIANT)/%.o) $(MD_OBJS:%.o=$(VARIANT)/%.o)

xall: $$(LIBS)
$(LIBS) : symlink $$(VARIANT) $$(OBJS)


.INIT: $(MD_FPA_HDRS) $(MD_SYS_HDRS)
clean: master.clean

symlink:
	rm -rf strings;\
	ln -s ../strings strings
