#
# @(#)Makefile 1.1 92/07/30 SMI;
#

#	libm D directory Makefile
#	for machine-independent double-precision Fortran calling convention functions

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

SRCS= 	\
d_acos_.c	d_bessel_.c	d_expm1_.c	d_log1p_.c	d_sqrt_.c \
d_acosh_.c	d_cbrt_.c	d_fabs_.c	d_log2_.c	d_tanh_.c \
d_asin_.c	d_cosh_.c	d_fmod_.c	d_log_.c	d_trig_.c \
d_asinh_.c	d_erf_.c	d_hypot_.c	d_pow_.c	id_fp_class_.c \
d_atan2_.c	d_exp10_.c	d_ieee_func_.c	d_remainder_.c \
d_atan_.c	d_exp2_.c	d_lgamma_.c	d_rint_.c \
d_atanh_.c	d_exp_.c	d_log10_.c	d_sinh_.c	d_ieee_test_.c \
d_trigpi_.c	d_asinpi_.c	d_acospi_.c	d_atanpi_.c	d_atan2pi_.c \
d_ieee_vals_.c	d_compound_.c	d_annuity_.c

#	These are machine-specific objects of machine-independent sources
#	from SRCS, but they may be replaced by machine-dependent versions later:

OBJS = 	\
$(MIDIR)/d_acos_.o	 $(MIDIR)/d_exp10_.o	  $(MIDIR)/d_log2_.o \
$(MIDIR)/d_acosh_.o	 $(MIDIR)/d_exp2_.o	  $(MIDIR)/d_log_.o \
$(MIDIR)/d_asin_.o	 $(MIDIR)/d_exp_.o	  $(MIDIR)/d_pow_.o \
$(MIDIR)/d_asinh_.o	 $(MIDIR)/d_expm1_.o	  $(MIDIR)/d_remainder_.o \
$(MIDIR)/d_atan2_.o	 $(MIDIR)/d_fabs_.o	  $(MIDIR)/d_rint_.o \
$(MIDIR)/d_atan_.o	 $(MIDIR)/d_fmod_.o	  $(MIDIR)/d_sinh_.o \
$(MIDIR)/d_atanh_.o	 $(MIDIR)/d_hypot_.o	  $(MIDIR)/d_sqrt_.o \
$(MIDIR)/d_bessel_.o	 $(MIDIR)/d_ieee_func_.o  $(MIDIR)/d_tanh_.o \
$(MIDIR)/d_cbrt_.o	 $(MIDIR)/d_lgamma_.o	  $(MIDIR)/d_trig_.o \
$(MIDIR)/d_cosh_.o	 $(MIDIR)/d_log10_.o	  $(MIDIR)/id_fp_class_.o \
$(MIDIR)/d_erf_.o	 $(MIDIR)/d_log1p_.o	  $(MIDIR)/d_ieee_test_.o \
$(MIDIR)/d_ieee_vals_.o	 $(MIDIR)/d_trigpi_.o	  $(MIDIR)/d_asinpi_.o \
$(MIDIR)/d_acospi_.o	 $(MIDIR)/d_atanpi_.o	  $(MIDIR)/d_atan2pi_.o \
$(MIDIR)/d_compound_.o	 $(MIDIR)/d_annuity_.o


.DEFAULT:
	sccs get -G$@ $@

all:	$(SRCS) $(OBJS) $(LIB)
	@echo done making D for $(LIB)

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

FORCEMAKE:
$(MIDIR)/d_acos_.o: d_acos_.c /usr/include/math.h $(IL)
$(MIDIR)/d_acosh_.o: d_acosh_.c /usr/include/math.h $(IL)
$(MIDIR)/d_acospi_.o: d_acospi_.c /usr/include/math.h $(IL)
$(MIDIR)/d_annuity_.o: d_annuity_.c /usr/include/math.h $(IL)
$(MIDIR)/d_asin_.o: d_asin_.c /usr/include/math.h $(IL)
$(MIDIR)/d_asinh_.o: d_asinh_.c /usr/include/math.h $(IL)
$(MIDIR)/d_asinpi_.o: d_asinpi_.c /usr/include/math.h $(IL)
$(MIDIR)/d_atan2_.o: d_atan2_.c /usr/include/math.h $(IL)
$(MIDIR)/d_atan2pi_.o: d_atan2pi_.c /usr/include/math.h $(IL)
$(MIDIR)/d_atan_.o: d_atan_.c /usr/include/math.h $(IL)
$(MIDIR)/d_atanh_.o: d_atanh_.c /usr/include/math.h $(IL)
$(MIDIR)/d_atanpi_.o: d_atanpi_.c /usr/include/math.h $(IL)
$(MIDIR)/d_bessel_.o: d_bessel_.c /usr/include/math.h $(IL)
$(MIDIR)/d_cbrt_.o: d_cbrt_.c /usr/include/math.h $(IL)
$(MIDIR)/d_compound_.o: d_compound_.c /usr/include/math.h $(IL)
$(MIDIR)/d_cosh_.o: d_cosh_.c /usr/include/math.h $(IL)
$(MIDIR)/d_erf_.o: d_erf_.c /usr/include/math.h $(IL)
$(MIDIR)/d_exp10_.o: d_exp10_.c /usr/include/math.h $(IL)
$(MIDIR)/d_exp2_.o: d_exp2_.c /usr/include/math.h $(IL)
$(MIDIR)/d_exp_.o: d_exp_.c /usr/include/math.h $(IL)
$(MIDIR)/d_expm1_.o: d_expm1_.c /usr/include/math.h $(IL)
$(MIDIR)/d_fabs_.o: d_fabs_.c /usr/include/math.h $(IL)
$(MIDIR)/d_fmod_.o: d_fmod_.c /usr/include/math.h $(IL)
$(MIDIR)/d_hypot_.o: d_hypot_.c /usr/include/math.h $(IL)
$(MIDIR)/d_ieee_func_.o: d_ieee_func_.c /usr/include/math.h $(IL)
$(MIDIR)/d_ieee_test_.o: d_ieee_test_.c /usr/include/math.h $(IL)
$(MIDIR)/d_ieee_vals_.o: d_ieee_vals_.c /usr/include/math.h $(IL)
$(MIDIR)/d_lgamma_.o: d_lgamma_.c /usr/include/math.h $(IL)
$(MIDIR)/d_log10_.o: d_log10_.c /usr/include/math.h $(IL)
$(MIDIR)/d_log1p_.o: d_log1p_.c /usr/include/math.h $(IL)
$(MIDIR)/d_log2_.o: d_log2_.c /usr/include/math.h $(IL)
$(MIDIR)/d_log_.o: d_log_.c /usr/include/math.h $(IL)
$(MIDIR)/d_pow_.o: d_pow_.c /usr/include/math.h $(IL)
$(MIDIR)/d_remainder_.o: d_remainder_.c /usr/include/math.h $(IL)
$(MIDIR)/d_rint_.o: d_rint_.c /usr/include/math.h $(IL)
$(MIDIR)/d_sinh_.o: d_sinh_.c /usr/include/math.h $(IL)
$(MIDIR)/d_sqrt_.o: d_sqrt_.c /usr/include/math.h $(IL)
$(MIDIR)/d_tanh_.o: d_tanh_.c /usr/include/math.h $(IL)
$(MIDIR)/d_trig_.o: d_trig_.c /usr/include/math.h $(IL)
$(MIDIR)/d_trigpi_.o: d_trigpi_.c /usr/include/math.h $(IL)
$(MIDIR)/id_fp_class_.o: id_fp_class_.c /usr/include/math.h $(IL)
