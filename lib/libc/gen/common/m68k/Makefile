#
# @(#)Makefile 1.1 92/07/30 SMI
#
# Copyright (c) 1988 by Sun Microsystems, Inc.
#

#
# MACHINE-DEPENDENT source
#
CRT = ../../crt/$(ARCH)
SYS = ../../sys/common/$(ARCH)
CFLAGS$(TARGET_MACH) += -I$(CRT) -I$(SYS)
ASFLAGS$(TARGET_MACH) += -ten  
CPPFLAGS$(TARGET_MACH) += -I$(CRT) -I$(SYS) 

FPA_HDRS = DEFS.h Wdefs.h Mdefs.h Sdefs.h fpcrtdefs.h

ASSRC = abs.s alloca.s \
	bcopy.s bzero.s \
	fabs.s ffs.s \
	insque.s q.s\
	memcpy.s memset.s \
	remque.s \
	strcat.s strcpy.s strlen.s strncat.s strncpy.s \
        Vaints.s Vanints.s Varints.s \
        Vrems.s Vmods.s Vfloors.s Vceils.s Vsqrts.s Vlength2s.s Vpowis.s \
        Faints.s Frems.s Fsqrts.s Fpowis.s Flength2s.s \
        Saints.s Srems.s Ssqrts.s Spowis.s Slength2s.s \
        Mgens.s Mpowis.s \
        Wgens.s Wpowis.s \
        frexp.s ldexp.s modf.s fmod.s \
        Vaintd.s Vanintd.s Varintd.s \
        Vremd.s Vmodd.s Vfloord.s Vceild.s Vsqrtd.s Vlength2d.s Vpowid.s \
        Faintd.s Fremd.s Fsqrtd.s Fpowid.s Flength2d.s \
        Saintd.s Sremd.s Ssqrtd.s Spowid.s Slength2d.s \
        Mgend.s Mpowid.s \
        Wgend.s Wpowid.s \
	base_conv.s
S5ASSRC = _setjmp.s sigsetjmp.s
4.2ASSRC = _setjmp.s sigsetjmp.s

libc_comm.so := MD_ASSRC = $(ASSRC:%.s=$(ARCH)/%.s)
libc.so := MD_ASSRC =$(4.2ASSRC:%.s=$(ARCH)/%.s)
libc.a libc_p.a:= MD_ASSRC = $(4.2ASSRC:%.s=$(ARCH)/%.s) $(ASSRC:%.s=$(ARCH)/%.s) 
libcs5.so := MD_ASSRC =$(S5ASSRC:%.s=$(ARCH)/%.s) 
libcs5.a libcs5_p.a:= MD_ASSRC =$(S5ASSRC:%.s=$(ARCH)/%.s) $(ASSRC:%.s=$(ARCH)/%.s)

libc_comm.so libc.a libc_p.a libcs5.a libcs5_p.a := MD_CSRC = $(ARCH)/isinf.c

MD_FPA_HDRS =$(FPA_HDRS:%.h=$(CRT)/%.h)
MD_HDRS = $(ARCH)/unpkd.h

# as in the case of crt/Wfloat.o -Wpowid.o will not get executed on a sun2
obj/Wpowid.o obj_p/Wpowid.o shared/Wpowid.o \
s5/Wpowid.o s5_p/Wpowid.o :=  ASFLAGS-mc68010 = -ten -mc68020
