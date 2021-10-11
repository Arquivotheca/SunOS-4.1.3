	.data
|	.asciz	"@(#)Ffltd.s 1.1 92/07/30 Copyr 1986 Sun Micro"
	.even
	.text

|	Copyright (c) 1986 by Sun Microsystems, Inc.

/*
 *	double-precision floating math run-time support
 *
 *	copyright 1981, 1982 Richard E. James III
 *	translated to SUN idiom 10/11 March 1983 rt
 *	parameter passing re-done 22 July 1983 rt
 */
#include "fpcrtdefs.h"

	SAVEMASK = 0x3c00
	RESTMASK = 0x003c

/*
 * type conversion operators
 *
 * integer-to-double conversion: fflti
 * input:
 *	d0	integer
 * output:
 *	d0/d1	double floating value
 */

RTENTRY(Ffltd)
	movel	d0,a0		| a0 saves x.
	movel	d2,a1		| a1 saves d2.
	movel	#0x41e,d2  	| d2 accumulates exponent.
	tstl	d0
	bpls	3f		| Branch if x is non-negative.
	negl	d0		| d0 gets abs(x).
3:
	bmis	2f		| Branch if d0 is normalized.
	bnes	1f		| Branch if x not zero.
	clrl	d1
	bras	Ffltddone	| Branch if zero.
1:
	subqw	#1,d2		| Adjust exponent.
	lsll	#1,d0		| Normalize.
	bpls	1b		| Branch if already normalized.
2:
	bclr	#31,d0		| Clear I bit.
	movel	d0,d1		| d1 gets significand.
	lsrl	#8,d0		| Align high order bits.
	lsrl	#3,d0		| Align high order bits.
	swap	d2
	lsll	#4,d2		| Align exponent.
	orl	d2,d0		| Stick exponent.
	movel	#21,d2
	lsll	d2,d1		| Align low order bits.
	cmpl	#0,a0
	bges	Ffltddone	| Branch if x was non-negative.
	bset	#31,d0		| Set sign if negative.
Ffltddone:
	movel	a1,d2		| Restore d2.
	RET

/*
 * double-to-integer conversion
 * input:
 *	d0/d1	double floating value
 * output:
 *	d0	unsigned integer
 */

RTENTRY(Fund)
        moveml  d2-d3,sp@-
        movel   d0,d3           | d3 gets top(x).
        cmpl    #0x3ff00000,d0
        bges    1f            	| Branch if (x) >= 1.
        clrl    d0
        bras    Funddone
1:
        cmpl    #0x41f00000,d0
        blts    2f       	| Branch if 1 <= |x| < 2**32.
        movel	#0x-1,d0	| Maximum negative integer.
        bras    Funddone
2:
        movel	d0,d2		| d2 gets abs(x).
	andl	#0xfffff,d0	| Clear sign and exponent.
	bset	#20,d0		| Set I bit.
	roll    #8,d2
        roll    #4,d2
        andw    #0xfff,d2       | d2 gets biased exponent.
        subw    #0x3ff+20,d2
        bges    3f           	| Branch if exp >= 20.
        negw	d2
        lsrl	d2,d0
	bras    Funddone
3:
        beqs    Funddone      	| Branch if exponent = 20 exactly.
	lsll	d2,d0		| Upper shift.
	negw	d2
	addw	#32,d2		| Compute right shift.
	lsrl	d2,d1		| This part goes from lower to upper.
	orl	d1,d0
Funddone:
        moveml  sp@+,d2-d3
        RET

/*
 * double-to-integer conversion
 * input:
 *	d0/d1	double floating value
 * output:
 *	d0	signed integer
 */

RTENTRY(Fintd)
        moveml  d2-d3,sp@-
        movel   d0,d3           | d3 gets top(x).
        bclr    #31,d0          | d0 gets top(abs(x)).
        cmpl    #0x3ff00000,d0
        bges    1f            	| Branch if abs(x) >= 1.
        clrl    d0
        bras    Fintddone
1:
        cmpl    #0x41e00000,d0
        bles    2f       	| Branch if 1 <= |x| <= 2**31.
        tstl	d3
	bmis	4f		| Branch if x is negative sign.
	movel	#0x7fffffff,d0	| Maximum positive integer.
	bras	Fintddone
4:
	movel	#0x80000000,d0	| Maximum negative integer.
        bras    Fintddone
2:
        movel	d0,d2		| d2 gets abs(x).
	andl	#0xfffff,d0	| Clear sign and exponent.
	bset	#20,d0		| Set I bit.
	roll    #8,d2
        roll    #4,d2
        andw    #0xfff,d2       | d2 gets biased exponent.
        subw    #0x3ff+20,d2
        bges    3f           	| Branch if exp >= 20.
        negw	d2
        lsrl	d2,d0
	bras    Fintdsign
3:
        beqs    Fintdsign      	| Branch if exponent = 20 exactly.
	lsll	d2,d0		| Upper shift.
	negw	d2
	addw	#32,d2		| Compute right shift.
	lsrl	d2,d1		| This part goes from lower to upper.
	orl	d1,d0
Fintdsign:
	tstl	d3
	bpls	Fintddone	| Branch if x was not negative.
	negl	d0
Fintddone:
        moveml  sp@+,d2-d3
        RET

RTENTRY(Fnintd)
        moveml  d2-d4,sp@-
        movel   d0,d3           | d3 gets top(x).
        bclr    #31,d0          | d0 gets top(abs(x)).
        cmpl    #0x3fe00000,d0
        bges    1f            	| Branch if abs(x) >= 0.5.
        clrl    d0
        bras    Fnintddone
1:
        cmpl    #0x41e00000,d0
        bles    2f       	| Branch if 0.5 <= |x| <= 2**31.
        tstl    d3
        bmis    4f              | Branch if x is negative sign.
        movel   #0x7fffffff,d0  | Maximum positive integer.
        bras    Fnintddone
4:
        movel   #0x80000000,d0  | Maximum negative integer.
        bras    Fnintddone
2:
        movel	d0,d2		| d2 gets abs(x).
	andl	#0xfffff,d0	| Clear sign and exponent.
	bset	#20,d0		| Set I bit.
	roll    #8,d2
        roll    #4,d2
        andw    #0xfff,d2       | d2 gets biased exponent.
        subw    #0x3ff+20,d2
        bges    3f           	| Branch if exp >= 20.
	negw	d2
        subqw	#1,d2
	lsrl	d2,d0		| Upper part.
	addql	#1,d0		| Round bit.
	lsrl	#1,d0		| Finish shift.
	bras    Fnintdsign
3:
        beqs    Fnintdround     | Branch if exponent = 20 exactly.
	movel	d1,d4		| d4 will get lower to upper part.
	lsll	d2,d0		| Upper shift.
	lsll	d2,d1		| Lower shift.
	negw	d2
	addw	#32,d2		| Compute right shift.
	lsrl	d2,d4		| This part goes from lower to upper.
	orl	d4,d0
Fnintdround:
	addl	#0x80000000,d1
	bccs	Fnintdsign	| Branch if no rounding carry propagate.
	addql	#1,d0
Fnintdsign:
	tstl	d3
	bpls	Fnintddone	| Branch if x was not negative.
	negl	d0
Fnintddone:
        moveml  sp@+,d2-d4
        RET

RTENTRY(Frintd)
        moveml  d2-d4,sp@-
        movel   d0,d3           | d3 gets top(x).
        bclr    #31,d0          | d0 gets top(abs(x)).
        cmpl    #0x3fe00000,d0
        bges    1f            	| Branch if abs(x) >= 0.5.
        clrl    d0
        bras    Fnintddone
1:
        cmpl    #0x41e00000,d0
        bles    2f       	| Branch if 0.5 <= |x| <= 2**31.
        tstl    d3
        bmis    4f              | Branch if x is negative sign.
        movel   #0x7fffffff,d0  | Maximum positive integer.
        bras    Fnintddone
4:
        movel   #0x80000000,d0  | Maximum negative integer.
        bras    Fnintddone
2:
        movel	d0,d2		| d2 gets abs(x).
	andl	#0xfffff,d0	| Clear sign and exponent.
	bset	#20,d0		| Set I bit.
	roll    #8,d2
        roll    #4,d2
        andw    #0xfff,d2       | d2 gets biased exponent.
        subw    #0x3ff+20,d2
        bges    3f           	| Branch if exp >= 20.
	movel	d1,d4		| d4 saves lower part.
	movel	d0,d1
	negw	d2
	lsrl	d2,d0		| Upper part.
	negw	d2
	addw	#32,d2
	lsll	d2,d1		| Lower part.
	tstl	d4
	beqs	Frintdround	| Branch if original low part clear.
	bset	#30,d1		| Set sticky bit to influence rounding.
	bras    Frintdround
3:
        beqs    Frintdround     | Branch if exponent = 20 exactly.
	movel	d1,d4		| d4 will get lower to upper part.
	lsll	d2,d0		| Upper shift.
	lsll	d2,d1		| Lower shift.
	negw	d2
	addw	#32,d2		| Compute right shift.
	lsrl	d2,d4		| This part goes from lower to upper.
	orl	d4,d0
Frintdround:
	addl	#0x80000000,d1
	jcc	Fnintdsign	| Branch if no rounding carry propagate.
	addql	#1,d0
	tstl	d1
	jne	Fnintdsign	| Branch if not ambiguous.
	bclr	#0,d0		| Force round to even.
	jra	Fnintdsign	
