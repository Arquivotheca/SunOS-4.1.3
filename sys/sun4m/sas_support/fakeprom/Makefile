#
# @(#)Makefile	1.1 8/6/90 SMI
#


#
# Makefile for FakePROM
#

include ../Makefile.master

FAKEPTE= fakepte




all: $(FOBJS) $(FAKEPTE)


mmu_crueltt.o: mmu_crueltt.s
	cc -E -I. -DNO_INITIALIZE_MMU -DNO_ENABLE_MMU \
		 mmu_crueltt.s > mmu_crueltt.i
	as -o  mmu_crueltt.o mmu_crueltt.i

simutil.o: simutil.s
	cc -E -I. simutil.s > simutil.i
	as -o  simutil.o simutil.i

# build pagetables for FakePROM
# 
$(FAKEPTE): $(FAKEPTE).o
	cc -o $@ $(FAKEPTE).o
	$@ > fakeprompte


clean: FRC
	rm -rf *.o *~ '#*#' *.i $(FAKEPTE) fakeprompte

FRC:
