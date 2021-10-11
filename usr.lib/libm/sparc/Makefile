#
# @(#)Makefile	1.1 7/30/92 SMI;
#

# libm/sparc machine-dependent makefile

.SUFFIXES:	.S .S~ .il .il~

CPP=	/lib/cpp

LIB=	libm.a
LIBP=	libm_p.a

# directory for objects from machine-independent sources
MIDIR=	midir

IL=	libm.il

SRCS=	_swapFLAGS.S \
Facos.S         Fatan.S         Fexp10.S        Fnint.S         Ftan.S\
Faint.S         Fcos.S          Fexp2.S         Fsin.S          Ftanh.S\
Fanint.S        Fcosh.S         Flog.S          Fsinh.S\
Fasin.S         Fexp.S          Flog10.S        Fsqrt.S\
fabs.S		log.S		remainder.S	trig.S \
atan.S		fmod.S		r_sqrt_.S	rint.S 		r_atan_.S \
exp.S		ieee_func.S	sqrt.S		r_trig_.S 	r_pow_.S \
r_exp_.S	r_log_.S	ieee_std.S	r_fmod_.S

OBJS=	_swapFLAGS.o \
Facos.o         Fatan.o         Fexp10.o        Fnint.o         Ftan.o\
Faint.o         Fcos.o          Fexp2.o         Fsin.o          Ftanh.o\
Fanint.o        Fcosh.o         Flog.o          Fsinh.o\
Fasin.o         Fexp.o          Flog10.o        Fsqrt.o\
fabs.o		log.o		remainder.o	trig.o \
atan.o		fmod.o		r_sqrt_.o	rint.o 		r_atan_.o \
exp.o		ieee_func.o	sqrt.o		r_trig_.o 	r_pow_.o \
r_exp_.o	r_log_.o	ieee_std.o	r_fmod_.o

.DEFAULT:
	sccs get -G$@ $@

all:    $(IL) $(SRCS) $(OBJS) $(LIB)

.il~.il:
	sccs get -G$@ $@

.S~.S:
	sccs get -G$@ $@

$(LIB): $(OBJS)
	ar cru $(LIB) $(OBJS)
 
.S.o:
	$(CC) $(ASFLAGS) -c -DPROF $(IL) $*.S -o ../$*.o 
	${LD} $(PLDFLAG) -r ../$*.o
	mv a.out ../$*.o
	mv ../$*.o midir/profiled
	$(CC) $(ASFLAGS) -c $(IL) $*.S -o ../$*.o 
	${LD} -x -r ../$*.o
	rm ../$*.o
	mv a.out $*.o

clean:
	rm -rf a.out core errs *.a *.o *.stamp $(MIDIR)

FORCEMAKE:

Facos.o:	 	Facos.S midir/Facos.o
Faint.o:	 	Faint.S midir/Faint.o
Fanint.o:	 	Fanint.S midir/Fanint.o
Fasin.o:	 	Fasin.S midir/Fasin.o
Fatan.o:	 	Fatan.S midir/Fatan.o
Fcos.o:	 		Fcos.S midir/Fcos.o
Fcosh.o:	 	Fcosh.S midir/Fcosh.o
Fexp.o:	 		Fexp.S midir/Fexp.o
Fexp10.o:	 	Fexp10.S midir/Fexp10.o
Fexp2.o:	 	Fexp2.S midir/Fexp2.o
Flog.o:	 		Flog.S midir/Flog.o
Flog10.o:	 	Flog10.S midir/Flog10.o
Fnint.o:	 	Fnint.S midir/Fnint.o
Fsin.o:	 		Fsin.S midir/Fsin.o
Fsinh.o:	 	Fsinh.S midir/Fsinh.o
Fsqrt.o:	 	Fsqrt.S midir/Fsqrt.o
Ftan.o:	 		Ftan.S midir/Ftan.o
Ftanh.o:	 	Ftanh.S midir/Ftanh.o
_swapFLAGS.o:	 	_swapFLAGS.S midir/_swapFLAGS.o
atan.o:	 		atan.S midir/atan.o
exp.o:	 		exp.S midir/exp.o
fabs.o:	 		fabs.S midir/fabs.o
fmod.o:	 		fmod.S midir/fmod.o
ieee_func.o:	 	ieee_func.S midir/ieee_func.o
log.o:	 		log.S midir/log.o
r_atan_.o:	 	r_atan_.S midir/r_atan_.o
r_exp_.o:	 	r_exp_.S midir/r_exp_.o
r_fmod_.o:	 	r_fmod_.S midir/r_fmod_.o
r_log_.o:	 	r_log_.S midir/r_log_.o
r_sqrt_.o:	 	r_sqrt_.S midir/r_sqrt_.o
r_trig_.o:	 	r_trig_.S midir/r_trig_.o
remainder.o:	 	remainder.S midir/remainder.o
rint.o:	 		rint.S midir/rint.o
sqrt.o:	 		sqrt.S midir/sqrt.o
trig.o:	 		trig.S midir/trig.o
ieee_std.o:	 	ieee_std.S midir/ieee_std.o
r_pow_.o:		r_pow_.S midir/r_pow_.o
