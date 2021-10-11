#
# @(#)Makefile 1.1 92/07/30 
#

TEXT= Copyright README.compressed README.half README.fullsrc README.intsrc \
	README.quarter README.sccs README.sunsrc README.winsrc 
PROGS= extract mksrc tapeblock
EXCLUDES= intsrc_exclude_list part_numbers sunsrc_exclude_list winsrc_list
SRCDIR= /usr/src
PRODUCTS= fullsrc intsrc sunsrc winsrc sccs

all: $(PRODUCTS)

$(PRODUCTS): $(TEXT) $(SCRIPTS) $(EXCLUDES)
	mksrc -s $(SRCDIR) $@
	mksrc -s $(SRCDIR) -f /dev/nrst8 $@

tapeblock: ../mktp/tapeblock.c
	cd $(?D); $(MAKE) $@
	cp $(?D)/$@ $@
