#
# @(#)Makefile 1.1 92/07/30 SMI
#
# Makefile for /boot, boot blocks, and related stand-alone programs 

ARCH	=	sun3
MACH	=	m68k
ARCHDIR	=	../$(ARCH)
MACHDIR	=	../${MACH}
ARCHOBJ	=	conf.o
ARCHOPTS=	"-Dprintf=(*romp->v_printf)" "-Dputchar=(*romp->v_putchar)"
ARCHLIBS=

#
# LOAD is the text segment start for booted user programs.
# It is also used by srt0.s for the initial stack.
#
LOAD=   4000
#
# BRELOC is the text segment start for final stage (2nd or 3rd stage) boots.
# Note that this must be higher than the kernel's edata and should be
# higher than the kernel's bss so the bss can be cleared.
#
BRELOC= 1c0000
#
# BBRELOC is the text segment start for boot blocks.
# Note that this must be higher than boot's edata and should be
# higher than boot's bss so the bss can be cleared.
#
BBRELOC= 240000

BOOTBLOCKS= bootpr bootxd bootxy

ALL=	boot.$(ARCH) tpboot.$(ARCH) $(BOOTBLOCKS) installboot

all: $(ALL)

include ../boot/Makefile.com

install_mdec:
	install bootpr $(BOOTBLKDIR)/rawboot
	$(RM) $(BOOTBLKDIR)/bootsd
	ln $(BOOTBLKDIR)/rawboot $(BOOTBLKDIR)/bootsd
	install bootxd $(BOOTBLKDIR)
	install bootxy $(BOOTBLKDIR)
	install -c tpboot.$(ARCH) $(STANDDIR)
