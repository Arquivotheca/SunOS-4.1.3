#
# @(#)Makefile 1.1 92/07/30 SMI
#
.DEFAULT:
	sccs get $@

FILES= 3101 aa aed512 beehive diablo dtc382 ibm3101 std stdcrt \
	teleray vt100 wyse-adds xerox1720 xerox1730 xerox1730-lm zenith29

all: $(FILES)

install: all
	cp $(FILES) ${DESTDIR}/usr/share/lib/tabset
