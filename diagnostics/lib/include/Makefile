#
#  @(#)Makefile 1.1 92/07/30 SMI;
#  Include files for libonline.a.
#
.DEFAULT:
	sccs get -G$@ $@

LIBUTIL	= libonline.h probe_sundiag.h
LIBXDR  = dtm_tcm.h tcm_dtm.h

all: $(LIBUTIL) $(LIBXDR)

install: all

clean:

lint:

info: FRC
	sccs info

FRC:
