#
# @(#)Makefile 1.1 92/07/30 SMI
#
HFILES = a.out.h asm_linkage.h buserr.h clock.h cpu.h diag.h eccreg.h eeprom.h \
	enable.h frame.h interreg.h memerr.h mmu.h param.h pcb.h psl.h \
	pte.h reg.h setjmp.h scb.h trap.h udvma.h vmparam.h vm_hat.h seg_kmem.h

HDIR=$(DESTDIR)/usr/include/sun3

install_h: $(HFILES) FRC
	install -m 444 $(HFILES) $(HDIR)

FRC:
