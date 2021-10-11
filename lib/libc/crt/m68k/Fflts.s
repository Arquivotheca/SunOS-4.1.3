	.data
|	.asciz	"@(#)Fflts.s 1.1 92/07/30 Copyr 1986 Sun Micro"
	.even
	.text

|	Copyright (c) 1986 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"

/*
 *	single-precision coersions
 */

/*
 * single integer to floating conversion:
 * input:
 *	d0	integer value
 * output:
 *	d0	floating value
 */

RTENTRY(Fflts)
	movel	d0,a0		| a0 saves x.
	movel	d2,a1		| a1 saves d2.
	movel	#0x9e,d1  	| d1 accumulates exponent.
	tstl	d0
	bpls	3f		| Branch if x is non-negative.
	negl	d0		| d0 gets abs(x).
3:
	bmis	2f		| Branch if d0 is normalized.
	beqs	Ffltsdone	| Branch if zero.
1:
	subqw	#1,d1		| Adjust exponent.
	lsll	#1,d0		| Normalize.
	bpls	1b		| Branch if already normalized.
2:
	bclr	#31,d0		| Clear I bit.
	movel	d0,d2		| d2 gets significand.
	lsrl	#8,d0		| Align high order bits.
	swap	d1
	lsll	#7,d1		| Align exponent.
	orl	d1,d0		| Stick exponent.
	movel	#24,d1
	lsll	d1,d2		| Align rounding bits.
	addl	#0x80000000,d2
	bccs	1f		| Branch if no carry out.
	addql	#1,d0		| Propagate carry.
	tstl	d2
	bnes	1f		| Branch if not ambiguous.
	bclr	#0,d0		| Force even.
1:
	cmpl	#0,a0
	bges	Ffltsdone	| Branch if x was non-negative.
	bset	#31,d0		| Set sign if negative.
Ffltsdone:
	movel	a1,d2		| Restore d2.
	RET

/*
 * single floating to unsigned integer conversion:
 * input:
 *	d0	floating value
 * output:
 *	d0	unsigned integer value
 */

RTENTRY(Funs)
        movel   d0,d1           | d1 gets x.
        cmpl    #0x3f800000,d1
        bges    2f             | Branch if x >= 1.
        clrl    d0  		| Result is zero.
        bras    Funsdone
2:
        cmpl    #0x4f800000,d1
        blts    1f        	| Branch if 1 <= |x| < 2**32.
        movel	#0x-1,d0	| Max unsigned integer.
        bras    Funsdone
1:
        movel   d0,a0           | Save x.
	andl	#0x7fffff,d0	| Clear sign and exponent.
	bset	#23,d0		| Set I bit.
	roll    #1,d1
        roll    #8,d1
        andw    #0xff,d1        | d1 gets biased exponent.
        subw    #23+0x7f,d1     | d1 gets unbiased exponent - 23 = left shift count.
        beqs	Funsdone	| Branch if no shift indicated.
	bmis	3f		| Branch if right shift indicated.
	lsll	d1,d0		| Left shift.
	bras	Funsdone
3:
	negw	d1		| Reverse count.
	lsrl	d1,d0		| Right shift.
Funsdone:
        RET

/*
 * single floating to signed integer conversion:
 * input:
 *	d0	floating value
 * output:
 *	d0	signed integer value
 */

RTENTRY(Fints)
        movel   d0,d1           | d1 gets x.
        bclr    #31,d1          | d1 gets abs(x).
        cmpl    #0x3f800000,d1
        bges    ge1             | Branch if abs(x) >= 1.
        clrl    d0  		| Result is zero.
        bras    Fintsdone
ge1:
        cmpl    #0x4f000000,d1
        bles    1f        | Branch if 1 <= |x| <= 2**31.
        tstl    d0
        bmis    4f              | Branch if x is negative sign.
        movel   #0x7fffffff,d0  | Maximum positive integer.
        bras    Fintsdone
4:
        movel   #0x80000000,d0  | Maximum negative integer.
        bras    Fintsdone
