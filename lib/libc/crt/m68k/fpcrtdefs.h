/*      @(#)fpcrtdefs.h 1.1 92/07/30 SMI      */

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include "DEFS.h"
#include "PIC.h"

#define ENTER(f) .globl f ; f:
#define OBSOLETE(f) ENTER(f) 		
/* Obsolete entry points, remove in next release. */
#define RTOBSOLETE(f) RTENTRY(f) 	

#define FP_UNSPECIFIED	0 	
#define FP_SOFTWARE	1 	
#define FP_SKYFFP	2 	
#define FP_MC68881	3 	
#define FP_SUNFPA	4 	
#define VOFFSET 2
#define FOFFSET 6
#define SOFFSET 10
#define MOFFSET 14
#define WOFFSET 18


#ifdef PIC
#define VECTORED(f) 					\
.text ;							\
ENTER(V/**/f);						\
	PIC_SETUP(a1);					\
	movel	a1@(_fp_switch:w), a1;			\
	movel	a1@, a1;				\
	cmpw	#FP_SUNFPA,a1;				\
	bnes	1f;					\
	JBSR(W/**/f,a1);				\
	rts ;						\
1:;							\
	cmpw	#FP_MC68881,a1;				\
	bnes	1f;					\
	JBSR(M/**/f,a1);				\
	rts;						\
1:;							\
	cmpw	#FP_SKYFFP,a1;				\
	bnes	1f;					\
	JBSR(S/**/f,a1);				\
	rts;						\
1:;							\
	cmpw	#FP_SOFTWARE,a1;			\
	bnes	1f;					\
	JBSR(F/**/f,a1);				\
	rts;						\
1:;							\
	moveml	a0/a1/d0/d1,sp@-;			\
	JBSR(Vinit,a1);					\
	moveml	sp@+,a0/a1/d0/d1;			\
	bras	V/**/f					

#define	VECTOR(f, g)					\
.text ;							\
OBSOLETE(g) ;						\
ENTER(V/**/f);						\
	PIC_SETUP(a1);					\
	movel	a1@(_fp_switch:w), a1;			\
	movel	a1@, a1;				\
	cmpw	#FP_SUNFPA,a1;				\
	bnes	1f;					\
	JBSR(W/**/f,a1);				\
	rts ;						\
1:;							\
	cmpw	#FP_MC68881,a1;				\
	bnes	1f;					\
	JBSR(M/**/f,a1);				\
	rts;						\
1:;							\
	cmpw	#FP_SKYFFP,a1;				\
	bnes	1f;					\
	JBSR(S/**/f,a1);				\
	rts;						\
1:;							\
	cmpw	#FP_SOFTWARE,a1;			\
	bnes	1f;					\
	JBSR(F/**/f,a1);				\
	rts;						\
1:;							\
	moveml	a0/a1/d0/d1,sp@-;			\
	JBSR(Vinit,a1);					\
	moveml	sp@+,a0/a1/d0/d1;			\
	bras	V/**/f
#else 
#define VECTORED(f) 					\
.text ;							\
ENTER(V/**/f);						\
	movel	_fp_switch, a1;				\
	cmpw	#FP_SUNFPA,a1;				\
	bnes	1f;					\
	JBSR(W/**/f,a1);				\
	rts ;						\
1:;							\
	cmpw	#FP_MC68881,a1;				\
	bnes	1f;					\
	JBSR(M/**/f,a1);				\
	rts;						\
1:;							\
	cmpw	#FP_SKYFFP,a1;				\
	bnes	1f;					\
	JBSR(S/**/f,a1);				\
	rts;						\
1:;							\
	cmpw	#FP_SOFTWARE,a1;			\
	bnes	1f;					\
	JBSR(F/**/f,a1);				\
	rts;						\
1:;							\
	moveml	a0/a1/d0/d1,sp@-;			\
	JBSR(Vinit,a1);					\
	moveml	sp@+,a0/a1/d0/d1;			\
	bras	V/**/f					

#define VECTOR(f, g) 					\
.text ;							\
OBSOLETE(g);						\
ENTER(V/**/f);						\
	movel	_fp_switch, a1;				\
	cmpw	#FP_SUNFPA,a1;				\
	bnes	1f;					\
	JBSR(W/**/f,a1);				\
	rts ;						\
1:;							\
	cmpw	#FP_MC68881,a1;				\
	bnes	1f;					\
	JBSR(M/**/f,a1);				\
	rts;						\
1:;							\
	cmpw	#FP_SKYFFP,a1;				\
	bnes	1f;					\
	JBSR(S/**/f,a1);				\
	rts;						\
1:;							\
	cmpw	#FP_SOFTWARE,a1;			\
	bnes	1f;					\
	JBSR(F/**/f,a1);				\
	rts;						\
1:;							\
	moveml	a0/a1/d0/d1,sp@-;			\
	JBSR(Vinit,a1);					\
	moveml	sp@+,a0/a1/d0/d1;			\
	bras	V/**/f
#endif 

/*	Entries to convert between C parameter convention and Vxxx convention. */

#define VCDTOD(f) \
	ENTER(f) ; \
	moveml	d0/d1,sp@- ; \
        jsr    	_C/**/f ; \
        addql   #8,sp ; \
        rts
 
#define VCDDTOD(f) \
	ENTER(f) ; \
	movel	a0@(4),sp@- ; \
	movel	a0@,sp@- ; \
	moveml	d0/d1,sp@- ; \
        jsr    	_C/**/f ; \
        addl    #16,sp ; \
        rts
 

/*	Entries to convert between Fortran parameter convention and Vxxx convention. */

#define VFSTOS(x) \
	ENTER(x) ; \
	movel	d0,sp@- ; \
        pea	sp@ ; \
	jsr    	_F/**/x/**/_ ; \
        addql    #8,sp ; \
        rts
 
#define VFSSTOS(x) \
	ENTER(x) ; \
	moveml	d0/d1,sp@- ; \
	pea	sp@(4) ; \
	pea	sp@(4) ; \
        jsr    	_F/**/x/**/_ ; \
        addl    #16,sp ; \
        rts
 
#define VFDTOD(x) \
	ENTER(x) ; \
	moveml	d0/d1,sp@- ; \
        pea	sp@ ; \
	jsr    	_F/**/x/**/_ ; \
        addl    #12,sp ; \
        rts
 
#define VFDDTOD(x) \
	ENTER(x) ; \
	moveml	d0/d1,sp@- ; \
	pea	a0@ ; \
	pea	sp@(4) ; \
        jsr    	_F/**/x/**/_ ; \
        addl    #16,sp ; \
        rts
 
/*	Floating point condition codes, as in movew	#FLT,cc  	*/

#define FEQ	4	/* Z bit */
#define	FLT	25	/* XNC bits */
#define FGT	0	/* no bits */
#define	FUN	2	/* V bit */

/*	Floating point modes and status. */

/*
 *      Rounding Direction
 */

#define ROUNDMASK ~0x30

#define RNEAREST   0
#define RZERO   0x10
#define RMINUS  0x20
#define RPLUS   0x30

/*
 *      Rounding Precision
 */

#define ROUNDTODOUBLE 0x80	/* 68881 rounding precision corresponding to F/S/W */

/* 
 *	Jump Table Equates - all odd and negative
 */

#define RETURNX 	-1	/* Return first argument */
#define RETURNY		-3	/* Return second argument */
#define	RETURNINVALID	-5	/* Return a quiet nan and set errno */
#define	RETURN1		-7	/* Return +1.0. */
#define	RETURNINFORZERO	-9	/* Return +inf for +inf, +0 for -inf. */

/*
 *	Context save/restore info.
 */

#define FPSAVESIZE	256	/* Block for stacking state of current floating point device. */

/*	68020 cache clear call 
 */

#define CLEARCACHE	trap	#2
