	.data
|	.asciz	"@(#)Fmuld.s 1.1 92/07/30 SMI"
	.even
	.text

|	Copyright (c) 1987 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"

/*
 *	double-precision floating math run-time support
 *
 *	copyright 1981, 1982 Richard E. James III
 *	translated to SUN idiom 10/11 March 1983 rt
 *	parameter passing re-done 22 July 1983 rt
 */

ARG2PTR = a0

/*
 *	ieee double floating divide
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
 *	    d0/d1	divisor
 *	    d2/d3	dividend
 *	    d4		count
 *	    d5		sign an exponent
 *	    d6		result exponent
 *	    d6/d7	quotient
 */
	NSAVED   = 7*4		| 7 registers * sizeof(register)

RTENTRY(Fdivd)
|	save registers and load operands into registers
	moveml	d2-d7/a2,sp@-
	movl	d0,d2
	movl	d1,d3
	movl	ARG2PTR@+,d0
	movl	ARG2PTR@ ,d1
	| save sign of result
	movl	d0,d5
	eorl	d2,d5		| sign of result
	clrl	d4		| flag for divide
	JBSR(extrem,a2)
	| compute resulting exponent
	movw	d6,d5
	subw	d7,d5
	| do top 30-31 bits of divide (d_rcp will post normalize)
	movw	#30,d4	| count 30..0
	bsr	shsub	| shift and subtract loop
	movl	d7,d6	| top of answer
	| do next 22 bits of divide
	movw	#23,d4	| count 23..0 (total = 54-55 bits)
	bsr	shsub
	| put together answer
	lsll	#8,d7	| line up bottom
	orl	d3,d2
	beqs	1$
	bset	#1,d7	| sticky bit on if remainder <> 0
1$:	lsll	#1,d7
	roxll	#1,d6
	| normalize (once), round, check result exp, pack
	movl	d6,d2	| upper
	movl	d7,d3	| lower
	movw	d5,d6	| exponent
	addw	#1023,d6	| re-bias
	JBSR(d_nrcp,a2)	| norm once, rnd, ck, pack
	bra	dmsign


/*
 * round, check for over/underflow, and pack in the exponent.
 * d_nrcp does one normalize and then calls d_rcp.
 * d_rcp rounds the double value and packs the exponent in,
 * catching infinity, zero, and denormalized numbers.
 * d_usel puts together the larger argument.
 * 
 * input:
 *      d2/d3   mantissa (- if norm)
 *      d6      biased exponent
 *      (need sign, sticky)
 * output:
 *      d2/d3   most of number, no sign or hidden bit,
 *              waiting to shift sign in.
 * other:
 *      d4      lost
 *      d5      unchanged
 */

d_nrcp:
        tstl    d2
#ifdef PIC
	jpl positive	| can not jmi to external label if pic
	PIC_SETUP(a2)
	movl	a2@(d_rcp),a2
	jmp	a2@
#else
        jmi     d_rcp   | already normalized
#endif
positive:
        subqw   #1,d6
        lsll    #1,d3   | do extra normalize (for mul/div)
        roxll   #1,d2
#ifdef PIC
	PIC_SETUP(a2)
	movl	a2@(d_rcp),a2
	jmp	a2@	
#else
	jra	d_rcp
#endif


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


/*
 * subroutine for extracting and taking care of 0/GU/INF/NAN.
 * input:
 *	d0/d1, d2/d3
 *	d4	+ for div; - for mod
 *
 * output:
 *	d0/d1 (bottom) and d2/d3 (top) converted to
 *		000XXXXX/XXXXXXXX
 *	d6	exponent of top
 *	d7	exponent of bottom
 *
 * unchanged:
 *	d5	sign
 */

extrem:
	movw	#0x7ff,d4	| mask, sign.l untouched
	exg	d2,d0
	exg	d3,d1
	bsrs	unp	| unpack top
	exg	d0,d2
	exg	d1,d3
	exg	d7,d6
	beqs	topzero	| top is zero
	cmpw	d4,d6
	beqs	topbig	| top is inf or nan
	bset	#20,d2	| set hidden bit
topzero:bsrs	unp	| unpack bottom
	beqs	botzero	| bottom is 0
	cmpw	d4,d7
	beqs	botbig	| bottom is inf or nan
	bset	#20,d0
	lsll	#1,d1	| left shift bottom
	roxll	#1,d0
	rts

/*
 * EXCEPTION HANDLING
 */

topbig:	| top is inf or nan
	bsrs	unp	| unpack bottom
	tstl	d4	
	bmis	isnan	| inf or nan / ...
	cmpw	d6,d7
	beqs	isnan	| both inf/nan
	tstl	d2	| top
	beqs	geninf	| inf / ... = inf
	bras	isnan	| nan / ... = nan

botzero:| bottom is 0
	tstl	d4
	bmis	gennan	| dmod(... 0) = nan
	orl	d2,d3	| top
	beqs	gennan	| 0/0 = nan
			| nonzero/0 = inf
	| generate infinity for answer
geninf:	movl	#0x7ff00000*2,d2	| infinity
	bras	clrbot

botbig:	tstl	d0	| bottom = nan, result = nan
	bnes	isnan
	tstl	d4
	bpls	genzero	| ... / inf = 0
	addqw	#4,sp
	addw    #11,d6 		| append sign
        JBSR(d_norm,a2)          | normalize
        JBSR(d_rcp,a2)           | adjust exp, ck extremes, pack
dmsign:	roxll	#1,d5		| sign -> x
	roxrl	#1, d2		| rotate in sign
	| answer is now in:
	|	d2	most significant 32 bits
	|	d3	least significant 32 bits
dmexit:	movl	d2,d0
	movl	d3,d1
	| restore registers and split
	moveml	sp@+,d2-d7/a2
	RET


	| invalid operand/operation
isnan:	cmpw	d7,d6
	bnes	1$	| exponents equal
	cmpl	d0, d2
1$:	bges	2$
	movw	d7,d6
	movl	d0,d2
2$:	swap	d2
	lslw	#(16-1-11),d6
	orw	d6,d2	| put back together
	swap	d2
	lsll	#1,d2
	cmpl	#0x7ff00000*2, d2
	bhis	gotnan	| use larger nan

gennan:	movl	#0x7ff00004*2,d2
	tstl	d4
	bpls	gotnan	| nan 4 for div
	addql	#2,d2	| nan 5 for mod
gotnan:	clrl	d5
	bras	clrbot

genzero:clrl	d2
clrbot:	clrl	d3
sign:	addqw	#4,sp
	bra	dmsign

/*
 *  the shsub subroutine does a shift-subtract loop
 * that is the heart of divide and mod.
 * the algorithm is a simple shift and subtract loop,
 * but it adds when it overshoots.
 * why not use the divs/divu instructions? That approach is slower!
 *
 * registers:
 *	d2/d3	current dividend (updated)
 *	d0/d1	divisor (unchanged)
 *	d4.w	(inpout) number of interations -1, and bit number
 *	d5/d6	-untouched-
 *	d7	quotient being devloped (ignored by mod)
 */


shsub:
	clrl	d7	| quotient
	| shift once, see if subtract needed
1$:	addl	d3,d3
	addxl	d2,d2	|(64-bit left shift)
	cmpl	d0,d2
	dbge	d4,1$	| loop while divident is small
	| tally quotient and subtract
	bset	d4,d7	| quotient bit
	subl	d1,d3
	subxl	d0,d2	| 64-bit subtract
	dbmi	d4,1$	| loop (d4) times
	| now one of three things has happeded:
	| 1. count exhausted and extra subtract done (first DB hit count)
	| 2. count exhausted in second DB
	| 3. overshot because compare didn't check lower parts
	bpls	2$	| case 2
	addl	d1,d3	| take care of overshoot
	addxl	d0,d2
	bclr	d4,d7
	tstw	d4
	dblt	d4,1$	| case 3
			| case 1
2$:	rts

/*
 *	ieee double floating multiply
 *	copyright 1981, Richard E. James III
 *	translated to SUN idiom 10 March 1983 rt
 */

/*
	Revised to do correct IEEE rounding by dgh 24 Aug 84
 *	Conventions in the main multiplication section are as follows:
 *	r is the argument passed in the registers d0/d1
 *	s is the argument passed on the stack, saved in a0/a1
 *		while multiplying
 *	r1,r2,r3,r4 are the 16 bit components of r in descending order
 *      s1,s2,s3,s4 are the 16 bit components of s
 *	r is kept in d0 and d1, sometimes with words swapped
 *	s is kept in a0 and a1 unchanged
 *      the product is kept in d2/d3/d4/d5
 *	d6 contains a current partial product
 *	d7 contains a current partial product
 *	d7 contains 0 when needed for addxl
 *	a2 saves the sign and exponent of the result
 *
 *	At the end, d4 and d5 if nonzero are jammed into lsb of d3.
 */
