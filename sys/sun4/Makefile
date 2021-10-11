#
# @(#)Makefile 1.1 92/07/30
#
HEREHFILES = ../sparc/a.out.h ../sparc/asm_linkage.h buserr.h clock.h cpu.h \
	diag.h eccreg.h eeprom.h enable.h ../sparc/frame.h intreg.h \
	iocache.h memerr.h mmu.h param.h ../sparc/pcb.h ../sparc/psl.h \
	pte.h reg.h setjmp.h scb.h ../sparc/trap.h vmparam.h vm_hat.h \
	seg_kmem.h

DEBUGHFILES = debug/allregs.h

HFILES  = $(HEREHFILES) $(DEBUGHFILES)

HDIR=$(DESTDIR)/usr/include/sun4

install_h: $(HFILES) FRC
	install -d -m 755 $(HDIR)/debug
	install -m 444 $(HEREHFILES) $(HDIR)
	install -m 444 $(DEBUGHFILES) $(HDIR)/debug
	rm -rf $(HDIR)/fpu && ln -s ../sparc/fpu $(HDIR)/fpu

FRC:
