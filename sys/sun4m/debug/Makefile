#	@(#)Makefile 1.1 90/03/23 SMI
#
# Sun-4m kernel debugger (kadb) makefile
#
ARCH=	sun4m
MACH=	sparc
CPP=	/lib/cpp -undef

CPPOPTS= -DKADB -Dsun -D$(ARCH) -D$(MACH) -DKERNEL -I.. -I../..
COPTS=	-O
CFLAGS=	$(CPPOPTS) $(COPTS)
ASFLAGS= -P $(CPPOPTS) -DLOCORE
KADBDIR= ../../kadb

HFILES= ../../sun4m/debug/reg.h

CFILES= ../../sun4m/debug/machdep.c ../../debug/main.c \
	../../debug/support.c ../../sun4m/clock.c

SFILES= locore.s map.s

SPECIAL= locore.o

OBJ=	$(SPECIAL) machdep.o map.o main.o support.o clock.o

debug.o: $(OBJ)
	$(LD) -r -o $@ $(OBJ)

assym.s: genassym.c
	$(CC) $(CFLAGS) -I. genassym.c
	./a.out >assym.s;
	@ rm a.out
#	-if [ `/bin/arch -k` = $(ARCH) ] ; \
#	then \
#		./a.out >assym.s; \
#	else \
#		sas -R a.out >assym.s 2>/dev/null; \
#	fi


main.o: ../../debug/main.c
	$(CC) $(CFLAGS) -c ../../debug/main.c

support.o: ../../debug/support.c
	$(CC) $(CFLAGS) -c ../../debug/support.c

clock.o: ../clock.c
	$(CC) $(CFLAGS) -c ../clock.c

reloc:
	@echo 'FFC00000'

tags:	$(KADBDIR)/tags
	@ cd $(KADBDIR); $(MAKE) -e $@
	ln -s $(KADBDIR)/tags $@

tags.list: FRC
	@echo $(HFILES) $(CFILES)

stags.list: FRC
	@for i in ${SFILES} ; do \
		echo ../../sun4m/debug/$$i ; done

clean:
	$(RM) a.out *.o assym.s

makedep:	assym.s FRC
	@cat /dev/null >makedep
	@-(for i in ${CFILES} ${SFILES} genassym.c ; do \
		${CPP} -M ${CPPOPTS} $$i >> makedep; done) \
		2>makedeperrs
	@sed \
		-e 's,^.*genassym\.o:,assym.s assym.L:,' \
		-e 's,^\(.*\)\.o:,\1.o \1.L:,' makedep > makedep1
	@awk ' { if ($$1 != prev) { print rec; rec = $$0; prev = $$1; } \
		else { if (length(rec $$3) > 78) { print rec; rec = $$0; } \
		else rec = rec " " $$3 } } \
		END { print rec } ' makedep1 > makedep
	@cat makedeperrs >&- 1>&2
	@(if [ -s makedeperrs ]; then false; fi)


depend:	makedep
	@echo '/^# DO NOT DELETE THIS LINE/+1,$$d' >eddep
	@echo '$$r makedep' >>eddep
	@echo 'w' >>eddep
	@cp Makefile Makefile.bak
	@ed - Makefile < eddep
	@echo '# DEPENDENCIES MUST END AT END OF FILE' >> Makefile
	@echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> Makefile
	@echo '# see make depend above' >> Makefile
	@$(RM) eddep makedep makedep1 makedeperrs

FRC:

# DO NOT DELETE THIS LINE -- make depend uses it

