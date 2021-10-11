        .data 
|        .asciz  "@(#)Fsqrts.s 1.1 92/07/30 SMI"
        .even
        .text

|	Copyright (c) 1987 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"
#include "PIC.h"

RTENTRY(Fsqrts)
	moveml	d2-d5/a2,sp@-
	movl	d0,d1
	movl	d1,d5				| save 
	JBSR(f_tst,a2)					| test the argument
	addw	d0,d0			 	| switch on the type field 
	movw	pc@(6,d0:w),d1
	jmp	pc@(2,d1:w)
Ltable:
	.word	Lerr-Ltable			| unknown type
	.word	Lzero-Ltable			| zero
	.word	Lgu-Ltable			| gradual undeflow
	.word	Lplain-Ltable			| ordinary number
	.word	Linf-Ltable			| infinity
	.word	Lnan-Ltable			| Nan

Lgu:
	movl	d5,d1		| d1 gets argument x.
	roxll	#1,d1		| X gets sign bit.
	jcs 	Lerrarg		| Error if negative.
	clrw	d4		| Exponent for max subnormal.
	bclr	#0,d1		| Remove sign bit - X junk.
	roll	#8,d1		| Move exponent to ls byte.
	bmis	main		| Branch if normalized now.
1$:	subqw	#1,d4		| Decrement exponent.
	lsll	#1,d1		| Normalize.
	bpls	1$		| Branch if still not normalized.
	bras	main

Lplain:
	movl	d5,d1		| d1 gets argument x.
	roll	#1,d1		| X gets sign bit; bit 0 gets junk.
	bcss	Lerrarg		| Branch if negative argument.
	bclr	#0,d1		| Clear bit 0.
	roll	#8,d1		| Move exponent to least sig byte.
	clrw	d4
	movb	d1,d4		| d4 gets (biased) exponent.
	movb	#1,d1		| Clear exponent and set i bit.
	rorl	#1,d1		| Position leading bit.

main:				| d1 has normalized significand;
				| d4 has biased exponent.
				| Since bias is odd, odd biased exponent =
				| even true exponent.
	btst	#0,d4		
	bnes	3$		| Branch if odd biased exponent.
	subqw	#1,d4		| Decrement exponent if even.
	bras	4$
3$:
	lsrl	#1,d1		| Extra alignment for odd exponent.
4$:
			| In the loop below, 
			| d0 contains the result z
			| d1 contains the remainder r
			| d2 contains the traveling bit b
			| d3 contains the loop count

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
	movl	#0x20000000,d2	| Traveling bit starts as bit 1.
	movw	#24,d3		| 25 steps through loop.
	lsrl	#1,d1		| Position remainder so S bit is clear.
	subl	d2,d1		| Initial subtract of 0.25.
toploop:
	bpls	plloop		| Branch if remainder was positive.
miloop:
	asll	#1,d1		| Double remainder for next step.
	addl	d2,d1
	lsrl	#1,d2		| Travel bit.
	addl	d2,d1
	addl	d0,d1		| r := r + z + 3.
	bras	botloop
plloop:
	asll	#1,d1		| Double remainder for next step.
	addl	d2,d0
	addl	d2,d0		| Turn on bit i-1 in result.	
	lsrl	#1,d2		| Travel bit.
	subl	d2,d1
	subl	d0,d1		| r := r - z - 1.
botloop:
	dbf	d3,toploop	| Branch if less than 25 steps.

	addl	#0x40,d0	| Add round bit.
				| On sqrt, no danger of ambiguous case or
				| of rounding carry overflow.
	lsll	#2,d0		| Clear S, and I bits.
	asrw	#1,d4		| Cut exponent in half.
	addw	#64,d4		| Cut bias in half.
	movb	d4,d0		| Insert exponent..
	rorl	#8,d0		| Reposition exponent.
	lsrl	#1,d0		| Reposition positive sign.
	
Ldone:
	moveml sp@+,d2-d5/a2
	RET
Lzero:					| sqrt(+-0) = +-0.
Lnan:					| sqrt(nan) = nan.
	movl	d5,d0
	bras	Ldone
Linf:
	movl	d5,d0
	bpls	Ldone			| sqrt(+inf) = +inf.
Lerr:
Lerrarg:
	JBSR(f_snan,a2)
	bras	Ldone
