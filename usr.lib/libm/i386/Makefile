#
# @(#)Makefile	1.1 7/30/92 SMI;
#

# libm/i386 machine-dependent makefile

.SUFFIXES:	.s .s~ .il .il~

CPP=	/lib/cpp

LIB=	libm.a
LIBP=	libm_p.a

# directory for objects from machine-independent sources
MIDIR=	midir

IL=	libm.il

SRCS=	_swapFLAGS.s \
	acos.s asin.s atan.s atan2.s exp.s exp10.s exp2.s expm1.s  \
	fabs.s fmod.s hypot.s ieee_func.s log.s log10.s \
	log2.s remainder.s rint.s sqrt.s trig.s \
	r_atan_.s r_exp_.s r_hypot_.s r_log_.s r_sqrt_.s \
	r_sqrt_.s r_trig_.s

OBJS=	_swapFLAGS.o \
	acos.o asin.o atan.o atan2.o exp.o exp10.o exp2.o expm1.o  \
	fabs.o fmod.o hypot.o ieee_func.o log.o log10.o \
	log2.o remainder.o rint.o sqrt.o trig.o \
	r_atan_.o r_exp_.o r_hypot_.o r_log_.o r_sqrt_.o \
	r_sqrt_.o r_trig_.o

.DEFAULT:
	sccs get -G$@ $@

all:    $(IL) $(SRCS) $(OBJS) $(LIB)

.il~.il:
	sccs get -G$@ $@

.s~.s:
	sccs get -G$@ $@

$(LIB): $(OBJS)
	ar cru $(LIB) $(OBJS)
 
.s.o:
	$(CC) $(ASFLAGS) -c -DPROF $(IL) $*.s -o ../$*.o 
	${LD} $(PLDFLAG) -r ../$*.o
	mv a.out ../$*.o
	mv ../$*.o midir/profiled
	$(CC) $(ASFLAGS) -c $(IL) $*.s -o ../$*.o 
	${LD} -x -r ../$*.o
	rm ../$*.o
	mv a.out $*.o

clean:
	rm -rf a.out core errs *.a *.o *.il *.stamp $(MIDIR)

FORCEMAKE:

#typical.o:	 	typical.s midir/typical.o
_swapFLAGS.o:		_swapFLAGS.s midir/_swapFLAGS.o
acos.o:			acos.s midir/acos.o 
asin.o:			asin.s midir/asin.o 
atan.o:			atan.s midir/atan.o 
atan2.o:		atan2.s midir/atan2.o 
exp.o:			exp.s midir/exp.o 
exp10.o:		exp10.s midir/exp10.o 
exp2.o:			exp2.s midir/exp2.o 
expm1.o:		expm1.s midir/expm1.o 
fabs.o:			fabs.s midir/fabs.o 
fmod.o:			fmod.s midir/fmod.o 
hypot.o:		hypot.s midir/hypot.o 
ieee_func.o:		ieee_func.s midir/ieee_func.o 
log.o:			log.s midir/log.o 
log10.o:		log10.s midir/log10.o 
log2.o:			log2.s midir/log2.o 
remainder.o:		remainder.s midir/remainder.o 
rint.o:			rint.s midir/rint.o
sqrt.o:			sqrt.s midir/sqrt.o 
trig.o:			trig.s midir/trig.o 
r_atan_.o:		r_atan_.s midir/r_atan_.o 
r_exp_.o:		r_exp_.s midir/r_exp_.o 
r_hypot_.o:		r_hypot_.s midir/r_hypot_.o 
r_log_.o:		r_log_.s midir/r_log_.o 
r_sqrt_.o:		r_sqrt_.s midir/r_sqrt_.o 
r_trig_.o:		r_trig_.s midir/r_trig_.o 
