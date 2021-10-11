	.data
/*	.asciz	"@(#)modf.s 1.1 92/07/30 SMI"	*/
	.text
 
|       Copyright (c) 1987 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"
#include "PIC.h"

/*
 * double
 * modf( value, iptr)
 *      double value, *iptr;
 *
 * return a value v and stores through iptr a value i s.t.:
 *	v + i == value, and
 *4.2 BSD:
 *	1 > v >= 0
 *	Note that for -0.5 < value < 0, v may not be representable.
 *System V:
	v has sign of value and |v| < 1.
	v is always exact.
 */

ENTRY(modf)
	moveml	PARAM,d0/d1	| d0/d1 gets x.
|	JBSR(Vfloord,a0)	| d0/d1 gets floor(x). 4.2 BSD definition.
 	JBSR(Vaintd,a0)		| d0/d1 gets aint(x). System V definition.
	movl	PARAM3,a0	| a0 gets address of *iptr.
	moveml	d0/d1,a0@	| *iptr gets floor(x).
	lea	PARAM,a0	| a0 points to x.
#ifdef PIC
	movl	a2,sp@-		| transparent to a2
#endif PIC
	JBSR(Vsubd,a2)		| d0/d1 gets floor(x)-x.
#ifdef PIC
	movl	sp@+,a2		| transparent to a2
#endif PIC
	bchg	#31,d0		| d0/d1 gets x-floor(x) >= 0 unless x is inf or nan.
	RET
