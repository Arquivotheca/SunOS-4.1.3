#
#	@(#)Makefile 1.1 92/07/30 SMI;
#

CPP=/lib/cpp
INCFLAGS += -I../../lib/libc/sys/common/m68k
ASFLAGS= 

ARCH-mc68020=	m68k
ARCH-sparc = sparc
ARCH= $(ARCH$(TARGET_MACH))

.KEEP_STATE:


sbrk.o: sbrk.s
	$(CPP) $(CPPFLAGS) $(INCFLAGS) sbrk.s >tmp.s
	$(AS) $(ASFLAGS) -o $(OBJDIR)/$@ tmp.s
	@rm -f tmp.s
	
clean:
	@rm -f tmp.s
