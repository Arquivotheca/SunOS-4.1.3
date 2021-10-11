#
# @(#)Makefile 1.1 92/07/30
#
HEREHFILES =	../sparc/a.out.h ../sparc/asm_linkage.h auxio.h buserr.h \
	clock.h cpu.h devr.h eeprom.h enable.h ../sparc/frame.h intreg.h \
	memerr.h mmu.h param.h ../sparc/pcb.h ../sparc/psl.h pte.h reg.h \
	romvec.h scb.h ../sparc/setjmp.h ../sun/seg_kmem.h  \
	trap.h ../sun4/vm_hat.h vmparam.h

DEBUGHFILES	=	debug/allregs.h debug/asm_linkage.h debug/reg.h

HFILES	= $(HEREHFILES) $(DEBUGHFILES)

HDIR=$(DESTDIR)/usr/include/sun4c

install_h: $(HFILES) FRC
	install -d -m 755 $(HDIR)/debug
	install -m 444 $(HEREHFILES) $(HDIR)
	install -m 444 $(DEBUGHFILES) $(HDIR)/debug
	rm -rf $(HDIR)/fpu && ln -s ../sparc/fpu $(HDIR)/fpu

FRC:
