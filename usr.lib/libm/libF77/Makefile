#
# @(#)Makefile 1.1 92/07/30 SMI;
#

#	libm libF77 directory Makefile
#	for libF77 routines in libm for efficiency

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

HDRS=	complex.h oldcomplex.h

SRCS=	\
	c_abs.c c_compare.c c_cos.c \
	c_exp.c c_log.c c_sin.c \
	c_sqrt.c complex.c d_abs.c \
	d_acos.c d_asin.c d_atan.c d_atn2.c \
	d_cnjg.c d_cos.c d_cosh.c d_dim.c \
	d_exp.c d_imag.c d_int.c d_lg10.c \
	d_log.c d_mod.c d_nint.c d_prod.c \
	d_sign.c d_sin.c d_sinh.c d_sqrt.c \
	d_tan.c d_tanh.c \
	d_acosd.c  d_atand.c  d_cosd.c   d_tand.c \
	d_asind.c  d_atn2d.c  d_sind.c  \
	extra.c h_dnnt.c h_nint.c i_dnnt.c \
	i_nint.c pow_cc.c pow_ci.c pow_dd.c \
	pow_di.c pow_ri.c pow_rr.c pow_zi.c \
	pow_zz.c r_abs.c r_acos.c r_asin.c \
	r_atan.c r_atn2.c r_cnjg.c r_cos.c \
	r_cosh.c r_dim.c r_exp.c r_imag.c \
	r_int.c r_lg10.c r_log.c r_mod.c \
	r_nint.c r_sign.c r_sin.c r_sinh.c \
	r_sqrt.c r_tan.c r_tanh.c rand_.c \
	r_acosd.c  r_atand.c  r_cosd.c   r_tand.c \
	r_asind.c  r_atn2d.c  r_sind.c \
	z_abs.c z_compare.c z_cos.c z_exp.c \
	z_log.c z_sin.c z_sqrt.c zomplex.c \
	dtime_.c etime_.c fp_convert.c \
	oldcomplex.c oldzomplex.c oldc_compare.c oldc_div.c

#	These are machine-specific objects of machine-independent sources
#	from SRCS, but they may be replaced by machine-dependent versions later:

OBJS = 	\
	$(MIDIR)/c_abs.o $(MIDIR)/c_compare.o $(MIDIR)/c_cos.o \
	$(MIDIR)/c_exp.o $(MIDIR)/c_log.o $(MIDIR)/c_sin.o \
	$(MIDIR)/c_sqrt.o $(MIDIR)/complex.o $(MIDIR)/d_abs.o \
	$(MIDIR)/d_acos.o $(MIDIR)/d_asin.o $(MIDIR)/d_atan.o $(MIDIR)/d_atn2.o \
	$(MIDIR)/d_cnjg.o $(MIDIR)/d_cos.o $(MIDIR)/d_cosh.o $(MIDIR)/d_dim.o \
	$(MIDIR)/d_exp.o $(MIDIR)/d_imag.o $(MIDIR)/d_int.o $(MIDIR)/d_lg10.o \
	$(MIDIR)/d_log.o $(MIDIR)/d_mod.o $(MIDIR)/d_nint.o $(MIDIR)/d_prod.o \
	$(MIDIR)/d_sign.o $(MIDIR)/d_sin.o $(MIDIR)/d_sinh.o $(MIDIR)/d_sqrt.o \
	$(MIDIR)/d_tan.o $(MIDIR)/d_tanh.o \
	$(MIDIR)/d_acosd.o $(MIDIR)/d_asind.o $(MIDIR)/d_atand.o \
	$(MIDIR)/d_atn2d.o $(MIDIR)/d_cosd.o $(MIDIR)/d_sind.o $(MIDIR)/d_tand.o \
	$(MIDIR)/extra.o $(MIDIR)/h_dnnt.o $(MIDIR)/h_nint.o $(MIDIR)/i_dnnt.o \
	$(MIDIR)/i_nint.o $(MIDIR)/pow_cc.o $(MIDIR)/pow_ci.o $(MIDIR)/pow_dd.o \
	$(MIDIR)/pow_di.o $(MIDIR)/pow_ri.o $(MIDIR)/pow_rr.o $(MIDIR)/pow_zi.o \
	$(MIDIR)/pow_zz.o $(MIDIR)/r_abs.o $(MIDIR)/r_acos.o $(MIDIR)/r_asin.o \
	$(MIDIR)/r_atan.o $(MIDIR)/r_atn2.o $(MIDIR)/r_cnjg.o $(MIDIR)/r_cos.o \
	$(MIDIR)/r_cosh.o $(MIDIR)/r_dim.o $(MIDIR)/r_exp.o $(MIDIR)/r_imag.o \
	$(MIDIR)/r_int.o $(MIDIR)/r_lg10.o $(MIDIR)/r_log.o $(MIDIR)/r_mod.o \
	$(MIDIR)/r_nint.o $(MIDIR)/r_sign.o $(MIDIR)/r_sin.o $(MIDIR)/r_sinh.o \
	$(MIDIR)/r_sqrt.o $(MIDIR)/r_tan.o $(MIDIR)/r_tanh.o $(MIDIR)/rand_.o \
	$(MIDIR)/r_acosd.o $(MIDIR)/r_asind.o $(MIDIR)/r_atand.o \
	$(MIDIR)/r_atn2d.o $(MIDIR)/r_cosd.o $(MIDIR)/r_sind.o $(MIDIR)/r_tand.o \
	$(MIDIR)/z_abs.o $(MIDIR)/z_compare.o \
	$(MIDIR)/z_cos.o $(MIDIR)/z_exp.o \
	$(MIDIR)/z_log.o $(MIDIR)/z_sin.o $(MIDIR)/z_sqrt.o $(MIDIR)/zomplex.o \
	$(MIDIR)/dtime_.o $(MIDIR)/etime_.o $(MIDIR)/fp_convert.o \
	$(MIDIR)/oldcomplex.o $(MIDIR)/oldzomplex.o $(MIDIR)/oldc_compare.o \
	$(MIDIR)/oldc_div.o

.DEFAULT:
	sccs get -G$@ $@

all:	$(HDRS) $(SRCS) $(OBJS) $(LIB)
	@echo done making libF77 for $(LIB)

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

