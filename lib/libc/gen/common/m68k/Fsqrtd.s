	.data
|        .asciz  "@(#)Fsqrtd.s 1.1 92/07/30 SMI"
 	.even
	.text

|	Copyright (c) 1987 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"
#include <sys/errno.h>

	SAVEALL	 = 0xff00	| registers d0-d7
	RESTALL	 = 0x00ff	
	RESTALLBUT = 0x00fc	| registers d2-d7

	| Register usage:
	| d0,d1 contain the original argument and the final result.
	| d2    contains the traveling bit.
	| d3 	contains the loop count.
	| d4	contains the exponent.
	| d5	contains the sign, briefly.
	| d5,d6	contain the remainder.
	| d7 	contains zero.

RTENTRY(Fsqrtd)
	moveml	#SAVEALL,sp@-
	roll    #4,d0		| Move sign.
	roll	#8,d0		| Move exponent.
	movw	d0,d4		| d4 gets sign and exponent.
	andl	#0xfffff000,d0	| Clear 12 bits from d0.
	andw	#0xfff,d4	| Remove junk.
	bclr	#11,d4		| Clear sign bit.
	sne	d5		| d5 gets sign.
	roll	#4,d1		| Align d1.
	roll	#8,d1
	movw	d1,d6		| d6 gets low order of d1.
	andl	#0xfffff000,d1	| Clear 12 bits from d1.
	andw	#0xfff,d6	| d6 gets 12 low order bits of argument.
	orw	d6,d0		| Move 12 bits to d0.
	tstw	d4
	bnes	testmax		| Branch if not zero or subnormal.
	tstl	d0
	bnes	subnormal
	tstl	d1
	bnes	subnormal
	
retarg:				| Return argument for +-0, +inf, nan.
	moveml	sp@+,#RESTALL
	RET

testmax:
	cmpw	#0x7ff,d4
	bnes	normal		| Branch if normal.
	tstl	d0
	bnes	retarg		| Nan.
	tstl	d1
	bnes	retarg		| Nan.
	tstb	d5
	beqs	retarg		| Branch if +infinity.

errarg:				| Return error nan and set EDOM.
	moveml	sp@+,#RESTALL
#ifdef PIC
	movl	a0,sp@-
	PIC_SETUP(a0)
	movl	a0@(_errno:w),a0
	movl	#EDOM,a0@
	movl	sp@+,a0
#else
	movel	#EDOM,_errno	| errno = EDOM.
#endif
	movel	#0x7fffffff,d0  | Motorola quiet nan.
	movel	#0xffffffff,d1
	RET

subnormal:			| Subnormal number.
	tstl	d0
	bmis	main		| Branch if normalized now.
1$:	subqw	#1,d4		| Decrement exponent.
	lsll	#1,d1		| Normalize.
	roxll	#1,d0
	bpls	1$		| Branch if still not normalized.
	bras	main

normal:
	lsrl	#1,d0		| Make room for leading bit.
	roxrl	#1,d1
	bset	#31,d0		| Set I bit.

main:				| d1 has normalized significand;
	tstb	d5
	bnes	errarg		| Branch if negative.
	movl	d0,d5		| d5,d6 will get remainder.
	movl	d1,d6
				| d4 has biased exponent.
				| Since bias is odd, odd biased exponent =
				| even true exponent.
	btst	#0,d4		
	bnes	3$		| Branch if odd biased exponent.
	subqw	#1,d4		| Decrement exponent if even.
	bras	4$
3$:
	lsrl	#1,d5		| Extra alignment for odd exponent.
	roxrl	#1,d6
4$:
			| In the loop below, 
			| d0 contains the result z
			| d1 contains the remainder
			| d2 contains the traveling bit b
			| d3 contains the loop count
			| d7 contains zero.

			| For historical reasons, the interpretation of the
			| bits of d0 and d1 is:
			| bit 31 is called the S bit
			| bit 30 is called the 0 bit
			| bit 30-i is called the i bit.
			| In the loop below, counting the first iteration as
			| 1 and the last n, the traveling bit is bit i at
			| the beginning of each loop step and bit i+1 at the
			| end.
			| This loop performs a non-restoring square root.

	clrl	d0		| Result z gets 0 to start.
	clrl	d1
	clrl	d7
	movl	#0x20000000,d2	| Traveling bit starts as bit 1.
	movw	#28,d3		| 29 steps through loop.
	lsrl	#1,d5		| Position remainder so S bit is clear.
	roxrl	#1,d6
	subl	d2,d5		| Initial subtract of 0.25.
toploop:
	bpls	plloop		| Branch if remainder was positive.
miloop:
	lsll	#1,d6		| Double remainder for next step.
	roxll	#1,d5
	addl	d2,d5
	lsrl	#1,d2		| Travel bit.
	addl	d2,d5
	addl	d0,d5		| r := r + z + 3.
	bras	botloop
plloop:
	lsll	#1,d6		| Double remainder for next step.
	roxll	#1,d5
	addl	d2,d0
	addl	d2,d0		| Turn on bit i-1 in result.	
	lsrl	#1,d2		| Travel bit.
	subl	d2,d5
	subl	d0,d5		| r := r - z - 1.
botloop:
	dbf	d3,toploop	| Branch if less than 25 steps.

	bpls	pl2loop		| Branch if remainder was positive.
mi2loop:
	lsll	#1,d6		| Double remainder for next step.
	roxll	#1,d5
	addl	#0x80000000,d6
	addxl	d2,d5
	addl	d0,d5		| r := r + z + 3.
	bras	bot2loop
pl2loop:
	lsll	#1,d6		| Double remainder for next step.
	roxll	#1,d5
	addql	#2,d0   	| Turn on bit i-1 in result.	
	subl	#0x80000000,d6
	subxl	d7,d5
	subl	d0,d5		| r := r - z - 1.
bot2loop:
	rorl	#1,d2		| Rotate traveling bit.
	tstl	d5
top3loop:
	bpls	pl3loop		| Branch if remainder was positive.
mi3loop:
	lsll	#1,d6		| Double remainder for next step.
	roxll	#1,d5
	addl	d2,d6
	addxl	d7,d5
	lsrl	#1,d2		| Travel bit.
	addl	d2,d6
	addxl	d0,d5		| r := r + z + 3.
	bras	bot3loop
pl3loop:
	lsll	#1,d6		| Double remainder for next step.
	roxll	#1,d5
	addql	#1,d0 		| Turn on bit i-1 in result.	
	lsrl	#1,d2		| Travel bit.
	subl	d2,d6
	subxl	d0,d5		| r := r - z - 1.
bot3loop:
	movw	#22,d3		| Steps required is 54 = 29+1+1+24.
	tstl	d5
top4loop:
	bpls	pl4loop		| Branch if remainder was positive.
mi4loop:
	lsll	#1,d6		| Double remainder for next step.
	roxll	#1,d5
	addl	d2,d6
	addxl	d7,d5
	lsrl	#1,d2		| Travel bit.
	addl	d2,d6
	addxl	d7,d5
	addl	d1,d6
	addxl	d0,d5		| r := r + z + 3.
	bras	bot4loop
pl4loop:
	lsll	#1,d6		| Double remainder for next step.
	roxll	#1,d5
	addl	d2,d1
	addl	d2,d1		| Turn on bit i-1 in result.	
	lsrl	#1,d2		| Travel bit.
	subl	d2,d6
	subxl	d7,d5
	subl	d1,d6
	subxl	d0,d5		| r := r - z - 1.
bot4loop:
	dbf	d3,top4loop	| Branch if less than 25 steps.
	
	addl	#0x200,d1	| Add round bit.
	addxl	d7,d0		| On sqrt, no danger of ambiguous case or
				| of rounding carry overflow.
	lsll	#1,d1		| Clear S bit.
	roxll	#1,d0
	lsll	#1,d1		| Clear I bit.
	roxll	#1,d0
	asrw	#1,d4		| Cut exponent in half.
	addw	#512,d4		| Cut bias in half.
	andw	#0xf000,d1	| Make space for extra bits.
	movw	d0,d6
	andw	#0xfff,d6	| D6 gets 12 extra bits for d1.
	orw	d6,d1
	rorl	#4,d1		| Reposition bits.
	rorl	#8,d1
	andw	#0xf000,d0	| Make space for exponent.
	orw	d4,d0		| Insert exponent.
	rorl	#4,d0		| Reposition exponent.
	rorl	#8,d0
	
	addql	#8,sp		| Don't restore d0,d1.
	moveml sp@+,#RESTALLBUT
	RET
