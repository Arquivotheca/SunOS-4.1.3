#
# @(#)Makefile 1.1 92/07/30 SMI
#
FILES=	addupc.s copy.s crt.s float.s mcount.s ocsum.s overflow.s \
	sparc_subr.s swtch.s underflow.s zs_asm.s
HFILES=	a.out.h asm_linkage.h frame.h pcb.h psl.h reg.h setjmp.h trap.h
HDIR=$(DESTDIR)/usr/include/sparc

all install: $(FILES)
	cd fpu && $(MAKE) $@

clean:

install_h: $(HFILES) FRC
	install -d -m 755 $(HDIR)
	install -m 444 $(HFILES) $(HDIR)
	cd fpu && $(MAKE) $@

FRC:

