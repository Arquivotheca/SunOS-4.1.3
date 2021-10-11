#
# @(#)Makefile 1.1 92/07/30 SMI
#
# Copyright (c) 1987 by Sun Microsystems, Inc.
#
M4 = /bin/m4
ECHO = /bin/echo
SW_TRAP	=/usr/include/machine/trap.h
ASSRC	= div.s rem.s multiply.s umultiply.s ptr_call.s misalign.s start_float.s
MONDIR = ../../csu/common

# the MD_SRC are files that are used to  generate other files
MD_SRC = $(ARCH)/divrem.m4.s $(ARCH)/stret.s
MD_ASSRC = $(ASSRC:%.s=$(ARCH)/%.s)
MD_CSRC =$(ARCH)/_ftou.c
MD_OBJS = stret1.o stret2.o stret4.o _ftou.o mon.o alloc_pbuf.o


$(ARCH)/div.s:	$(ARCH)/divrem.m4.s $(SW_TRAP)
	$(ECHO) 'define(ANSWER,quotient)' | $(M4) - $(ARCH)/divrem.m4.s > $(ARCH)/div.s

$(ARCH)/rem.s:	$(ARCH)/divrem.m4.s $(SW_TRAP)
	$(ECHO) 'define(ANSWER,remainder)' | $(M4) - $(ARCH)/divrem.m4.s > $(ARCH)/rem.s

obj/stret1.o obj_p/stret1.o shared/stret1.o \
s5/stret1.o s5_p/stret1.o :$(ARCH)/stret.s 
	$(AS) $(ASFLAGS$(TARGET_MACH)) $(ASFLAGS) -DSTRET1 -o $@ $(ARCH)/stret.s

obj/stret2.o obj_p/stret2.o shared/stret2.o \
s5/stret2.o s5_p/stret2.o : $(ARCH)/stret.s
	$(AS) $(ASFLAGS$(TARGET_MACH)) $(ASFLAGS) -DSTRET2 -o $@ $(ARCH)/stret.s  

obj/stret4.o obj_p/stret4.o shared/stret4.o \
s5/stret4.o s5_p/stret4.o : $(ARCH)/stret.s
	$(AS) $(ASFLAGS$(TARGET_MACH)) $(ASFLAGS) -DSTRET4 -o $@ $(ARCH)/stret.s  
obj/mon.o obj_p/mon.o shared/mon.o \
s5/mon.o s5_p/mon.o:	$(MONDIR)/mon.c $(MONDIR)/mon.h 
	$(CC) -c $(CFLAGS) $(MONDIR)/mon.c -o $@

obj/alloc_pbuf.o obj_p/alloc_pbuf.o shared/alloc_pbuf.o \
s5/alloc_pbuf.o s5_p/alloc_pbuf.o:	$(MONDIR)/alloc_pbuf.c 
	$(CC) -c $(CFLAGS) $(MONDIR)/alloc_pbuf.c -o $@

clean: master.clean
