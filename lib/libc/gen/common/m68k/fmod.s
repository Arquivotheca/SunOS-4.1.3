|	.data
|       .asciz  "@(#)fmod.s 1.1 92/07/30 SMI"
	.even
	.text

|	Copyright (c) 1987 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"

/*
 * double
 * fmod ( x, y )
 *	double x, y ;
 *
 * return a value v such that (x-v)/y is an integral value
 * and sign(v) = sign(x), which is equivalent to Fortran MOD.
 */

ENTRY(fmod)
	moveml	PARAM,d0/d1	| d0/d1 gets x.
	lea	PARAM3,a0	| a0 gets address of y.
#ifdef PIC
	movl	a2,sp@-		| transparent
#endif PIC
	JBSR(Vmodd,a1)
#ifdef PIC
	movl	sp@+,a2
#endif PIC
	RET
