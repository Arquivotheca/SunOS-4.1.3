        .data
|       .asciz  "@(#)Fclass2s.s 1.1 92/07/30 SMI"
        .even
        .text

|       Copyright (c) 1987 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"
#include <sys/errno.h>

|	Fclass2s is a front end routine for floating point software.
|	It is called as follows:
|	
|	Fxxxs:	jsr	Fclass2s
|		.long	xnorm.ynorm,xnorm.yzero,xnorm.ysub,xnorm.yinf
|		.long	xzero.ynorm,xzero.yzero,xzero.ysub,xzero.yinf
|		.long	xsub.ynorm,xsub.yzero,xsub.ysub,xsub.yinf
|		.long	xinf.ynorm,xinf.yzero,xinf.ysub,xinf.yinf
|
|	Each table entry is a small positive integer or an address.  The integers correspond
|	to predefined routines for common operations.  The addresses are addresses of
|	caller-supplied routines.  x and y are classified, and if the indicated table entry
| 	is one of the predefined routines, that routine is executed followed by return to
|	the caller of Fxxxs.  Otherwise the entry point listed in the routine is jumped to with
|	a stack frame containing:
|
|	top:	original d0-d7/a1 save
|		Fxxxd+6, the jsr return
|
|	d0/d3 contain unpacked x, and d4/d7 contain unpacked y.
|	The sign of x is in bit 31 of d3, the unbiased exponent in bits 0 to 15,
|	the significand in d0, normalized.
|	Note that predefined routines should be used if either argument is zero or infinity.
|	NaNs are checked for automatically and return quiet NaNs.

offnorm	= 0
offzero = 1
offsub  = 2
offinf  = 3

ENTER(Fclass2s)
	moveml	d0-d7/a1,sp@-		| Save all d registers.
	movel	sp@(36),a1		| a1 gets address of jump table.
	roll	#1,d0
	roll	#8,d0			| Align exponent.
	movel	d0,d3
	andl	#0xff,d3		| d3 gets biased exponent.
	beqs	xmin			| Branch if x is 0 or subnormal.
	cmpw	#0xff,d3
	beqs	xmax			| Branch if x is inf or nan.
	addl	#16*offnorm,a1		| a1 gets offset for x.
	bset	#0,d0			| Turn on i bit.
	bras	y
xmax:	
	eorl	d0,d3			| Turns on s and f bits in d0.
	bclr	#8,d3			| Turn off sign bit.
	tstl	d3
	bnes	xnan
	addl	#16*offinf,a1		| a1 gets offset for x.
	bras	y
xnan:
	bset	#22,sp@(1)		| Make x quiet.
	jra	returnx
xmin:	
	movel	d0,d2
	bclr	#8,d2			| Clear sign bit.
	tstl	d2
	bnes	xsub
	addl	#16*offzero,a1		| a1 gets offset for x.
	bras	y
xsub:
	addl	#16*offsub,a1		| a1 gets offset for x.
	bclr	#0,d0			| Turn off i bit.
	movew	#1,d3			| Subnormal exponent.
	bras	y
y:
	movel	d1,d4			| d4 get y.
	roll	#1,d4
	roll	#8,d4			| Align exponent.
	movel	d4,d7
	andl	#0xff,d7		| d7 gets biased exponent.
	beqs	ymin			| Branch if y is 0 or subnormal.
	cmpw	#0xff,d7
	beqs	ymax			| Branch if y is inf or nan.
	addl	#4*offnorm,a1		| a1 gets offset for y.
	bset	#0,d4			| Turn on i bit.
	bras	fetchtable
ymax:	
	eorl	d4,d7			| Turns on s and f bits in d4.
	bclr	#8,d7			| Turn off sign bit.
	tstl	d7
	bnes	ynan
	addl	#4*offinf,a1		| a1 gets offset for y.
	bras	fetchtable
ynan:
	bset	#22,d1			| Make y quiet.
	jra	returny
ymin:	
	movel	d4,d6
	bclr	#8,d6			| Clear sign bit.
	tstl	d6
	bnes	ysub
	addl	#4*offzero,a1		| a1 gets offset for y.
	bras	fetchtable
ysub:
	addl	#4*offsub,a1		| a1 gets offset for y.
	bclr	#0,d4			| Turn off i bit.
	movew	#1,d7			| Subnormal exponent.
	bras	fetchtable
fetchtable:
        btst    #0,a1@(3)               | Test lsb.
        movel   a1@,a1                  | a1 gets table entry.
        beqs    unpack                  | If lsb == 0, it's a real address.
        cmpl    #RETURNX,a1
	beqs	returnx
	cmpl	#RETURNY,a1
	beqs	returny
	cmpl	#RETURNINVALID,a1
	beqs	returninvalid	
	bras	returninvalid

returninvalid:
#ifdef PIC
	PIC_SETUP(a1)
	movel	a1@(_errno:w),a1
	movel	#EDOM,a1@
#else	
	movel   #EDOM,_errno    	| errno = EDOM.
#endif
	movel	invalidnan,d0
	bras	bypassx
returny:
	movel	d1,d0			| Result y.
bypassx:
	addql	#8,sp			| Bypass x.
	moveml	sp@+,d2-d7/a1		| Restore registers.
	bras	bypass
returnx:
	moveml	sp@+,d0-d7/a1		| Restore x and d registers.
bypass:
	addql	#4,sp			| Bypass return address of Fclass2d.
	RET

invalidnan:
	.single	0rNaN

|	Unpack x.

unpack:
	subw	#0x7f,d3		| Unbias x exponent.
	btst	#8,d0
	beqs	1f			| Branch if x positive.
	bset	#31,d3			| Negative sign in d3.
1:
	rorl	#1,d0			| Align significand.
	andl	#0xffffff00,d0		| Remove sign and exponent from d0.
	bmis	1f
2:
	subqw	#1,d3
	lsll	#1,d0
	bpls	2b
1:	
|	Unpack y.

	subw	#0x7f,d7		| Unbias y exponent.
	btst	#8,d4
	beqs	1f			| Branch if y positive.
	bset	#31,d7			| Negative sign in d7.
1:
	rorl	#1,d4			| Align significand.
	andl	#0xffffff00,d4		| Remove sign and exponent from d4.
	bmis	1f
2:
	subqw	#1,d7
	lsll	#1,d4
	bpls	2b
1:	
	jmp	a1@			| Not predefined, so go to it.

