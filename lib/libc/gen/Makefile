#
# @(#)Makefile 1.1 92/07/30 SMI
#

all: xall

include ../Makefile.master

ALLDIRS = common 4.2 sys5

clean libc.so		       		:= SUBDIRS += 4.2
clean libc.a libc_p.a			:= SUBDIRS += 4.2 common
clean libcs5.so				:= SUBDIRS += sys5
clean libcs5.a libcs5_p.a		:= SUBDIRS += sys5 common
clean libc_comm.so			:= SUBDIRS += common
clean libbsdmalloc.a				:= SUBDIRS += common

xall: $$(LIBS)

$(LIBS) : $$(SUBDIRS)

$(ALLDIRS): FRC
	cd $@; $(MAKE) $(MFLAGS) $(LIB)

clean:
	@set -x; for i in $(SUBDIRS); do (cd $$i; $(MAKE) clean); done

FRC:
