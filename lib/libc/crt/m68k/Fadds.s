	.data
|	.asciz	"@(#)Fadds.s 1.1 92/07/30 Copyr 1986 Sun Micro"
	.even
	.text

|	Copyright (c) 1986 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"

/*
 *	ieee single floating run-time support
 *	copyright 1981, Richard E. James III
 *	translated to SUN idiom 14 March 1983 rt
 */

/*	single floating addition and subtraction	*/
/*
 *	entry conditions:
 *	    first argument in d0
 *	    second argument in d1
 *	exit conditions:
 *	    result (4 bytes) in d0
 *
 *	register conventions:
 *	    d0		operand 1
 *	    d1		operand 2/sum
 *	    d2		sign of operand 1
 *	    d3		sign of operand 2
 *	    d4		exponent of operand 1
 *	    d5		exponent of operand 2
 *	    d6		difference of exponents
 *	    d7		reserved for sticky
 */
	SAVEMASK = 0x3f00	| registers d2-d7
	RESTMASK = 0xfc
	NSAVED   = 6*4		| 6 registers * sizeof(register)

RTENTRY(Fsubs)
	bchg	#31,d1
	jra	adding
RTENTRY(Fadds)
adding:
|	save registers and load operands into registers
	moveml	#SAVEMASK,sp@-	| registers d2-d7
	| extract signs
	asll	#1,d0	| sign ->c
	scs	d2	| c -> d4
	asll	#1,d1
	scs	d3
	| compare and exchange to put larger in d0
	cmpl	d1,d0
	blss	1$
	exg	d0,d1
	exg	d2,d3
	| extract exponents
1$:	roll	#8,d1
	roll	#8,d0
	clrl	d5
	movb	d1,d5		| larger exp
	clrw	d4
	movb	d0,d4		| smaller exp
	bnes	3$		| not zero or denormalized
	| smaller is 0 or is denormalized
	tstl	d0		| if smaller == 0
	jeq	signofzero	| then use larger (sign of 0-0 unpredictable)
	tstb	d5		| larger exponent
	bnes	4$		| not gu
	| both are denormalized
	cmpb	d3,d2		| compare signs
	bnes	2$
	addl	d0,d1
	addxb	d1,d1		| incr exp to 1 if overflow
	jra 	asbuild
2$:	subl	d0,d1		| subtract
	jne 	asbuild
	jra	cancel
| neither is denormalized
3$:	movb	#1,d0		| clear exp and
	rorl	#1,d0
4$:	movb	#1,d1		| ... clr hidden bit
	rorl	#1,d1
	cmpb	#0xff,d5
	jeq 	ovfl		| inf or nan
	| align smaller
	movw	d5,d6
	subw	d4,d6		| difference of exponents
	cmpw	#16,d6
	bges	bigshift	| Branch if shift 26 or more places.
check8:	cmpw	#8,d6
	blts	shortshift	| exit when <8 *s
	tstb	d0
	beqs	shift8		| Branch if no bits to be lost.
	bset 	#8,d0		| Sticky bit on.
shift8:
	subqw	#8,d6
	lsrl	#8,d0		| align
	bra	check8		| loop
bigshift:
	cmpw	#26,d6
	bges	greatshift	| Branch if only sticky left.
	tstw	d0
	beqs	1$
	bset	#16,d0		| Set sticky bit.
1$:
	clrw	d0
	swap	d0
	subw	#16,d6
	bras	check8
greatshift:
	moveq	#1,d0		| Here's a sticky bit.
	bras	addorsub
shortshift: 
	movb	d0,d7
	lsrl	d6,d0		| finish align
	cmpw	#2,d6
	bges	stickit		| Branch if shift >=2 so whole byte sticky.
	andb	#1,d7		| Shift = 0 or 1 so only lsb sticks.
stickit:
	tstb	d7
	beqs	addorsub	| Branch if no bits to stick.
	bset	#1,d0		| Set a sticky bit on.
				| Bset #0 wouldn't work if round carried out.
addorsub:
	cmpb	d3,d2		| cmp signs
	bnes	diff		| decide whether to add or subtract

	| add them
	addl	d0,d1		| sum
	bccs	endas		| no carry here
	roxrl	#1,d1		| pull in overflow *s
	addqw	#1,d5
	cmpw	#0xff,d5
	blts	endas		| no ofl
	bras	a_geninf

	
signofzero:			| Set proper sign of zero.
	cmpb	d2,d3
	beqs	usel		| If signs are equal, use d3.
	tstl	d1
	bnes	usel		| Branch if not zero.
	clrb	d3		| Sign if positive otherwise.
	bras	usel

| subtract them
diff:	subl	d0,d1
	bmis	endas		| if normalized
	beqs	cancel		| result == 0
9$:	asll	#1,d1		| normalize
	dbmi	d5,9$		| dec exponent
	subqw	#1,d5
	bgts	endas		| not gu
	beqs	10$
	clrw	d5
	lsrl	#1,d1		| grad und
10$:	lsrl	#1,d1

endas:	| round (NOT FULLY  STANDARD)
	addl	#0x80,d1	| round
	bccs	testeven	| round did not cause mantissa to ofl
	roxrl	#1,d1
	addqw	#1,d5
	cmpw	#0xff,d5
	beqs	a_geninf	| round caused exp to overflow
	bras	rebuild
testeven:			| Test if ambiguous case.
	tstb	d1
	bnes	rebuild		| Branch if not ambiguous.
	bclr	#8,d1		| Force round to even.
rebuild:			| rebuild answer
	lsll	#1,d1		| toss hidden
usel:	movb	d5,d1		| insert exponent
asbuild:rorl	#8,d1
	roxrb	#1,d3
	roxrl	#1,d1		| apply sign
	| d1 now has answer
	movl	d1,d0
asexit:
	moveml	sp@+,#RESTMASK
	RET

	| EXCEPTION CASES
ovfl:	| largest exponent = 255
	lsll	#1,d1
	tstl	d1		| larger mantissa
	bnes	usel		| larger == nan
	cmpb	d4,d5		| exps
	bnes	usel		| larger == inf
	| AFFINE MODE ASSUMED IN THIS IMPLEMENTATION
	cmpb	d3,d2		| signs
	beqs	usel		| inf+inf = inf
|gennan:
	movl	#0x7f800001,d0
	bras	asexit

	| complete cancellation
cancel:	clrl	d0		| result == 0
	| need minus 0 for (-0)+(-0), round to -inf
	bras	asexit
	| result overflows:
a_geninf:movl	#0xff,d1
	bras	asbuild


/*
 *	ieee single floating compare
 *	copyright 1981, Richard E. James III
 *	translated to SUN idiom 29 March 1983 rt
 */

/*
 *	entry conditions:
 *	    first argument in d0
 *	    second argument in d1
 *	exit conditions:
 *	    result in condition code
 *	    d0/d1 trashed
 *
 *	register conventions:
 *	    d0		operand 1
 *	    d1		operand 2
 *	    d2		scratch
 */
	NSAVED	= 4
	CODE	= NSAVED

RTENTRY(Fcmps)
#ifdef PROF
	unlk	a6		| don't get in the way of the cc.
#endif
	subqw	#2,sp		| save space for condition code return
	movl	d2,sp@-		| save register

	movl	d1,d2
	andl	d0,d2		| compare signs
	bpls	1$
	exg	d0,d1		| both minus
1$:	cmpl	d1,d0		| main compare
	bgts	1f
	blts	2f
	movw	#FEQ,sp@(CODE)
	bras	3f
1:
	movw	#FGT,sp@(CODE)
	bras	3f
2:
	movw	#FLT,sp@(CODE)
3:
	lsll	#1,d0
	lsll	#1,d1
	cmpl	d1,d0
	bccs	2$
	exg	d0,d1	| find larger
2$:	cmpl	#0xff000000,d0
	blss	3$
	| nan
	movw	#FUN,sp@(CODE)	| c for unordered
3$:	tstl	d0
	bnes	4$
	movw	#FEQ,sp@(CODE)	| -0 == 0
	| result is in sp@(CODE)
4$:	| restore saved register and go
	movl	sp@+,d2
	rtr
