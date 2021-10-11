#
# @(#)Makefile 1.1 92/07/30 SMI
#
# Copyright (c) 1988 by Sun Microsystems, Inc.
#

ASFLAGS$(TARGET_MACH) += -I../../../crt/$(ARCH)

CSRC = isinf.c base_conv.c 

ASSRC = abs.s alloca.s \
        bcmp.s bcopy.s bzero.s ffs.s insque.s remque.s \
	strcpy.s strcmp.s strlen.s memcmp.s strncmp.s strncpy.s

S5ASSRC = _setjmp.s sigsetjmp.s
4.2ASSRC = $(S5ASSRC)

libc_comm.so := MD_ASSRC = $(ASSRC:%.s=$(ARCH)/%.s)
libc.so := MD_ASSRC = $(4.2ASSRC:%.s=$(ARCH)/%.s)
libc.a libc_p.a := MD_ASSRC = $(4.2ASSRC:%.s=$(ARCH)/%.s) $(ASSRC:%.s=$(ARCH)/%.s)
libcs5.so := MD_ASSRC = $(S5ASSRC:%.s=$(ARCH)/%.s) 
libcs5.a libcs5_p.a :=MD_ASSRC = $(S5ASSRC:%.s=$(ARCH)/%.s) $(ASSRC:%.s=$(ARCH)/%.s)

libc_comm.so libc.a libc_p.a libcs5.a libcs5_p.a := MD_CSRC = $(CSRC:%.c=$(ARCH)/%.c)

clean: master.clean