/*
 *	entry conditions:
 *	    first argument in d0/d1
 *	    second argument on stack
 *	exit conditions:
 *	    result (8 bytes) in d0/d1
 *
 *	register conventions:
 *	    d0-d3	operands or pieces of result
 *	    d5		exponent of larger
 *	    d5		temp for multiplying
 *	    d6		sign and exponent
 *	    d7		exponent of smaller
 *	    d7		zero
 */
	NSAVED   = 9*4		| 9 registers * sizeof(register)

	SAVE03	 = 0xf000	| registers d0-d3
	FETCH03  = 0x000f

RTENTRY(Fsqrd)
	moveml	d2-d7/a0-a2,sp@-
	movel	d0,d2
	movel	d1,d3
	bras	Fmuld2
RTENTRY(Fmuld)
|	save registers and load operands into registers
	moveml	d2-d7/a0-a2,sp@-
	movl	ARG2PTR@+,d2
	movl	ARG2PTR@ ,d3
Fmuld2:
				| save sign of result
	movl	d0,d5
	eorl	d2,d5		| sign of result
	asll	#1,d0		| toss sign
	asll	#1,d2		| EEmmmm0
	cmpl	d2,d0
	| order operands (exponents at least)
	blss	eswap
	exg	d0,d2		| d2/d3 = larger
	exg	d1,d3
	| extract and check exponents
eswap:	JBSR(d_exte,a2)
	movw	d6,d5		| larger exp
	movl	d5,d6
	addw	d7,d6		| result exp (and sign)
	cmpw	#0x7ff,d5 	| check larger
	jeq	ovfl		| inf or nan
	tstw	d7
	jeq	ufl		| 0 or 	gradual underflow
	| set hidden bits
	bset	#31,d0
back:	bset	#31,d2

|		Main multiply sequence.

	movl	d2,a0		| a0 gets s1s2.
	movl	d3,a1		| a1 gets s3s4.
	movl	d6,a2		| a2 saves sign and exponent of result.

	movl	a0,d3		| d3 gets s1s2.
	swap	d3		| d3 gets s2s1.
	movw	d3,d4		| d4 gets s1.
	mulu	d1,d4		| d4 gets r4*s1.
	mulu	d0,d3		| d3 gets r2*s1.
	clrw	d2		| d2 gets 0; only gets carries in this phase.

	movl	a1,d6		| d6 gets s3s4.
	swap	d6		| d6 gets s4s3.
	movw	d6,d5		| d5 gets s3.
	jeq	s3zero		| Branch if s3=0 to avoid following.
	mulu	d1,d5		| d5 gets r4*s3.
	movw	d6,d7		| d7 gets s3.
	mulu	d0,d7		| d7 gets r2*s3.
	addl	d7,d4
	clrl	d7		| Make a zero for addx.
	addxl	d7,d3
	addxw	d7,d2
phase3:
	swap	d0		| d0 gets r2r1.
	swap	d1		| d1 gets r4r3.
	movw	a0,d6		| d6 gets s2.
	beqs	4$		| Skip following if s2=0.
	movw	d6,d7		| d7 gets s2.
	mulu	d0,d6		| d6 gets r1*s2.
	mulu	d1,d7		| d7 gets r3*s2.
	addl	d7,d4
	addxl	d6,d3
	clrw	d7
	addxw	d7,d2
4$:
	movw	a1,d6		| d6 gets s4.
	beqs	5$		| Skip if s4=0.
	movw	d6,d7	 	| d7 gets s4.
	mulu	d0,d6		| d6 gets r1*s4.
	mulu	d1,d7		| d7 gets r3*s4.
	addl	d7,d5
	addxl	d6,d4
	clrl	d7
	addxl	d7,d3
	addxw	d7,d2
5$:
	swap	d2		| Exchange order of registers which contain
	swap	d3		| all the "odd" partial products.
	swap	d4
	swap	d5
	movw	d3,d2		| It's really a 16 bit shift!
	movw	d4,d3
	movw	d5,d4
	clrw	d5

	movl	a1,d6		| d6 gets s3s4.
	swap	d6		| d6 gets s4s3.
	movw	d6,d7		| d7 gets s3.
	beqs	6$
	mulu	d0,d6		| d6 gets r1*s3.
	mulu	d1,d7		| d7 gets r3*s3.
	addl	d7,d4
	addxl	d6,d3
	clrl	d7
	addxl	d7,d2

6$:
	movl	a0,d6		| d6 gets s1s2.
	swap	d6		| d6 gets s2s1.
	movw	d6,d7		| d7 gets s1.
	mulu	d0,d6		| d6 gets r1*s1.
	mulu	d1,d7		| d7 gets r3*s1.
	addl	d7,d3
	addxl	d6,d2

	swap	d0		| d0 gets r1r2.
	swap	d1		| d1 gets r3r4.
	movw	a0,d6		| d6 gets s2.
	beqs	8$
	movw	d6,d7		| d7 gets s2.
	mulu	d0,d6		| d6 gets r2*s2.
	mulu	d1,d7		| d7 gets r4*s2.
	addl	d7,d4
	addxl	d6,d3
	clrl	d7
	addxl	d7,d2
8$:
	movw	a1,d6		| d6 gets s4.
	beqs	9$
	movw	d6,d7		| d7 gets s4.
	mulu	d0,d6		| d6 gets r2*s4.
	mulu	d1,d7		| d7 gets r4*s4.
	addl	d7,d5
	addxl	d6,d4
	clrl	d7
	addxl	d7,d3
	addxl	d7,d2
9$:
	orl	d5,d4
	beqs	10$		| Branch if no sticky bits.
	bset	#0,d3		| Set that sticky!
10$:
	movl	a2,d6		| Restore sign/exponent of result.

	subw	#1022,d6	| toss xtra bias
	JBSR(d_nrcp,a2)		| norm, rnd, ck, pack
msign:	roxll	#1,d6
	roxrl	#1,d2		| append sign
mexit: | answer is now in d2/d3: put in d0/d1
	movl	d2,d0
	movl	d3,d1
	| restore registers and split
	moveml	sp@+,d2-d7/a0-a2
	| slide down return address to pop off junk
	RET

s3zero:
	clrl	d5		| We do need to clear d5 sometime.
	jra	phase3

| EXCEPTION CASES

ovfl:	movl	d2,d5	| larger mantissa, if it is nan, use it
	orw	d7,d5	| smaller exponent
	orl	d0,d5
	orl	d1,d5	| smaller value
	beqs	m_gennan| inf * 0
	| else nan*x or inf*nonzero:
	movw	#0x7ff,d6
	JBSR(d_usel,a2)
	bras	msign

ufl:	movl	d0,d7	| mantissa of smaller
	orl	d1,d7
	beqs	signed0	| 0*number
normu:	subqw	#1,d6
	lsll	#1,d1	| adjust denormalized number
	roxll	#1,d0
	bpls	normu
	addqw	#1,d6
	jra	back
	| (if both are denormalized, answer will be zero anyway)
m_gennan:movl	#0x7ff00002,d2
	bras	mexit
signed0:clrl	d2
	clrl	d3
	bras	msign

	EXP	= d2
	TYPE	= d3
	/* type values: */
	    ZERO  = 1 | wonderful
	    GU    = 2
	    PLAIN = 3
	    INF   = 4
	    NAN   = 5

RTENTRY(Fscaleid)
	moveml	d2-d7/a2,sp@-
	JBSR(d_unpk,a2)
	cmpb	#PLAIN,TYPE	| is it a funny number?
	bgts	gohome		| yes -- return argument
	cmpb	#ZERO,TYPE
	beqs	gohome		| is zero -- return arg
	| normal path through here
	movw	EXP,d7		
	extl	d7		| d7 gets long exponent.
	addl	a0@,d7		| d7 gets modified exponent.
	bvcs	nooflo		| Branch if no overflow.
	bmis	posov		| Branch if positive overflow to -.
				| Branch if negative overflow to +.
negov:
	movw	#-2000,EXP
	bras	gohome
posov:	
	movw	#2000,EXP
	bras	gohome
nooflo:
	cmpl	#2000,d7
	bges	posov
	cmpl	#-2000,d7
	bles	negov
	movw	d7,EXP		| Final OK exponent.
gohome:
	JBSR(d_pack,a2)
gone:	moveml	sp@+,d2-d7/a2
	RET
