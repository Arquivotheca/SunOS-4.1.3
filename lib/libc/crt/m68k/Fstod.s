	.data
|	.asciz	"@(#)Fstod.s 1.1 92/07/30 Copyr 1985 Sun Micro"
	.even
	.text

|	Copyright (c) 1985 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"

RTENTRY(Fstod)
	movel	d0,a0			| a0 saves x.
rotate3:
	rorl	#3,d0			| d0 gets 3|1|8|20
	movel	d0,d1			| d1 gets 3|1|8|20
	andl	#0xe0000000,d1		| d1 gets 3 low order significand bits.
	andl	#0x0fffffff,d0		| d0 gets exponent and 20 high order significand.
	addl	#(0x3ff00000-0x7f00000),d0 | Adjust exponent from single to double.
	cmpl	#0x38100000,d0
	blts	minexp			| Branch if zero or subnormal.
	cmpl	#0x47f00000,d0
	bges	maxexp			| Branch if inf or nan.
sign:
	cmpl	#0,a0
	bges	1f			| Branch if sign of x is positive.
	orl	#0x80000000,d0		| Set negative.
1:
	RET

maxexp:
	orl	#0x7ff00000,d0		| d0 gets max double exponent.
	cmpl	#0x7ff00000,d0
	bgts	nan			| Branch if nan.
	tstl	d1
	beqs	sign			| Branch if inf.
nan:
	orl	#0x00400000,d0		| Force quiet nan.
	bras	sign
minexp:
	movel	a0,d0			| d0 gets x.
	andl	#0x007fffff,d0		| d0 gets x significand.
	beqs	sign			| Branch if x is true zero.
	movel	#0x4000,d1		| d1 gets single format exponent +1.
	bras	2f
norm:
	subw	#0x0080,d1		| Decrement exponent by 1.
2:
	lsll	#1,d0
	btst	#23,d0
	beqs	norm			| Branch if not yet normalized.
	andl	#0x007fffff,d0		| Clear exponent.
	swap	d1
	orl	d1,d0			| Insert single exponent.
	rorl	#3,d0			| d0 gets 3|1|8|20
	movel	d0,d1			| d1 gets 3|1|8|20
	andl	#0xe0000000,d1		| d1 gets 3 low order significand bits.
	andl	#0x0fffffff,d0		| d0 gets exponent and 20 high order significand.
	addl	#(0x3ff00000-0xff00000),d0 | Adjust exponent from single 
					| to double and unbias.
	bras	sign 
