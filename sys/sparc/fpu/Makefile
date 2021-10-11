#
# @(#)Makefile 1.1 92/07/30 SMI
#
FILES=	addsub.c compare.c div.c fpu.c fpu_simulator.c iu_simulator.c \
	mul.c pack.c unpack.c utility.c uword.c
HFILES=	fpu_simulator.h globals.h ieee.h
HDIR=$(DESTDIR)/usr/include/sparc/fpu

all: $(HFILES) $(FILES)

install: $(FILES)

clean:

install_h: $(HFILES) FRC
	install -d -m 755 $(HDIR)
	install -m 444 $(HFILES) $(HDIR)
FRC:

