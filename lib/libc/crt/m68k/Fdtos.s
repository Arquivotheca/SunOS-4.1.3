	.data
|	.asciz	"@(#)Fdtos.s 1.1 92/07/30 Copyr 1985 Sun Micro"
	.even
	.text

|	Copyright (c) 1985 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"

RTENTRY(Fdtos)
	movel	d0,a0			| a0 saves x.
	andl	#0x7fffffff,d0		| Clear sign bit.
	cmpl	#0x38100000,d0
	blts	small			| Branch if small exponent.
	cmpl	#0x47f00000,d0
	bges	big			| Branch if big exponent.
	subl	#0x38000000,d0
	lsll	#1,d1			| Shift left three places.
	roxll	#1,d0
	lsll	#1,d1
	roxll	#1,d0
	lsll	#1,d1
	roxll	#1,d0
	addl	#0x80000000,d1		| Add round bit.
	bccs	1f
	addql	#1,d0			| Propagate carry.
1:
	tstl	d1
	bnes	sign			| Branch if not ambiguous.
	andl	#0xfffffffe,d0		| Force round to even.
sign:
	cmpl	#0,a0
	bges	1f			| Branch if sign of x is positive.
	orl	#0x80000000,d0		| Set negative.
1:
	RET

big:
	cmpl	#0x7ff00000,d0
	bgts	nan			| Branch if nan.
	blts	inf			| Branch if overflow.
	tstl	d1
	bnes	nan			| Branch if nan.
inf:
	movel	#0x7f800000,d0		| Make inf.
	clrl	d1
	bras	sign
nan:
	lsll	#1,d1			| Shift left three places.
	roxll	#1,d0
	lsll	#1,d1
	roxll	#1,d0
	lsll	#1,d1
	roxll	#1,d0
	orl	#0x7fc00000,d0		| Force quiet nan.
	bras	sign
small:
	cmpl	#0x36900000,d0
	bges	subnorm			| Branch if possible subnorm.
	clrl	d0
	bras	sign			| Branch if zero.
subnorm:
	lsll	#1,d1			| Shift left three places.
	roxll	#1,d0
	lsll	#1,d1
	roxll	#1,d0
	lsll	#1,d1
	roxll	#1,d0
	tstl	d1
	beqs	1f			| Branch if no lower bits.
	orl	#0x80000000,d1		| Set sticky bit.
	bras	2f
1:
	clrl	d1			| Clear sticky bit.
2:
	swap    d0
	movew	d0,d1
	swap 	d0
	lsrw	#7,d1
	andl	#0x007fffff,d0		| Clear exponent.
	orl	#0x00800000,d0		| Set I bit.
	subw	#0x0180,d1		| Convert to shift count.
	bges	2f

norm:
	lsrl	#1,d0
	bcc	1f
	orl	#0x80000000,d1		| Force sticky bit.
1:
	addqw	#1,d1
	blts	norm

2:
	addql	#1,d0			| Round bit.
	btst	#0,d0
	bnes	2f			| Branch not ambiguous.
	btst	#31,d1
	bnes	2f			| Branch not ambiguous.
	andl	#0x01fffffc,d0		| Force round to even.
2:
	lsrl	#1,d0			| Remove round bit.
	jra	sign

