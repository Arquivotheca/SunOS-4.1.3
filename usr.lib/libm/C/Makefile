#
# @(#)Makefile 1.1 92/07/30 SMI;
#
# Copyright (c) 1988 by Sun Microsystems, Inc.
#

#	libm C directory Makefile
#	for machine-independent double-precision C calling convention functions

#	The following are expected to be defined outside this makefile:

#	FP=		floating-point option
#	../OBJDIR=	name of subdirectory to build libraries
#	IL=		inline expansion template file

#	The target libraries for this version:

OBJD=	$(WD)/../$(OBJDIR)
LIB = 	$(OBJD)/libm.a
LIBP = 	$(OBJD)/libm_p.a
MIDIR=  $(OBJD)/midir

#	These machine-independent sources are local:

HDRS=	libm.h

SRCS= 	_swapFLAGS.c \
SVID_libm_err.c	cabs.c		fabs.c		log.c		rint.c \
acos.c		cbrt.c		fmod.c		log10.c		sinh.c \
acosh.c		fp_class.c	frexp.c		log1p.c		sqrt.c \
asin.c		cosh.c		hypot.c		log2.c		tanh.c \
asinh.c		erf.c		ieee_flags.c	matherr.c	trig.c \
atan.c		exp.c		ieee_func.c	modf.c 		underflow.c \
atan2.c		exp10.c		ieee_handler.c	pow.c \
atanh.c		exp2.c		ldexp.c \
bessel.c	expm1.c		lgamma.c	remainder.c \
ieee_environ.c	ieee_retro.c	ieee_test.c	ieee_vals.c \
trigpi.c	asinpi.c	acospi.c	atanpi.c	atan2pi.c  \
compound.c	annuity.c	ieee_std.c	_tbl_atan.c \
_tbl_log.c  	release.c	__infinity.c

#	These are machine-specific objects of machine-independent sources
#	from SRCS, but they may be replaced by machine-dependent versions later:

OBJS = 	$(MIDIR)/_swapFLAGS.o \
$(MIDIR)/SVID_libm_err.o  $(MIDIR)/exp.o	    $(MIDIR)/log10.o \
$(MIDIR)/acos.o		  $(MIDIR)/exp10.o	    $(MIDIR)/log1p.o \
$(MIDIR)/acosh.o	  $(MIDIR)/exp2.o	    $(MIDIR)/log2.o \
$(MIDIR)/asin.o		  $(MIDIR)/expm1.o	    $(MIDIR)/matherr.o \
$(MIDIR)/asinh.o	  $(MIDIR)/fabs.o	    $(MIDIR)/modf.o \
$(MIDIR)/atan.o		  $(MIDIR)/fmod.o	    $(MIDIR)/pow.o \
$(MIDIR)/atan2.o	  $(MIDIR)/frexp.o \
$(MIDIR)/atanh.o	  $(MIDIR)/hypot.o	    $(MIDIR)/remainder.o \
$(MIDIR)/bessel.o	  $(MIDIR)/ieee_flags.o	    $(MIDIR)/rint.o \
$(MIDIR)/cabs.o		  $(MIDIR)/ieee_func.o	    $(MIDIR)/sinh.o \
$(MIDIR)/cbrt.o		  $(MIDIR)/ieee_handler.o   $(MIDIR)/sqrt.o \
$(MIDIR)/fp_class.o	  $(MIDIR)/ldexp.o	    $(MIDIR)/tanh.o \
$(MIDIR)/cosh.o		  $(MIDIR)/lgamma.o	    $(MIDIR)/trig.o \
$(MIDIR)/erf.o		  $(MIDIR)/log.o	    $(MIDIR)/ieee_environ.o \
$(MIDIR)/ieee_test.o	  $(MIDIR)/underflow.o	    $(MIDIR)/ieee_retro.o \
$(MIDIR)/ieee_vals.o	  $(MIDIR)/trigpi.o	    $(MIDIR)/asinpi.o \
$(MIDIR)/acospi.o	  $(MIDIR)/atanpi.o	    $(MIDIR)/atan2pi.o \
$(MIDIR)/compound.o	  $(MIDIR)/annuity.o	    $(MIDIR)/ieee_std.o  \
$(MIDIR)/release.o	  $(MIDIR)/__infinity.o

TBLS = $(MIDIR)/_tbl_atan.o \
$(MIDIR)/_tbl_log.o


.DEFAULT:
	sccs get -G$@ $@

all:	$(HDRS) $(SRCS) $(OBJS) $(TBLS) $(LIB)
	@echo done making C for $(LIB)

$(LIB): $(OBJS)
	ar cru $(LIB) $(OBJS)

$(OBJS):
	$(CC) $(CFLAGS) -DIEEE -c -$(FP) `basename $(@F) .o`.c $(IL) -p
	$(LD) $(PLDFLAG) -r $(@F)
	mv a.out $(@F)
	mv $(@F) $(MIDIR)/profiled
	$(CC) $(CFLAGS) -DIEEE -c -$(FP) `basename $(@F) .o`.c $(IL)
	$(LD) -x -r $(@F)
	mv a.out $(@F)
	mv $(@F) $(MIDIR)

$(TBLS):
	$(CC) -R -c `basename $(@F) .o`.c
	cp $(@F) $(MIDIR)/profiled
	mv $(@F) $(MIDIR)

