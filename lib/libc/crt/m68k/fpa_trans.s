	.data
|	.asciz	"@(#)fpa_trans.s 1.1 92/07/30"
	.even
	.text

|	Copyright (c) 1989 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"

/*
 	double fpa_transcendental ( n, x )
	long n ; double x ;
	
	computes fn(x) where
	0 = sin
	1 = cos
	2 = tan
	3 = atan
	4 = exp-1
	5 = ln(1+x)
	6 = exp
	7 = ln
	8 = sqrt
	9 = sqrt(x*2+y*2) where double x is a union containing &x and &y
*/

RTENTRY(_fpa_transcendental)
	movel	PARAM,d0
	lsll	#2,d0
#ifdef PIC
	PIC_SETUP(a0)
	movl	a0@(table:w),a0
#else
	lea	table,a0
#endif PIC
	addl	d0,a0
	movel	a0@,a0
	jmp	a0@		
#ifdef PIC
	.data			| Put dispatch table in data if PIC
#endif PIC
table:
	.long	msind,mcosd,mtand,matand,mexp1d,mlog1d,mexpd,mlogd,msqrtd,mhypotd
	.text
mlogd:	
	flognd	PARAM2,fp0
	bras	1f
msqrtd:	
	fsqrtd	PARAM2,fp0
	bras	1f
msind:	
	fsind	PARAM2,fp0
	bras	1f
mcosd:	
	fcosd	PARAM2,fp0
	bras	1f
mtand:	
	ftand	PARAM2,fp0
	bras	1f
matand:	
	fatand	PARAM2,fp0
	bras	1f
mexp1d:	
	fetoxm1d	PARAM2,fp0
	bras	1f
mexpd:	
	fetoxd	PARAM2,fp0
	bras	1f
mlog1d:	
	flognp1d	PARAM2,fp0
	bras	1f
mhypotd:	
	movel	PARAM2,a0
	movel	PARAM3,a1
	fmoved	a0@,fp0
	fmulx	fp0,fp0
	fmoved	a1@,fp1
	fmulx	fp1,fp1
	faddx	fp1,fp0
	fsqrtx	fp0,fp0
	bras	1f
1:
	fmoved	fp0,sp@-
	moveml	sp@+,d0/d1
	RET

/*
	double	fpa_tolong ( x )
	double x ;

	computes (double) (long) x
	according to Weitek mode
*/

RTENTRY(_fpa_tolong)
	fpmove	fpamode,d0
	andb	#2,d0
	beqs	1f		| Branch if round to current mode.
	fintrzd	PARAM,fp0
	bras	2f
1:
	fintd	PARAM,fp0
2:
	fmovel	fp0,d0
	fmovel	d0,fp0
	fmoved	fp0,PARAM
	moveml	PARAM,d0/d1
	RET