1:
        movel   d0,a0           | Save x.
	andl	#0x7fffff,d0	| Clear sign and exponent.
	bset	#23,d0		| Set I bit.
	roll    #1,d1
        roll    #8,d1
        andw    #0xff,d1        | d1 gets biased exponent.
        subw    #23+0x7f,d1     | d1 gets unbiased exponent - 23 = left shift count.
        beqs	Fintsign	| Branch if no shift indicated.
	bmis	Fintsright	| Branch if right shift indicated.
Fintsleft:
	lsll	d1,d0		| Left shift.
	bras	Fintsign
Fintsright:
	negw	d1		| Reverse count.
	lsrl	d1,d0		| Right shift.
Fintsign:
	cmpl	#0,a0
	bges	Fintsdone	| Branch if x was non-negative.
	negl	d0		| Negate result.
Fintsdone:
        RET

RTENTRY(Fnints)
        movel   d0,d1           | d1 gets x.
        bclr    #31,d1          | d1 gets abs(x).
        cmpl    #0x3f000000,d1
        bges    gehalf          | Branch if abs(x) >= 0.5.
        clrl    d0  		| Result is zero.
        bras    Fintsdone
gehalf:
        cmpl    #0x4f000000,d1
        bles    1f        | Branch if 0.5 <= |x| <= 2**31.
        tstl    d0
        bmis    4f              | Branch if x is negative sign.
        movel   #0x7fffffff,d0  | Maximum positive integer.
        bras    Fintsdone
4:
        movel   #0x80000000,d0  | Maximum negative integer.
        bras    Fintsdone
1:
        movel   d0,a0           | Save x.
	andl	#0x7fffff,d0	| Clear sign and exponent.
	bset	#23,d0		| Set I bit.
	roll    #1,d1
        roll    #8,d1
        andw    #0xff,d1        | d1 gets biased exponent.
        subw    #23+0x7f,d1     | d1 gets unbiased exponent - 23 = left shift count.
        beqs	Fintsign	| Branch if no shift indicated.
	bpls	Fintsleft	| Branch if right shift indicated.
	negw	d1		| Reverse count.
	subqw	#1,d1		| Decrement for rounding position.
	lsrl	d1,d0		| Right shift.
	addql	#1,d0		| Round.
	lsrl	#1,d0		| Finish shift.
	jra	Fintsign
 
RTENTRY(Frints)
        movel   d0,d1           | d1 gets x.
        bclr    #31,d1          | d1 gets abs(x).
        cmpl    #0x3f000000,d1
        bgts    gthalf          | Branch if abs(x) > 0.5.
        clrl    d0  		| Result is zero.
        jra    Fintsdone
gthalf:
        cmpl    #0x4f000000,d1
        bles    1f        	| Branch if 0.5 < |x| <= 2**31.
        tstl    d0
        bmis    4f              | Branch if x is negative sign.
        movel   #0x7fffffff,d0  | Maximum positive integer.
        jra     Fintsdone
4:
        movel   #0x80000000,d0  | Maximum negative integer.
        jra     Fintsdone
1:
        movel   d0,a0           | Save x.
	andl	#0x7fffff,d0	| Clear sign and exponent.
	bset	#23,d0		| Set I bit.
	roll    #1,d1
        roll    #8,d1
        andw    #0xff,d1        | d1 gets biased exponent.
        subw    #23+0x7f,d1     | d1 gets unbiased exponent - 23 = left shift count.
        jeq	Fintsign	| Branch if no shift indicated.
	jpl	Fintsleft	| Branch if right shift indicated.
	movel	d2,a1		| a1 saves d2.
	movel	d0,d2
	negw	d1		| Reverse count.
	lsrl	d1,d0		| Right shift.
	negw	d1
	addw	#32,d1
	lsll	d1,d2		| d2 gets round and sticky bits.
	addl	#0x80000000,d2	| Round.
	bccs	2f		| Branch if no carry out.
	addl	#1,d0		| Propagate carry.
	tstl	d2
	bnes	2f		| Branch if not ambiguous.
	bclr	#0,d0		| Force even.
2:
	movel	a1,d2		| Restore d2.
	jra	Fintsign
 
