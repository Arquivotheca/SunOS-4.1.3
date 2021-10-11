	.data
|	.asciz	"@(#)lmult.s 1.1 92/07/30 Copyr 1983 Sun Micro"
	.even
	.text

|	Copyright (c) 1983 by Sun Microsystems, Inc.

|J. Test	10x81
|R. Tuck	09-82
|C. Aoki	02-85
|signed long multiply: c = a * b
| takes arguments in d0, d1: result in d0, clobbers d1
| uses and saves registers d2 and d2
| does NOT do link/ulink (sorry about that, adb)
| optimized for 16-bit operands (seems to work well with unix for some reason)
#include "DEFS.h"

	.text
	| register usage
	a     = d0
	b     = d1
	work1 = d2
	work2 = d3

RTENTRY(ulmult) | unsigned lmult
	movl	work1,sp@-	|save d2, d3
|
| assuming that short operands occur "most of the time" (wnj),
| try to do them as quickly as possible.
|
	movl	a,work1		| if ( (ushort)a == a && (ushort)b == b )
	orl	b,work1
	clrw	work1
	tstl	work1
	bnes	ulong_case
	mulu	b,a		| 	return((ushort)a * (ushort)b);
	movl	sp@+,work1
	RET

ulong_case:
	movl	work2,sp@-
	tstl	a
	beqs	zero_result
	tstl	b
	beqs	zero_result
	bras	pos_result

RTENTRY(lmult)	
|
| assuming that short operands occur "most of the time" (wnj),
| try to do them as quickly as possible.
|
	movl	work1,sp@-
	movl	a,work1		| if ( (ushort)a == a && (ushort)b == b )
	orl	b,work1
	clrw	work1
	tstl	work1
	bnes	long_case
	mulu	b,a		|	return((ushort)a * (ushort)b);
	movl	sp@+,work1
	RET

long_case:
	movl	work2,sp@-
	tstl	a
	blts	a_neg
	beqs	zero_result
	tstl	b
	blts	b_neg
	beqs	zero_result

pos_result:			|here, we know the result will be positive
	movw	a,work1		|work1 = a(lo)
	mulu	b,work1		|work1 = b(lo)*a(lo) is "low product"
	movw	a,work2		|work2 = a(lo)
	clrw	a		|a     = a(hi)
	swap	a		|if a(hi) == 0, cross product is zero
	beqs	2$
	mulu	b,a		|a     = a(hi)*b(lo)
2$:
	swap	b		|b     = b(hi)
	tstw	b		|if b(hi) == 0, cross product is zero
	beqs	3$		| yes, skip a step
	mulu	b,work2		|work2 = a(lo)*b(hi)
	addl	work2,a		|a     = (a(hi)*b(lo)) + (a(lo)*b(hi))
3$:
	swap	a		|( a is "cross product")
	clrw	a		|a   <<= 16
	addl	work1,a		|a     = low-product + cross-product
	movl	sp@+,work2
	movl	sp@+,work1
	RET

zero_result:
	moveq	#0,a
	movl	sp@+,work2
	movl	sp@+,work1
	RET

a_neg:				| a <0: try b
	negl	a
	tstl	b
	bgts	neg_result	| a < 0 & b > 0 => ab<0
	beqs	zero_result
	negl	b
	bras	pos_result	| a < 0 & b < 0 => ab>0

b_neg:				| a > 0 & b < 0 => ab<0
	negl	b
neg_result:
				| here, we know the result will be negative
	movw	a,work1		|work1 = a(lo)
	mulu	b,work1		|work1 = b(lo)*a(lo) is "low product"
	movw	a,work2		|work2 = a(lo)
	swap	a		|a     = a(hi)
	mulu	b,a		|a     = a(hi)*b(lo)
	swap	b		|b     = b(hi)
	mulu	b,work2		|work2 = a(lo)*b(hi)
	addl	work2,a		|a     = (a(hi)*b(lo)) + (a(lo)*b(hi))
	swap	a		|( a is "cross product")
	clrw	a		|a   <<= 16
	addl	work1,a		|a     = low-product + cross-product
	negl	a		| result is negative
	movl	sp@+,work2
	movl	sp@+,work1
	RET
