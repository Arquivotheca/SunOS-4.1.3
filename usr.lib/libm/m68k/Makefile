#
# @(#)Makefile 1.1 92/07/30 SMI;
#
# Copyright (c) 1987 by Sun Microsystems, Inc.
#

.SUFFIXES:	.il .il~ .S .S~

SRCS=\
libm2d.S      libm3d.S      libmd.S \
libm.il       libm2s.S      libm3s.S      libms.S

all:	$(SRCS)

.il~.il:
	sccs get -G$@ $@
	
.S~.S:
	sccs get -G$@ $@
	
clean:
	rm -f a.out core errs *.o

install:
