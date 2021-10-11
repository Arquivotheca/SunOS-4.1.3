#
# @(#)Makefile 1.1 92/07/30 SMI
#
FILES=	addupc.s frame.h mcount.s movc.s ocsum.s reg.h scb.h setjmp.s trap.h
HFILES = excframe.h frame.h reg.h scb.h trap.h

HDIR=$(DESTDIR)/usr/include/m68k

all: $(FILES)

install: $(FILES)

clean:

install_h: $(HFILES) FRC
	install -m 444 $(HFILES) $(HDIR)
FRC:
