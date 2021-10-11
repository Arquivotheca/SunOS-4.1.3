	.data
|	.asciz	"@(#)Faddd.s 1.1 92/07/30 Copyr 1985 Sun Micro" 
	.even
	.text

|	Copyright (c) 1985 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"
#include "DEFS.h"

/*
 *	double-precision floating math run-time support
 *
 *	copyright 1981, 1982 Richard E. James III
 *	translated to SUN idiom 10/11 March 1983 rt
 *	parameter passing re-done 22 July 1983 rt
 */

ARG2PTR = a0

/*
 * extract exponents from two double-precision numbers.
 *
 * input:
 *	d0/d1 one operand
 *	d2/d3 other operand
 *
 * output:
 *	d0/d1 mantissa, waiting for hidden bit to be turned on
 *	d2/d3 other mantissa, likewise
 *	d6	exponent from d2/d3
 *	d7	exponent for d0/d1
 *
 * destroys d4
 */

ENTER(d_exte)
	moveq	#11,d4	| size of exponent
	roll	d4,d0
	roll	d4,d2
	roll	d4,d1
	roll	d4,d3
	movl	#0x7ff,d6
	movl	d6,d7
	andl	d2,d6
	eorl	d6,d2
	movl	d7,d4
	andl	d3,d4
	eorl	d4,d3
	lsrl	#1,d2
	orl	d4,d2
	| end transformation of larger
	movl	d7,d4
	andl	d0,d7
	eorl	d7,d0
	andl	d1,d4
	eorl	d4,d1
	lsrl	#1,d0
	orl	d4,d0
	| end transformation of smaller
	rts

/*
 *	ieee double floating compare
 *	copyright 1981, Richard E. James III
 *	translated to SUN idiom 30 March 1983 rt
 */

/*
 *	entry conditions:
 *	    first argument in d0/d1
 *	    second argument on stack
 *	exit conditions:
 *	    result in cc -- carry flag set if either a NAN
 *	problems:
 *	    unordered cases (e.e.: projective infinities and NANs)
 *	    produce random results.
 *	    A NAN, however, does compare not equal to anything.
 *
 *	register conventions:
 *	    d0/d1	first operand
 *	    d2/d3	second operand
 *	    d4		scratch
 */
	SAVEMASK = 0x3800	| registers d2-d4
	RESTMASK = 0x1c
	NSAVED   = 3*4		| 6 registers * sizeof(register)
	CODE	 = NSAVED

RTENTRY(Fcmpd)
	subql	#2,sp	| save room for result
|	save registers and load operands into registers
	moveml	#SAVEMASK,sp@-
	movl	ARG2PTR@+,d2
	movl	ARG2PTR@ ,d3
	| we are now set up.
	movl	d2,d4
	andl	d0,d4		| compare signs
	bpls	nbothmi
	exg	d0,d2		| both minus
	exg	d1,d3
nbothmi:cmpl	d2,d0		| main compare
	bnes	gotcmp		| got the answer
	movl	d1,d4
	subl	d3,d4		| compare lowers
	beqs	gotcmp		| entirely equal
	roxrl	#1,d4
	andb	#0xa,cc		| clear z, in case differ by 1 ulp
gotcmp:	andb	#0xe,cc		| clear carry
        bgts    1f
        blts    2f
        movw    #FEQ,sp@(CODE)
        bras    3f
1:
        movw    #FGT,sp@(CODE)
        bras    3f
2:
        movw    #FLT,sp@(CODE)
3:
 	lsll	#1,d0
	cmpl	#0xfffe00000,d0
	bcss	bnanFcmpd	| A not a nan
	bhis	nanFcmpd	| A is nan
	tstl	d1
	bnes	nanFcmpd	| A is nan
bnanFcmpd:
	lsll	#1,d2
	cmpl	#0xfffe00000,d2
	bcss	nonanFcmpd	| B not a nan
	bhis	nanFcmpd	| B is nan
	tstl	d3
	beqs	nonanFcmpd	| B is inf
nanFcmpd:
	movw	#FUN,sp@(CODE)	| c, nz
	bras	doneFcmpd	| one was a nan
nonanFcmpd:
	orl	d1,d0
	orl	d2,d0
	orl	d3,d0
	bnes	doneFcmpd
	movw	#FEQ,sp@(CODE)	| -0 == 0
	| done, now go
doneFcmpd:	
	moveml	sp@+,#RESTMASK	| put back saved registers
	movw	sp@+,cc		| install condition code
	RET


/*
 *	ieee double floating add
 *	copyright 1981, Richard E. James III
 *	translated to SUN idiom 10 March 1983 rt
 */

/*
 *	entry conditions:
 *	    first argument in d0/d1
 *	    second argument on stack
 *	exit conditions:
 *	    result (8 bytes) in d0/d1
 *
 *	register conventions:
 *	    d0/d1	smaller operand (d0=most significant)
 *	    d2/d3	larger operand
 *	    d4		11 or mask of 11 bits
 *	    d5		signs: sign of .w = sign of answer
 *			       sign of .b = comparison of signs
 *	    d6		exponent of larger
 *	    d7		exponent of smaller
 */
	SAVEMASK = 0x3f00	| registers d2-d7
	RESTMASK = 0xfc
	NSAVED   = 6*4		| 6 registers * sizeof(register)

RTENTRY(Fsubd)
|	save registers and load operands into registers
	moveml	#SAVEMASK,sp@-	| registers d2-d7
	movl	ARG2PTR@+,d2
	movl	ARG2PTR@ ,d3
	bchg	#31,d2
	jra	adding
RTENTRY(Faddd)
|	save registers and load operands into registers
	moveml	#SAVEMASK,sp@-	| registers d2-d7
	movl	ARG2PTR@+,d2
	movl	ARG2PTR@ ,d3
adding:
	| extract signs
	asll	#1,d0	| sign ->c
	scs	d4	| c -> d4
	asll	#1,d2
	scs	d5
	| compare and exchange to put larger in d0/d1
	cmpl	d2,d0
	blss	1$
	exg	d0,d2
	exg	d1,d3
	exg	d4,d5
1$:	extw	d5	| sign of larger
	eorb	d4,d5	| comparison of signs
	| extract exponents
	JBSR(d_exte,a0)	| larger ->d2/d3,d6; smaller ->d0/d1,d7
	tstw	d7
	bnes	2$	| not zero or denormalized
	| here, smaller is zero or is denormalized
	movl	d0,d4
	orl	d1,d4
	jeq	signofzero | if smaller == 0 use larger
	| (sign of 0-0 unpredictable)
	lsll	#1,d1
	roxll	#1,d0
	tstw	d6	| larger exp
	bnes	3$	| not gradual underflow
	lsll	#1,d3
	roxll	#1,d2
	bras	addorsub | both gradual-underflow, no hidden or align needed
2$:	bset	#31,d0	| add hidden bit
3$:	cmpw	#0x7ff,d6
	jeq 	a_ovfl	| inf/nan
	bset	#31,d2
	| align smaller
	| shift-by-eight loop
	subw 	d6,d7
	negw	d7	| d7 = difference of exponents
	cmpw	#16,d7
	jge	rsge16	| Branch if shift of 16 or more.
rs015:			| Right shift 0..15.
	subqw	#8,d7
	blts	5$	| exit loop when difference <8

	tstb	d1
	beqs	99$	| Branch if no bits to lose in shift.
	bset	#8,d1	| Turn on the sticky bit if any bits will be lost.
99$:
	movb	d0,d1	| shift eight bits down
	rorl	#8,d1
	lsrl	#8,d0
	bras	rs015
5$:	addqw	#7,d7
	bmis	addorsub
	tstb	d1
	beqs	98$
	bset	#8,d1	| Turn on sticky bit.
98$:
6$:	lsrl	#1,d0
	roxrl	#1,d1
	dbra	d7,6$	| final  part of alignment
addorsub:
	| decide whether to add or subtract
	tstb	d5	| compare signs
	bmis	diff
	| add them
	addl	d1,d3	| sum
	addxl	d0,d2
	bccs	endas	| no c, ok
	roxrl	#1,d2
	roxrl	#1,d3
	addqw	#1,d6
	cmpw	#0x7ff,d6
	blts	endas	| no overflow
	jra	a_geninf

rsge16:			| Right shift 16 or more.
	cmpw	#32,d7
	blts	rs1631	| Branch if shift is 16..31.
	cmpw	#64,d7
	blts	rs3263	| Branch if shift is 32..63.
	clrl	d0	| Top will be zero.
	moveq	#1,d1	| Bottom will be sticky.
	bras	addorsub
rs3263:			| Shift 32.
	tstl	d1
	beqs	1$
	bset	#0,d0   | Sticky bit on.
1$:
	movl	d0,d1
	clrl	d0
	subw	#32,d7
	cmpw	#16,d7
	blts	rs015	| Branch if shift < 16.
rs1631:			| Shift 16.
	tstw	d1	
	beq	2$	| Branch if no bits in D.
	bset	#16,d1	| Turn on sticky bit in C.
2$:
	clrw	d1	| d1 gets Cs,0.
	movw	d0,d1	| d1 gets Cs,B.
	swap	d1	| d1 gets B,Cs.
	clrw	d0	| d0 gets A,0.
	swap	d0	| d0 gets 0,A.
	subw	#16,d7
	jra	rs015


		| subtract then
diff:	subl	d1,d3	| subtract lowers
	subxl	d0,d2	| subtract uppers
	bccs	9$
	| cancelled down into 2nd word, but got wrong sign
	notw	d5	| flip result sign
	negl	d3
	negxl	d2	| negate value
9$:	bnes	subrenorm | Branch if result nonzero.
	tstl	d3
	bnes	subrenorm | Branch if result nonzero.
	clrw	d5	| Exact zero result has positive sign.
subrenorm:		| Renormalize result after cancellation.	
	JBSR(d_norm,a0)	
	| rejoin, round
endas:	JBSR(d_rcp,a0)	| round, check, and pack
assgn:	lslw	#1,d5	| get sign
	roxrl	#1,d2	| put in sign

	| answer is now in d2/d3: put in d0/d1
	movl	d2,d0
	movl	d3,d1
asexit:	| restore registers and split
	moveml	sp@+,#RESTMASK
	RET

| EXCEPTION CASES
signofzero:		| Set up proper sign for exact zero.
	tstb	d5
	beqs	useln	| Branch if signs equal: either will do.
	tstw	d6
	bnes	useln	| Branch if not zero or subnormal.
	tstl	d2
	bnes	useln	| Branch if subnormal.
	tstl	d3
	bnes	useln	| Branch if subnormal.
	clrw	d5	| Signs unequal so set positive.

useln:	tstw	d6
	beqs	usel	| Branch if subnormal: don't set i bit.
	bset	#31,d2  | Set i bit of normal number. 
usel:	JBSR(d_usel,a0)	| use the larger
	bras	assgn

| larger exponent = 1-23
a_ovfl:	movl	d2,d4	| larger mantissa
	orl	d3,d4
	bnes	usel	| larger = nan, use it
	cmpw	d6,d7	| exps
	bnes	usel	| larger=inf and smaller=number
	| (need nan...)
	tstb	d5	| comparison of signs
	bpls	usel	| inf+inf=inf; inf-inf=nan
	movl	#0x7ff00001,d0	| NAN
	clrl	d1
	bras asexit
| result overflows
a_geninf:
	movl	#0xffe00000,d2
	clrl	d3
	bras	assgn

/*
 * subroutine for unpacking one operand, and normalizing a denormalized number
 * input:
 *	d0/d1	number
 * output:
 *	d0/d1	mantissa
 *	d7.w	exponent
 *	z	on iff mantissa is zero_
 *
 * unchanged:
 *	d4	bottom = 0xf77
 */

unp:	movl	d0,d7	| start getting exp
	andl	#0xfffff,d0	| clear out sign and exp
	swap	d7
	lsrw	#(16-1-11),d7
	andw	d4,d7	| expondnt
	bnes	3$	| normal number
	| denormalized number or zero:
	tstl	d0	| upper
	bnes	1$
	tstl	d1	| lower
	beqs	3$	|zero
1$:	addqw	#1,d7
2$:	subql	#1,d7
	lsll	#1,d1
	roxll	#1,d0	| normalize
	btst	#20,d0
	beqs	2$	| loop until normalized
3$:	rts
