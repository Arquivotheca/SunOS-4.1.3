#
# @(#)Makefile 1.1 92/07/30
#
HEREHFILES= \
	../sparc/a.out.h ../sparc/asm_linkage.h async.h auxio.h \
	buserr.h clock.h cpu.h devaddr.h devr.h eeprom.h enable.h \
	../sparc/frame.h intreg.h iocache.h iommu.h memerr.h \
	mmu.h module.h param.h pcb.h ../sparc/psl.h pte.h \
	reg.h romvec.h sas_simdef.h scb.h ../sun/seg_kmem.h \
	../sparc/setjmp.h trap.h vm_hat.h vmparam.h

DEBUGHFILES	=	debug/allregs.h debug/asm_linkage.h debug/reg.h

HFILES	= $(HEREHFILES) $(DEBUGHFILES)

HDIR=$(DESTDIR)/usr/include/sun4m

install_h: $(HFILES) FRC
	install -d -m 755 $(HDIR)/debug
	install -m 444 $(HEREHFILES) $(HDIR)
	install -m 444 $(DEBUGHFILES) $(HDIR)/debug
	rm -rf $(HDIR)/fpu && ln -s ../sparc/fpu $(HDIR)/fpu

FRC:
