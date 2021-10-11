	.data
|	.asciz	"@(#)Slength2d.s 1.1 92/07/30 SMI"
	.even
	.text


|	Copyright (c) 1987 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"
#include "Sdefs.h"

RTENTRY(Slength2d)
	link	a6,#-12
scalefactor = -4			| temp on stack
ysquared = -12				| temp on stack
	moveml	d0/d1/a2,sp@-		| Save 
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
#ifdef PIC
	PIC_SETUP(a2)
	movl	a2@(__skybase:w),a2
	movl	a2@, SKYBASE
#else
        movl    __skybase,SKYBASE
#endif 
        FMULD(d0,d1,d0,d1,a6@(ysquared),a6@(ysquared+4)) | ysquared get (y/s)**2
	moveml	sp@+,d0/d1		| Restore x.
	lea	a6@(scalefactor),a0	| a0 gets address of scale factor.
	JBSR(Fscaleid,a2)		| d0/d1 get x/s.
	movw	#0x1018,SKYBASE@(-OPERAND)
	movl	d0,SKYBASE@
	movl	d1,SKYBASE@
	movl	d0,SKYBASE@
	movl	d1,SKYBASE@
	movel	a6@(ysquared),SKYBASE@
	movel	a6@(ysquared+4),SKYBASE@
	IORDY				| r0 := y * y
	movl	SKYBASE@,d0
	movl	SKYBASE@,d1		| d0/d1 := x * x + y * y
	JBSR(Fsqrtd,a2)
	negl	a6@(scalefactor)	| Reverse scale factor again.
	lea	a6@(scalefactor),a0	| a0 gets address of scale factor.
	JBSR(Fscaleid,a2)
	movl	sp@+,a2
	unlk	a6
	RET
