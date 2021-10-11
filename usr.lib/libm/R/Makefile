#
# @(#)Makefile 1.1 92/07/30 SMI;
#

#	libm R directory Makefile
#	for machine-independent single-precision Fortran calling convention functions

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
ir_fp_class_.c	r_atanh_.c	r_exp_.c	r_log10_.c	r_sinh_.c \
r_acos_.c	r_bessel_.c	r_expm1_.c	r_log1p_.c	r_sqrt_.c \
r_acosh_.c	r_cbrt_.c	r_fabs_.c	r_log2_.c	r_tanh_.c \
r_asin_.c	r_cosh_.c	r_fmod_.c	r_log_.c	r_trig_.c \
r_asinh_.c	r_erf_.c	r_hypot_.c	r_pow_.c \
r_atan2_.c	r_exp10_.c	r_ieee_func_.c	r_remainder_.c \
r_atan_.c	r_exp2_.c	r_lgamma_.c	r_rint_.c	r_ieee_test_.c \
r_trigpi_.c	r_asinpi_.c	r_acospi_.c	r_atanpi_.c 	r_atan2pi_.c \
r_ieee_vals_.c	r_compound_.c	r_annuity_.c	_tbl_r_atan_.c \
_tbl_r_log_.c


#	These are machine-specific objects of machine-independent sources
#	from SRCS, but they may be replaced by machine-dependent versions later:

OBJS = 	\
$(MIDIR)/ir_fp_class_.o	 $(MIDIR)/r_erf_.o	  $(MIDIR)/r_log1p_.o \
$(MIDIR)/r_acos_.o	 $(MIDIR)/r_exp10_.o	  $(MIDIR)/r_log2_.o \
$(MIDIR)/r_acosh_.o	 $(MIDIR)/r_exp2_.o	  $(MIDIR)/r_log_.o \
$(MIDIR)/r_asin_.o	 $(MIDIR)/r_exp_.o	  $(MIDIR)/r_pow_.o \
$(MIDIR)/r_asinh_.o	 $(MIDIR)/r_expm1_.o	  $(MIDIR)/r_remainder_.o \
$(MIDIR)/r_atan2_.o	 $(MIDIR)/r_fabs_.o	  $(MIDIR)/r_rint_.o \
$(MIDIR)/r_atan_.o	 $(MIDIR)/r_fmod_.o	  $(MIDIR)/r_sinh_.o \
$(MIDIR)/r_atanh_.o	 $(MIDIR)/r_hypot_.o	  $(MIDIR)/r_sqrt_.o \
$(MIDIR)/r_bessel_.o	 $(MIDIR)/r_ieee_func_.o  $(MIDIR)/r_tanh_.o \
$(MIDIR)/r_cbrt_.o	 $(MIDIR)/r_lgamma_.o	  $(MIDIR)/r_trig_.o \
$(MIDIR)/r_cosh_.o	 $(MIDIR)/r_log10_.o 	  $(MIDIR)/r_ieee_test_.o \
$(MIDIR)/r_ieee_vals_.o  $(MIDIR)/r_trigpi_.o	  $(MIDIR)/r_asinpi_.o \
$(MIDIR)/r_acospi_.o	 $(MIDIR)/r_atanpi_.o	  $(MIDIR)/r_atan2pi_.o \
$(MIDIR)/r_compound_.o	 $(MIDIR)/r_annuity_.o

TBLS = $(MIDIR)/_tbl_r_atan_.o \
$(MIDIR)/_tbl_r_log_.o

.DEFAULT:
	sccs get -G$@ $@

all:	$(SRCS) $(OBJS) $(TBLS) $(LIB)
	@echo done making R for $(LIB)

$(LIB):	$(OBJS)
	ar cru $(LIB) $(OBJS)