machdep.o machdep.L: ../../sun4m/debug/machdep.c ../../sys/param.h
machdep.o machdep.L: ../machine/param.h ../../sys/signal.h ../../vm/faultcode.h
machdep.o machdep.L: ../../sys/types.h ../../sys/sysmacros.h ../../sys/errno.h
machdep.o machdep.L: ../../sys/vmmac.h ../machine/buserr.h ../machine/enable.h
machdep.o machdep.L: ../machine/mmu.h ../../debug/debug.h ../../mon/sunromvec.h
machdep.o machdep.L: ../../sys/types.h ../../sys/param.h ../../mon/openprom.h
machdep.o machdep.L: ../machine/romvec.h ../machine/cpu.h ../../mon/sunromvec.h
machdep.o machdep.L: ../machine/pte.h ../machine/scb.h ../machine/psl.h
machdep.o machdep.L: ../machine/trap.h ../machine/clock.h ../machine/intreg.h
machdep.o machdep.L: ../machine/eeprom.h ../../mon/eeprom.h
machdep.o machdep.L: ../../mon/keyboard.h ../../mon/password.h
machdep.o machdep.L: ../../mon/idprom.h ../machine/asm_linkage.h
machdep.o machdep.L: ../machine/reg.h ../machine/frame.h
machdep.o machdep.L: ../../sun4m/debug/allregs.h ../../debug/debug.h
machdep.o machdep.L: ../../debug/debugger.h /usr/include/setjmp.h
machdep.o machdep.L: /usr/include/machine/setjmp.h ../../debug/debug.h
main.o main.L: ../../debug/main.c ../../sys/param.h ../machine/param.h
main.o main.L: ../../sys/signal.h ../../vm/faultcode.h ../../sys/types.h
main.o main.L: ../../sys/sysmacros.h ../../sys/reboot.h ../machine/reg.h
main.o main.L: ../../debug/debugger.h /usr/include/setjmp.h
main.o main.L: /usr/include/machine/setjmp.h ../../debug/debug.h
main.o main.L: ../../mon/sunromvec.h ../../sys/types.h ../../sys/param.h
main.o main.L: ../../mon/openprom.h ../machine/romvec.h ../a.out.h
main.o main.L: ../../sys/exec.h /usr/include/ctype.h
support.o support.L: ../../debug/support.c ../../sys/param.h ../machine/param.h
support.o support.L: ../../sys/signal.h ../../vm/faultcode.h ../../sys/types.h
support.o support.L: ../../sys/sysmacros.h /usr/include/time.h
support.o support.L: /usr/include/tzfile.h /usr/include/string.h
support.o support.L: ../../sys/errno.h ../../sys/time.h ../machine/mmu.h
support.o support.L: ../../debug/debug.h ../../mon/sunromvec.h
support.o support.L: ../../sys/types.h ../../sys/param.h ../../mon/openprom.h
support.o support.L: ../machine/romvec.h ../machine/pte.h ../machine/reg.h
support.o support.L: ../../debug/debugger.h /usr/include/setjmp.h
support.o support.L: /usr/include/machine/setjmp.h ../../debug/debug.h
support.o support.L: ../../sys/ptrace.h /usr/include/varargs.h
support.o support.L: ../machine/debug/allregs.h
clock.o clock.L: ../../sun4m/clock.c ../../sys/param.h ../machine/param.h
clock.o clock.L: ../../sys/signal.h ../../vm/faultcode.h ../../sys/types.h
clock.o clock.L: ../../sys/sysmacros.h ../../sys/time.h ../../sys/kernel.h
clock.o clock.L: ../machine/clock.h ../machine/intreg.h ../machine/pte.h
locore.o locore.L: locore.s ./assym.s ../../sys/errno.h ../../sys/param.h
locore.o locore.L: ../machine/param.h ../../sys/signal.h ../../vm/faultcode.h
locore.o locore.L: ../../sys/types.h ../../sys/sysmacros.h
locore.o locore.L: ../machine/asm_linkage.h ../machine/reg.h ../machine/mmu.h
locore.o locore.L: ../../debug/debug.h ../../mon/sunromvec.h ../../sys/types.h
locore.o locore.L: ../../sys/param.h ../../mon/openprom.h ../machine/romvec.h
locore.o locore.L: ../machine/psl.h ../machine/pte.h ../machine/enable.h
locore.o locore.L: ../machine/cpu.h ../../mon/sunromvec.h ../machine/trap.h
locore.o locore.L: ../machine/eeprom.h ../../mon/eeprom.h ../../mon/keyboard.h
locore.o locore.L: ../../mon/password.h ../../mon/idprom.h
locore.o locore.L: ./../../debug/debug.h
map.o map.L: map.s ./assym.s ../../sys/param.h ../machine/param.h
map.o map.L: ../../sys/signal.h ../../vm/faultcode.h ../../sys/types.h
map.o map.L: ../../sys/sysmacros.h ../machine/mmu.h ../../debug/debug.h
map.o map.L: ../../mon/sunromvec.h ../../sys/types.h ../../sys/param.h
map.o map.L: ../../mon/openprom.h ../machine/romvec.h ../machine/pte.h
map.o map.L: ../machine/enable.h ../machine/cpu.h ../../mon/sunromvec.h
map.o map.L: ../machine/eeprom.h ../../mon/eeprom.h ../../mon/keyboard.h
map.o map.L: ../../mon/password.h ../../mon/idprom.h ../machine/asm_linkage.h
map.o map.L: ../machine/reg.h ../../debug/debug.h
assym.s assym.L: genassym.c ../../sys/param.h ../machine/param.h
assym.s assym.L: ../../sys/signal.h ../../vm/faultcode.h ../../sys/types.h
assym.s assym.L: ../../sys/sysmacros.h ../machine/pte.h ../machine/psl.h
assym.s assym.L: ../machine/mmu.h ../../debug/debug.h ../../mon/sunromvec.h
assym.s assym.L: ../../sys/types.h ../../sys/param.h ../../mon/openprom.h
assym.s assym.L: ../machine/romvec.h ../machine/cpu.h ../../mon/sunromvec.h
assym.s assym.L: ../../mon/sunromvec.h ./allregs.h ../machine/reg.h
assym.s assym.L: ../../debug/debug.h
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
