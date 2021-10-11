#
# @(#)Makefile 1.1 92/07/30 SMI
#
HFILES = a.out.h asm_linkage.h buserr.h clock.h cpu.h devaddr.h diag.h \
	eccreg.h eeprom.h enable.h frame.h interreg.h iocache.h memerr.h \
	mmu.h param.h pcb.h psl.h pte.h reg.h scb.h seg_kmem.h setjmp.h \
	trap.h vm_hat.h vmparam.h

HDIR=$(DESTDIR)/usr/include/sun3x

install_h: $(HFILES) FRC
	install -d -m 755 $(HDIR)
	install -m 444 $(HFILES) $(HDIR)

FRC:
