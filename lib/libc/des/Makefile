#
# @(#)Makefile 1.1 92/07/30 SMI
#
# Copyright (c) 1987 by Sun Microsystems, Inc.
#

all: xall
include ../Makefile.arch
include ../Makefile.master
CFLAGS += -I.. 
HDRS = des_crypt.h 
SRCS = des_crypt.c des_soft.c
OBJS = $(SRCS:%.c=$(VARIANT)/%.o)

.INIT: $(HDRS)

xall: $$(LIBS)
$(LIBS) : symlink $$(VARIANT) $$(OBJS)

symlink:
	rm -rf strings;\
	ln -s ../strings strings

obj/des_soft.o obj_p/des_soft.o shared/des_soft.o \
s5/des_soft.o s5_p/des_soft.o shared_s5/des_soft.o: $(HDRS) $(SRCS)
	$(CC) $(CFLAGS$(TARGET_MACH)) $(CFLAGS) -R -c -o $@ des_soft.c

install: des_crypt.h
	install -c -m 444 des_crypt.h ${DESTDIR}/usr/include/des_crypt.h

# Used for those hard-to-get files when building the kernel.
init:

clean: master.clean 
