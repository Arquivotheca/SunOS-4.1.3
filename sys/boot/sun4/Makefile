#
# @(#)Makefile 1.1 92/07/30 SMI
#
# Makefile for /boot, boot blocks, and related stand-alone programs 

ARCH	=	sun4
MACH	=	sparc
ARCHDIR	= 	../${ARCH}
MACHDIR	=	../${MACH}
ARCHOBJ	=	conf.o trap.o stubs.o
ARCHOPTS=	"-Dprintf=(*romp->v_printf)" "-Dputchar=(*romp->v_putchar)"
ARCHLIBS=
MODEL	=	-DSUN4_470 -DSUN4_330 -DSUN4_260 -DSUN4_110
 
#
# LOAD is the text segment start for booted user programs.
# It is also used by srt0.s for the initial stack.
#
LOAD= 4000
#
# BRELOC is the text segment start for final stage (2nd or 3rd stage) boots.
# Note that this must be higher than the kernel's edata and should be
# higher than the kernel's bss so the bss can be cleared.
#
BRELOC= 200000
#
# BBRELOC is the text segment start for boot blocks.
# Note that this must be higher than boot's edata and should be
# higher than boot's bss so the bss can be cleared.
#
BBRELOC= 240000

BOOTBLOCKS= bootpr bootxd bootxy bootid

ALL=	boot.$(ARCH) tpboot.$(ARCH) $(BOOTBLOCKS) installboot

all: $(ALL)

# Rules for arch-dependent .c files
$$(LIB)(%.o): $(ARCHDIR)/%.c
	$(CC) $(CFLAGS) -DLOAD=0x4000 -DBRELOC=0x$(BRELOC) -c $<
	@ $(AR) $(ARFLAGS) $(LIB) $% ; $(RM) $%

# Rules for arch-dependent .s files
$$(LIB)(%.o): $(ARCHDIR)/%.s
	@ $(RM) tmp.c tmp.i $%
	cp $< tmp.c
	/lib/cpp -P -DBOOTBLOCK -DLOCORE $(CPPOPTS) $(MODEL) $(BOOTOPT) \
	    -DLOAD=0x$(LOAD) -DBRELOC=0x$(BRELOC) tmp.c > tmp.s
	$(AS) -P -Q  tmp.s -o $%
	@ $(AR) $(ARFLAGS) $(LIB) $%
	@ $(RM) tmp.c tmp.i tmp.s $%

# Standalone start-off.
SRTOPT=			  -DLOCORE -DLOAD=0x$(LOAD) $(MODEL)
srt0xx.o:=	SRTOPT += -DBOOTBLOCK

srt0.o srt0xx.o: $(ARCHDIR)/srt0.S
	cc -c $(CFLAGS) $(SRTOPT) $(BOOTOPT) -o $@ $(ARCHDIR)/srt0.S

include ../boot/Makefile.com

install_mdec:
	install bootpr $(BOOTBLKDIR)/rawboot
	$(RM) $(BOOTBLKDIR)/bootsd
	ln $(BOOTBLKDIR)/rawboot $(BOOTBLKDIR)/bootsd
	install bootxd $(BOOTBLKDIR)
	install bootxy $(BOOTBLKDIR)
	install bootid $(BOOTBLKDIR)
	install -c tpboot.$(ARCH) $(STANDDIR)
