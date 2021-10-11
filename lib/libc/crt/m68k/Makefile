#
# @(#)Makefile 1.1 92/07/30 SMI
#
# Copyright (c) 1988 by Sun Microsystems, Inc.
#
INCLUDE = -I../sys/common/$(ARCH) -I$(ARCH)
CPPFLAGS += $(INCLUDE) 
CFLAGS$(TARGET_MACH) += $(INCLUDE)
ASFLAGS$(TARGET_MACH) += -ten 
SYS_HDRS = SYS.h PIC.h
MD_SYS_HDRS = $(SYS_HDRS:%=../sys/common/$(ARCH)/%)
FPA_HDRS = DEFS.h Mdefs.h Sdefs.h Wdefs.h fpa_support.h  \
	fpcrtdefs.h fpcrttypes.h sky.h divide.include
MD_FPA_HDRS = $(FPA_HDRS:%=$(ARCH)/%)
ASSRC = lmodt.s ldivt.s lmult.s ptwo.s \
	start_float.s float_switch.s V_switch.s \
	Vinit.s Vswitch.s \
	Vmode.s Vstatus.s \
	Vdtos.s Vstod.s \
	Vflts.s Vuns.s Vints.s Vnints.s Vrints.s \
	Vadds.s Vsubs.s Vmuls.s Vdivs.s Vcmps.s \
	Vsqrs.s Vscaleis.s \
	Vfltd.s Vund.s Vintd.s Vnintd.s Vrintd.s \
	Vaddd.s Vsubd.s Vmuld.s Vdivd.s Vcmpd.s \
	Vsqrd.s Vscaleid.s \
	Finit.s Fswitch.s \
	Fclass2d.s Fclass2s.s \
	Ffloat.s Fdtos.s Fstod.s \
	Fflts.s Fadds.s Fmuls.s \
	Ffltd.s Faddd.s Fmuld.s \
	Sinit.s Sswitch.s \
	Sfloat.s \
	Snints.s Sscaleis.s \
	Snintd.s Sscaleid.s \
	Minit.s Mdefault.s ma93n.s Mswitch.s \
	Mfloat.s \
	Winit.s Wswitch.s \
	Wfloat.s \
	fpa_handler.s fpa_81comp.s fpa_trans.s fp_commons.s\
	fp_save.s rel2support.s

CSRC = finitfp.c fp_globals.c fpa_handle.c \
	minitfp.c sinitfp.c vinitfp.c winitfp.c \
	mon.c alloc_pbuf.c

MD_ASSRC = $(ASSRC:%.s=$(ARCH)/%.s)
MD_CSRC = $(CSRC:%.c=$(ARCH)/%.c)

MONDIR = ../../csu/common

obj/fpa_81comp.o obj_p/fpa_81comp.o shared/fpa_81comp.o \
s5/fpa_81comp.o s5_p/fpa_81comp.o : $(ARCH)/fpa_recompute.c $(ARCH)/fpa_support.h
	$(CC) $(CFLAGS)  $(CFLAGS$(TARGET_MACH)) -DF68881 -S -O -f68881 $(ARCH)/fpa_recompute.c /usr/lib/f68881/libm.il
	sed /f68881_used/d fpa_recompute.s > tmp.s
	$(AS) $(ASFLAGS) $(ASFLAGS$(TARGET_MACH)) tmp.s -o $@
	rm -f tmp.s fpa_recompute.s

obj/Wfloat.o obj_p/Wfloat.o s5/Wfloat.o s5_p/Wfloat.o \
shared/Wfloat.o := ASFLAGS-mc68010 = -ten -mc68020 

obj/fpa_handle.o shared/fpa_handle.o s5/fpa_handle.o := CFLAGS += -A-R

obj/fpa_handler.o shared/fpa_handler.o s5/fpa_handler.o := ASFLAGS += -R

obj/minitfp.o shared/minitfp.o s5/minitfp.o := CFLAGS += -A-R

obj/sinitfp.o shared/sinitfp.o s5/sinitfp.o := CFLAGS += -A-R

obj/winitfp.o shared/winitfp.o s5/winitfp.o := CFLAGS += -A-R

obj/fpa_81comp.o shared/fpa_81comp.o s5/fpa_81comp.o  := ASFLAGS += -R

obj/mon.o obj_p/mon.o shared/mon.o \
s5/mon.o s5_p/mon.o:	$(MONDIR)/mon.c $(MONDIR)/mon.h 
	$(CC) -c $(CFLAGS) $(MONDIR)/mon.c -o $@

obj/alloc_pbuf.o obj_p/alloc_pbuf.o shared/alloc_pbuf.o \
s5/alloc_pbuf.o s5_p/alloc_pbuf.o:	$(MONDIR)/alloc_pbuf.c 
	$(CC) -c $(CFLAGS) $(MONDIR)/alloc_pbuf.c -o $@
