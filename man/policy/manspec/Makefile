# @(#)Makefile 1.1 92/07/30 SMI;
#
# Makefile for building and printing the Man Page Specification (manspec).
# Please see the README in this directory before using this Makefile.
# If you edit this Makefile, don't forget to update README if necessary.

FILES= frnt.mex pref.mex intro.mex generic.mex system.mex libfunc.mex \
dni.mex fileform.mex appa.mex appb.mex appc.mex appd.mex
TROFF=/usr/doctools/bin/troff
SPEC.DIT=/tmp/manspec.dit
PRINTER=lw

$(SPEC.DIT): $(TROFF)
	tbl $(FILES) | $(TROFF) -t -mex > $(SPEC.DIT)

print: $(SPEC.DIT)
	lpr -n -P$(PRINTER) $(SPEC.DIT)

clean:
	rm -f $(SPEC.DIT)