$(MIDIR)/c_abs.o: c_abs.c complex.h /usr/include/math.h $(IL)
$(MIDIR)/c_compare.o: c_compare.c complex.h /usr/include/math.h $(IL)
$(MIDIR)/c_cos.o: c_cos.c complex.h /usr/include/math.h $(IL)
$(MIDIR)/c_exp.o: c_exp.c complex.h /usr/include/math.h $(IL)
$(MIDIR)/c_log.o: c_log.c complex.h /usr/include/math.h $(IL)
$(MIDIR)/c_sin.o: c_sin.c complex.h /usr/include/math.h $(IL)
$(MIDIR)/c_sqrt.o: c_sqrt.c complex.h /usr/include/math.h $(IL)
$(MIDIR)/complex.o: complex.c complex.h /usr/include/math.h $(IL)
$(MIDIR)/d_abs.o: d_abs.c /usr/include/math.h $(IL)
$(MIDIR)/d_acos.o: d_acos.c /usr/include/math.h $(IL)
$(MIDIR)/d_asin.o: d_asin.c /usr/include/math.h $(IL)
$(MIDIR)/d_atan.o: d_atan.c /usr/include/math.h $(IL)
$(MIDIR)/d_atn2.o: d_atn2.c /usr/include/math.h $(IL)
$(MIDIR)/d_cnjg.o: d_cnjg.c complex.h /usr/include/math.h $(IL)
$(MIDIR)/d_cos.o: d_cos.c /usr/include/math.h $(IL)
$(MIDIR)/d_cosh.o: d_cosh.c /usr/include/math.h $(IL)
$(MIDIR)/d_dim.o: d_dim.c /usr/include/math.h $(IL)
$(MIDIR)/d_exp.o: d_exp.c /usr/include/math.h $(IL)
$(MIDIR)/d_imag.o: d_imag.c complex.h /usr/include/math.h $(IL)
$(MIDIR)/d_int.o: d_int.c /usr/include/math.h $(IL)
$(MIDIR)/d_lg10.o: d_lg10.c /usr/include/math.h $(IL)
$(MIDIR)/d_log.o: d_log.c /usr/include/math.h $(IL)
$(MIDIR)/d_mod.o: d_mod.c /usr/include/math.h $(IL)
$(MIDIR)/d_nint.o: d_nint.c /usr/include/math.h $(IL)
$(MIDIR)/d_prod.o: d_prod.c /usr/include/math.h $(IL)
$(MIDIR)/d_sign.o: d_sign.c /usr/include/math.h $(IL)
$(MIDIR)/d_sin.o: d_sin.c /usr/include/math.h $(IL)
$(MIDIR)/d_sinh.o: d_sinh.c /usr/include/math.h $(IL)
$(MIDIR)/d_sqrt.o: d_sqrt.c /usr/include/math.h $(IL)
$(MIDIR)/d_tan.o: d_tan.c /usr/include/math.h $(IL)
$(MIDIR)/d_tanh.o: d_tanh.c /usr/include/math.h $(IL)
$(MIDIR)/d_acosd.o: d_acosd.c /usr/include/math.h $(IL)
$(MIDIR)/d_asind.o: d_asind.c /usr/include/math.h $(IL)
$(MIDIR)/d_atand.o: d_atand.c /usr/include/math.h $(IL)
$(MIDIR)/d_atn2d.o: d_atn2d.c /usr/include/math.h $(IL)
$(MIDIR)/d_cosd.o: d_cosd.c /usr/include/math.h $(IL)
$(MIDIR)/d_sind.o: d_sind.c /usr/include/math.h $(IL)
$(MIDIR)/d_tand.o: d_tand.c /usr/include/math.h $(IL)
$(MIDIR)/extra.o: extra.c complex.h /usr/include/math.h $(IL)
$(MIDIR)/h_dnnt.o: h_dnnt.c /usr/include/math.h $(IL)
$(MIDIR)/h_nint.o: h_nint.c /usr/include/math.h $(IL)
$(MIDIR)/i_dnnt.o: i_dnnt.c /usr/include/math.h $(IL)
$(MIDIR)/i_nint.o: i_nint.c /usr/include/math.h $(IL)
$(MIDIR)/pow_cc.o: pow_cc.c complex.h /usr/include/math.h $(IL)
$(MIDIR)/pow_ci.o: pow_ci.c complex.h /usr/include/math.h $(IL)
$(MIDIR)/pow_dd.o: pow_dd.c /usr/include/math.h $(IL)
$(MIDIR)/pow_di.o: pow_di.c /usr/include/math.h $(IL)
$(MIDIR)/pow_ri.o: pow_ri.c /usr/include/math.h $(IL)
$(MIDIR)/pow_rr.o: pow_rr.c /usr/include/math.h $(IL)
$(MIDIR)/pow_zi.o: pow_zi.c complex.h /usr/include/math.h $(IL)
$(MIDIR)/pow_zz.o: pow_zz.c complex.h /usr/include/math.h $(IL)
$(MIDIR)/r_abs.o: r_abs.c /usr/include/math.h $(IL)
$(MIDIR)/r_acos.o: r_acos.c /usr/include/math.h $(IL)
$(MIDIR)/r_asin.o: r_asin.c /usr/include/math.h $(IL)
$(MIDIR)/r_atan.o: r_atan.c /usr/include/math.h $(IL)
$(MIDIR)/r_atn2.o: r_atn2.c /usr/include/math.h $(IL)
$(MIDIR)/r_cnjg.o: r_cnjg.c complex.h /usr/include/math.h $(IL)
$(MIDIR)/r_cos.o: r_cos.c /usr/include/math.h $(IL)
$(MIDIR)/r_cosh.o: r_cosh.c /usr/include/math.h $(IL)
$(MIDIR)/r_dim.o: r_dim.c /usr/include/math.h $(IL)
$(MIDIR)/r_exp.o: r_exp.c /usr/include/math.h $(IL)
$(MIDIR)/r_imag.o: r_imag.c complex.h /usr/include/math.h $(IL)
$(MIDIR)/r_int.o: r_int.c /usr/include/math.h $(IL)
$(MIDIR)/r_lg10.o: r_lg10.c /usr/include/math.h $(IL)
$(MIDIR)/r_log.o: r_log.c /usr/include/math.h $(IL)
$(MIDIR)/r_mod.o: r_mod.c /usr/include/math.h $(IL)
$(MIDIR)/r_nint.o: r_nint.c /usr/include/math.h $(IL)
$(MIDIR)/r_sign.o: r_sign.c /usr/include/math.h $(IL)
$(MIDIR)/r_sin.o: r_sin.c /usr/include/math.h $(IL)
$(MIDIR)/r_sinh.o: r_sinh.c /usr/include/math.h $(IL)
$(MIDIR)/r_sqrt.o: r_sqrt.c /usr/include/math.h $(IL)
$(MIDIR)/r_tan.o: r_tan.c /usr/include/math.h $(IL)
$(MIDIR)/r_tanh.o: r_tanh.c /usr/include/math.h $(IL)
$(MIDIR)/r_acosd.o: r_acosd.c /usr/include/math.h $(IL)
$(MIDIR)/r_asind.o: r_asind.c /usr/include/math.h $(IL)
$(MIDIR)/r_atand.o: r_atand.c /usr/include/math.h $(IL)
$(MIDIR)/r_atn2d.o: r_atn2d.c /usr/include/math.h $(IL)
$(MIDIR)/r_cosd.o: r_cosd.c /usr/include/math.h $(IL)
$(MIDIR)/r_sind.o: r_sind.c /usr/include/math.h $(IL)
$(MIDIR)/r_tand.o: r_tand.c /usr/include/math.h $(IL)
$(MIDIR)/rand_.o: rand_.c /usr/include/math.h $(IL)
$(MIDIR)/z_abs.o: z_abs.c complex.h /usr/include/math.h $(IL)
$(MIDIR)/z_compare.o: z_compare.c complex.h /usr/include/math.h $(IL)
$(MIDIR)/z_cos.o: z_cos.c complex.h /usr/include/math.h $(IL)
$(MIDIR)/z_exp.o: z_exp.c complex.h /usr/include/math.h $(IL)
$(MIDIR)/z_log.o: z_log.c complex.h /usr/include/math.h $(IL)
$(MIDIR)/z_sin.o: z_sin.c complex.h /usr/include/math.h $(IL)
$(MIDIR)/z_sqrt.o: z_sqrt.c complex.h /usr/include/math.h $(IL)
$(MIDIR)/zomplex.o: zomplex.c complex.h /usr/include/math.h $(IL)
$(MIDIR)/dtime_.o: dtime_.c $(IL)
$(MIDIR)/etime_.o: etime_.c $(IL)
$(MIDIR)/fp_convert.o: fp_convert.c $(IL)
$(MIDIR)/oldcomplex.o: oldcomplex.c oldcomplex.h /usr/include/math.h $(IL)
$(MIDIR)/oldzomplex.o: oldzomplex.c oldcomplex.h /usr/include/math.h $(IL)
$(MIDIR)/oldc_compare.o: oldc_compare.c oldcomplex.h /usr/include/math.h $(IL)
$(MIDIR)/oldc_div.o: oldc_div.c oldcomplex.h /usr/include/math.h $(IL)