FORCEMAKE:

$(MIDIR)/_swapFLAGS.o: _swapFLAGS.c libm.h /usr/include/math.h $(IL) 
$(MIDIR)/SVID_libm_err.o: SVID_libm_err.c libm.h /usr/include/math.h $(IL)
$(MIDIR)/acos.o: acos.c libm.h /usr/include/math.h $(IL)
$(MIDIR)/acosh.o: acosh.c libm.h /usr/include/math.h $(IL)
$(MIDIR)/acospi.o: acospi.c libm.h /usr/include/math.h $(IL)
$(MIDIR)/annuity.o: annuity.c libm.h /usr/include/math.h $(IL)
$(MIDIR)/asin.o: asin.c libm.h /usr/include/math.h $(IL)
$(MIDIR)/asinh.o: asinh.c libm.h /usr/include/math.h $(IL)
$(MIDIR)/asinpi.o: asinpi.c libm.h /usr/include/math.h $(IL)
$(MIDIR)/atan.o: atan.c libm.h /usr/include/math.h $(IL)
$(MIDIR)/atanpi.o: atanpi.c libm.h /usr/include/math.h $(IL)
$(MIDIR)/atan2.o: atan2.c libm.h /usr/include/math.h $(IL)
$(MIDIR)/atan2pi.o: atan2pi.c libm.h /usr/include/math.h $(IL)
$(MIDIR)/atanh.o: atanh.c libm.h /usr/include/math.h $(IL)
$(MIDIR)/bessel.o: bessel.c libm.h /usr/include/math.h $(IL)
$(MIDIR)/cabs.o: cabs.c libm.h /usr/include/math.h $(IL)
$(MIDIR)/cbrt.o: cbrt.c libm.h /usr/include/math.h $(IL)
$(MIDIR)/compound.o: compound.c libm.h /usr/include/math.h $(IL)
$(MIDIR)/fp_class.o: fp_class.c libm.h /usr/include/math.h $(IL)
$(MIDIR)/cosh.o: cosh.c libm.h /usr/include/math.h $(IL)
$(MIDIR)/erf.o: erf.c libm.h /usr/include/math.h $(IL)
$(MIDIR)/exp.o: exp.c libm.h /usr/include/math.h $(IL)
$(MIDIR)/exp10.o: exp10.c libm.h /usr/include/math.h $(IL)
$(MIDIR)/exp2.o: exp2.c libm.h /usr/include/math.h $(IL)
$(MIDIR)/expm1.o: expm1.c libm.h /usr/include/math.h $(IL)
$(MIDIR)/fabs.o: fabs.c libm.h /usr/include/math.h $(IL)
$(MIDIR)/fmod.o: fmod.c libm.h /usr/include/math.h $(IL)
$(MIDIR)/frexp.o: frexp.c libm.h /usr/include/math.h $(IL)
$(MIDIR)/hypot.o: hypot.c libm.h /usr/include/math.h $(IL)
$(MIDIR)/ieee_environ.o: ieee_environ.c /usr/include/floatingpoint.h
$(MIDIR)/ieee_flags.o: ieee_flags.c libm.h /usr/include/math.h $(IL)
$(MIDIR)/ieee_func.o: ieee_func.c libm.h /usr/include/math.h $(IL)
$(MIDIR)/ieee_handler.o: ieee_handler.c libm.h /usr/include/math.h $(IL)
$(MIDIR)/ieee_retro.o: ieee_retro.c libm.h /usr/include/math.h $(IL)
$(MIDIR)/ieee_std.o: ieee_std.c
$(MIDIR)/ieee_test.o: ieee_test.c libm.h /usr/include/math.h $(IL)
$(MIDIR)/ieee_vals.o: ieee_vals.c libm.h /usr/include/math.h $(IL)
$(MIDIR)/ldexp.o: ldexp.c libm.h /usr/include/math.h $(IL)
$(MIDIR)/lgamma.o: lgamma.c libm.h /usr/include/math.h $(IL)
$(MIDIR)/log.o: log.c libm.h /usr/include/math.h $(IL)
$(MIDIR)/log10.o: log10.c libm.h /usr/include/math.h $(IL)
$(MIDIR)/log1p.o: log1p.c libm.h /usr/include/math.h $(IL)
$(MIDIR)/log2.o: log2.c libm.h /usr/include/math.h $(IL)
$(MIDIR)/matherr.o: matherr.c libm.h /usr/include/math.h $(IL)
$(MIDIR)/modf.o: modf.c libm.h /usr/include/math.h $(IL)
$(MIDIR)/pow.o: pow.c libm.h /usr/include/math.h $(IL)
$(MIDIR)/remainder.o: remainder.c libm.h /usr/include/math.h $(IL)
$(MIDIR)/rint.o: rint.c libm.h /usr/include/math.h $(IL)
$(MIDIR)/sinh.o: sinh.c libm.h /usr/include/math.h $(IL)
$(MIDIR)/sqrt.o: sqrt.c libm.h /usr/include/math.h $(IL)
$(MIDIR)/tanh.o: tanh.c libm.h /usr/include/math.h $(IL)
$(MIDIR)/trig.o: trig.c libm.h /usr/include/math.h $(IL)
$(MIDIR)/trigpi.o: trigpi.c libm.h /usr/include/math.h $(IL)
