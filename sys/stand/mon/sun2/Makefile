#
# @(#)Makefile 1.1 92/07/30 SMI; from UCB X.X XX/XX/XX
#
FILES = copy.c  genassym.c  ktab.vt100.c sunlogo.c version.c \
	drawchr.c insdel.c machdep.c vec.c fill.c \
	ktab.klunk.c mem_grab.c  vector.c\
	cpu.misc.h globram.h  montrap.h  sunmon.h \
	cpu.addrs.h dpy.h  ndio.h suntimer.h \
	cpu.buserr.h enable.h   m68vectors.h    param.h \
	cpu.map.h framebuf.h machdep.h\
	blts.s cpu.map.s patt.s set_evec.s trap.s \
	bres.s db.s probe.s setbus.s \
	cmp.s getidprom.s romvec.s space.s 

HFILES= sunmon.h
KFILES= cpu.map.h cpu.addrs.h

all:	$(FILES)

touchall:	$(FILES) all
	@chmod +w $(FILES)
	@touch $(FILES)

clean:
	sccs clean

install_h: $(HFILES) $(KFILES)
	@set -x; for i in $(HFILES) ; do \
		install -c $$i $(DESTDIR)/usr/include/sun2; done