$(OBJS):
	$(CC) $(CFLAGS) -fsingle -DIEEE -c -$(FP) `basename $(@F) .o`.c $(IL) -p
	$(LD) $(PLDFLAG) -r $(@F)
	mv a.out $(@F)
	mv $(@F) $(MIDIR)/profiled
	$(CC) $(CFLAGS) -fsingle -DIEEE -c -$(FP) `basename $(@F) .o`.c $(IL)
	$(LD) -x -r $(@F)
	mv a.out $(@F)
	mv $(@F) $(MIDIR)

$(TBLS):
	$(CC) -R -c `basename $(@F) .o`.c
	cp $(@F) $(MIDIR)/profiled
	mv $(@F) $(MIDIR)

FORCEMAKE:
$(MIDIR)/ir_fp_class_.o: ir_fp_class_.c $(IL)
$(MIDIR)/r_acos_.o: r_acos_.c $(IL)
$(MIDIR)/r_acosh_.o: r_acosh_.c $(IL)
$(MIDIR)/r_acospi_.o: r_acospi_.c $(IL)
$(MIDIR)/r_annuity_.o: r_annuity_.c $(IL)
$(MIDIR)/r_asin_.o: r_asin_.c $(IL)
$(MIDIR)/r_asinh_.o: r_asinh_.c $(IL)
$(MIDIR)/r_asinpi_.o: r_asinpi_.c $(IL)
$(MIDIR)/r_atan2_.o: r_atan2_.c $(IL)
$(MIDIR)/r_atan2pi_.o: r_atan2pi_.c $(IL)
$(MIDIR)/r_atan_.o: r_atan_.c $(IL)
$(MIDIR)/r_atanpi_.o: r_atanpi_.c $(IL)
$(MIDIR)/r_atanh_.o: r_atanh_.c $(IL)
$(MIDIR)/r_bessel_.o: r_bessel_.c $(IL)
$(MIDIR)/r_cbrt_.o: r_cbrt_.c $(IL)
$(MIDIR)/r_compound_.o: r_compound_.c $(IL)
$(MIDIR)/r_cos_.o: r_cos_.c $(IL)
$(MIDIR)/r_cosh_.o: r_cosh_.c $(IL)
$(MIDIR)/r_erf_.o: r_erf_.c $(IL)
$(MIDIR)/r_exp10_.o: r_exp10_.c $(IL)
$(MIDIR)/r_exp2_.o: r_exp2_.c $(IL)
$(MIDIR)/r_exp_.o: r_exp_.c $(IL)
$(MIDIR)/r_expm1_.o: r_expm1_.c $(IL)
$(MIDIR)/r_fabs_.o: r_fabs_.c $(IL)
$(MIDIR)/r_fmod_.o: r_fmod_.c $(IL)
$(MIDIR)/r_hypot_.o: r_hypot_.c $(IL)
$(MIDIR)/r_ieee_func_.o: r_ieee_func_.c $(IL)
$(MIDIR)/r_ieee_test_.o: r_ieee_test_.c $(IL)
$(MIDIR)/r_ieee_vals_.o: r_ieee_vals_.c $(IL)
$(MIDIR)/r_lgamma_.o: r_lgamma_.c $(IL)
$(MIDIR)/r_log10_.o: r_log10_.c $(IL)
$(MIDIR)/r_log1p_.o: r_log1p_.c $(IL)
$(MIDIR)/r_log2_.o: r_log2_.c $(IL)
$(MIDIR)/r_log_.o: r_log_.c $(IL)
$(MIDIR)/r_pow_.o: r_pow_.c $(IL)
$(MIDIR)/r_remainder_.o: r_remainder_.c $(IL)
$(MIDIR)/r_rint_.o: r_rint_.c $(IL)
$(MIDIR)/r_sin_.o: r_sin_.c $(IL)
$(MIDIR)/r_sinh_.o: r_sinh_.c $(IL)
$(MIDIR)/r_sqrt_.o: r_sqrt_.c $(IL)
$(MIDIR)/r_tan_.o: r_tan_.c $(IL)
$(MIDIR)/r_tanh_.o: r_tanh_.c $(IL)
$(MIDIR)/r_trig_.o: r_trig_.c $(IL)
$(MIDIR)/r_trigpi_.o: r_trigpi_.c $(IL)
