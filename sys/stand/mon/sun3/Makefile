#
# @(#)Makefile 1.1 92/07/30 SMI
#
FILES= blts.s cmp.s cpu.map.s db.s patt.s romvec.s space.s \
	trap.s asm_linkage.h assym.h cpu.addrs.h cpu.buserr.h \
	cpu.map.h cpu.misc.h dpy.h keyboard.h m68vectors.h \
	machdep.h memerr.h memreg.h structconst.h sunmon.h



all: $(FILES)

clean:
	sccs clean

HFILES=	sunmon.h
KFILES= cpu.map.h cpu.addrs.h

install_h: $(HFILES) $(KFILES)
	install -m 444 $(HFILES) $(DESTDIR)/usr/include/sun3
