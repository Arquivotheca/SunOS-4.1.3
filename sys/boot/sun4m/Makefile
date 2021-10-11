#
# @(#)Makefile 1.1 92/07/30 SMI
#
# Makefile for /boot, boot blocks, and related stand-alone programs 

ARCH=		sun4m
MACH=		sparc
ARCHDIR=	../$(ARCH)
MACHDIR=	../$(MACH)
ARCHOBJ=	conf.o trap.o stubs.o
ARCHOPTS=	-DSUN4M_690 -DSUN4M_50 $(ARCHDEFS)
ARCHDEFS=	$(LIBPROMCPPOPTS)
ARCHLIBS=	$(LIBPROM)
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
BRELOC= 400000
#
# BBRELOC is the text segment start for boot blocks.
# Note that this must be higher than boot's edata and should be
# higher than boot's bss so the bss can be cleared.
#
BBRELOC= 440000

BOOTBLOCKS= bootopen

ALL=	boot.$(ARCH) $(BOOTBLOCKS) installboot

all: $(ALL)

include ../boot/Makefile.com

# Special rules for sun4m, which uses a.out executables as its boot programs.
boot.$(ARCH): boot
	cp $? $@; strip $@

# Rules for arch-dependent .c files
$$(LIB)(%.o): $(ARCHDIR)/%.c
	$(CC) $(CFLAGS) -DLOAD=0x4000 -c $<
	@ $(AR) $(ARFLAGS) $(LIB) $% ; $(RM) $%

# Rules for arch-dependent .s files
$$(LIB)(%.o): $(ARCHDIR)/%.s
	@ $(RM) tmp.c tmp.i $%
	cp $< tmp.c
	/lib/cpp -P -DBOOTBLOCK -DLOCORE $(CPPOPTS) $(BOOTOPT) -DLOAD=0x$(LOAD) tmp.c > tmp.s
	$(AS) -P -Q  tmp.s -o $%
	@ $(AR) $(ARFLAGS) $(LIB) $%
	@ $(RM) tmp.c tmp.i tmp.s $%

# Standalone start-off.
SRTOPT=			  -DLOCORE -DLOAD=0x$(LOAD)
srt0xx.o:=	SRTOPT += -DBOOTBLOCK

srt0.o srt0xx.o: $(ARCHDIR)/srt0.S
	cc -c $(CFLAGS) $(SRTOPT) $(BOOTOPT) -o $@ $(ARCHDIR)/srt0.S

install_mdec:
	install bootopen $(BOOTBLKDIR)/rawboot
	$(RM) $(BOOTBLKDIR)/bootsd $(BOOTBLKDIR)/bootfd $(BOOTBLKDIR)/bootid
	ln $(BOOTBLKDIR)/rawboot $(BOOTBLKDIR)/bootsd
	ln $(BOOTBLKDIR)/rawboot $(BOOTBLKDIR)/bootfd
	ln $(BOOTBLKDIR)/rawboot $(BOOTBLKDIR)/bootid
	# HACK - see bugid 1041421
	echo "DUMMY sun4m tpboot file to get around suninstall bug in 4.1" > $(STANDDIR)/tpboot.sun4m
	echo "tpboot is now OBSOLETE for sun4m, but 4.1 suninstall expects it" >> $(STANDDIR)/tpboot.sun4m
	chmod 644 $(STANDDIR)/tpboot.sun4m
