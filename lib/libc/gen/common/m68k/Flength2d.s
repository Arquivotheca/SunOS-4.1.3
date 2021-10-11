	.data
|        .asciz  "@(#)Flength2d.s 1.1 92/07/30 SMI"
 	.even
	.text

|	Copyright (c) 1987 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"
#include "PIC.h"
RTENTRY(Flength2d)
	link	a6,#-12
scalefactor = -4			| temp on stack
ysquared = -12				| temp on stack
	moveml	d0/d1/a2,sp@-		| Sarve d0/d1/d2/d3/d4.
	JBSR(Fexpod,a2)			| d0 gets exponent of x.
	movel	d0,d1			| d1 gets exponent of x.
	movel	a0@,d0			| d0 gets upper part of y.
	JBSR(Fexpod,a2)			| d0 gets exponent of y.
	cmpl	d0,d1
	bges	1f
	movel	d0,a6@(scalefactor)	| gets scale factor.
	bras	2f
1:
	movel	d1,a6@(scalefactor)	| gets scale factor.
2:
	negl	a6@(scalefactor)	| Reverse sign.
	moveml	a0@,d0/d1		| d0/d1 get y.
	lea	a6@(scalefactor),a0	| a0 gets address of scale factor.
	JBSR(Fscaleid,a2)		| d0/d1 get y/s.
	JBSR(Fsqrd,a2)			| d0/d1 get (y/s)**2
	moveml	d0/d1,a6@(ysquared)
	moveml	sp@+,d0/d1		| Restore x.
	lea	a6@(scalefactor),a0	| a0 gets address of scale factor.
	JBSR(Fscaleid,a2)		| d0/d1 get x/s.
	JBSR(Fsqrd,a2)			| d0/d1 get (x/s)**2
	lea	a6@(ysquared),a0
	JBSR(Faddd,a2)			| d0/d1 get sum of squares.
	JBSR(Fsqrtd,a2)
	negl	a6@(scalefactor)	| Reverse scale factor again.
	lea	a6@(scalefactor),a0	| a0 gets address of scale factor.
	JBSR(Fscaleid,a2)
	movl	sp@+,a2			| Be transparent
	unlk	a6
	RET
