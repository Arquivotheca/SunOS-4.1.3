	.data
|	.asciz	"@(#)Fmuls.s 1.1 92/07/30 SMI"
	.even
	.text

|	Copyright (c) 1987 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"

/*
 *	ieee single floating multiply
 *	copyright 1981, 1982 Richard E. James III
 *	translated to SUN idiom 14 March 1983 rt
 */

/*
 *	entry conditions:
 *	    first argument in d0
 *	    second argument in d1
 *	exit conditions:
 *	    result (4 bytes) in d0
 *
 *	register conventions:
 *	    d0		operand1/upper1
 *	    d1		operand2/upper2
 *	    d2		9/lower1
 *	    d3		lower2
 *	    d4		exponent
 *	    d5		sign
 */
	NSAVED   = 5*4		| 5 registers * sizeof(register)

RTENTRY(Fsqrs)
	movel	d0,d1		| Copy second argument.
	bras	Fmuls2
RTENTRY(Fmuls)
|	save registers
Fmuls2:
	moveml	d2-d5/a2,sp@-	| registers 
	| save sign of result
	movl	d0,d5
	eorl	d1,d5		| sign of result
	asll	#1,d0		| toss sign
	asll	#1,d1		| EEmmmm0
	cmpl	d1,d0
	| order operands (exponents at least)
	blss	eswap
	exg	d0,d1		| d1 = larger
	| extract and check exponents
eswap:	roll	#8,d0
	roll	#8,d1		| mmmmm0ee
	clrw	d4
	movb	d0,d4
	clrw	d3
	movb	d1,d3
	addw	d3,d4		| result exp
	cmpb	#0xff, d1
	beqs	ofl		| infinity or nan
	tstb	d0
	beqs	ufl		| 0 or gu (denormalized)
	| clear exponent; set hidden bit
	movb	#1,d0
	rorl	#1,d0
back:	movb	#1,d1
	rorl	#1,d1

	movl	d0,d2		| d2 gets x.
	movl	d1,d3		| d3 gets y.
	swap	d0		| d0 gets xu.
	swap	d1		| d1 gets yu.
	movw	d1,d5		| d5 gets yu.
	mulu	d2,d1		| d1 gets xl*yu.
	mulu	d3,d2		| d2 gets xl*yl.
	mulu	d0,d3		| d3 gets xu*yl.
	mulu	d5,d0		| d0 gets xu*yu.
	addl	d3,d1		| d1 gets middle = xu*yl+xl*yu.
	bccs	1$		| Branch if no carry occurred.
	addl	#0x10000,d0	| Propagate carry to upper product.
1$:
	swap	d1		| d1 gets middle(m) (ml,mu).
	clrl	d3		
	movw	d1,d3		| d3 gets (0,mu).
	clrw	d1		| d1 gets (ml,0).
	addl	d2,d1		| d1 gets lower part of final product.
	addxl	d3,d0		| d0 gets upper part of final product.
	tstl	d1
	beqs	2$		| Branch if lower part is exact.
	bset	#0,d0		| Set sticky bit if lower part has bits.
2$: 		
	subw	#126,d4		| toss extra bias
	JBSR(f_rcp,a2)		| round check, pack

	| build answer
mbuild:	rorl	#8,d0
	roxll	#1,d5
	roxrl	#1,d0		| append sign

	| answer in d0
mexit:	moveml	sp@+,d2-d5/a2
	RET

	| EXCEPTION HANDLING
ofl:	clrb	d1
	tstl	d1		| larger mantissa
	bnes	mni		| user larger nan
	tstl	d0
	beqs	m_gennan		| 0*inf
mni:	movb	#0xff,d1	| inf or nan
	movl	d1,d0
	bras	mbuild
ufl:	tstl	d0		| mantissa of smaller
	beqs	mbuild
	| normalizing mode is embodied int the next few lines:
	bmis	back
normden:subql	#1,d4		| adj exponent
	lsll	#1,d0
	bpls	normden
	bras	back

m_gennan:movl	#0x7f800002, d0
	bras	mexit

/*
 *	ieee single floating divide
 *	copyright 1981, Richard E. James III
 *	translated to SUN idiom 14 March 1983 rt
 */

/*
 *	entry conditions:
 *	    first argument in d0
 *	    second argument in d1
 *	exit conditions:
 *	    result (4 bytes) in d0
 *
 *	register conventions:
 *	    d0		top; ab;  rq
 *	    d1		bot; c
 *	    d2		     q
 *	    d3		bottom exp; d
 *	    d4		top/ final exp
 *	    d5		sign 
 */
|
|	same as for multiply, above
|	NSAVED   = 5*4		| 5 registers * sizeof(register)

RTENTRY(Fdivs)
|	save registers
	moveml	d2-d5/a2,sp@-	| registers 
	| determine sign
	movl	d0,d5
	eorl	d1,d5		| sign in bit 31
	| split out exponent
	roll	#1,d0
	roll	#1,d1
	roll	#8,d0
	roll	#8,d1
	clrw	d3
	clrw	d4
	movb	d0,d4		| exp of top
	movb	d1,d3		| exp of bottom
	andw	#0xfe00,d0	| clear out s, exp
	andw	#0xfe00,d1	| clear out s, exp
	| test exponents
	addqb	#1,d4		| top
	subqw	#1,d4
	bles	toperr
	addqb	#1,d0		| hidden bit
backtop:addqb	#1,d3
	subqw	#1,d3		| bottom
	jle	boterr
	addqb	#1,d1		| hidden bit
	| position mantissas
backbot:rorl	#2,d1		| 01X... 	(c*w+d) w = 2**16
	rorl	#4,d0		| 0001X... 	(a*w+b)
	| compute tentative exponent
	subw	d3,d4
	movew	d4,d5		| d5 saves sign | exponent.
	| to compute ab/cd:
	|    first do ab/c -> q, remainder -> r
	movw	d1,d3		| save d
	movl	d1,d4		| d4 saves 	(c*w+d).
	swap	d1		| get c		(d*w+c)
	divu	d1,d0		| ab/c 29/15->15 bits (q*w+r)
	movw	d0,d2		| save q
	mulu	d2,d3		| q*d
	clrw	d0		| r in top
	subl	d3,d0		| r-q*d = +-31 * w
	asrl	#2,d0		| avoid overflow
	divs	d1,d0		| more quotient
	movl	d0,d1		| d1 gets remainder in upper word.
	movw	d4,d3		| d3 gets d.
	muls	d0,d3		| d3 gets signed(q2)*unsigned(d).
	btst	#15,d4
	beqs	99$		| Branch if d appeared positive in muls.
	swap	d3		| If d appeared negative, correct product.
	addw	d0,d3
	swap	d3
99$:				|
	extl	d2		| q
	extl	d0		| second quot
	swap	d2
	asll	#2,d0
	addl	d2,d0
	asll	#1,d0
	clrw	d1		| d1 gets remainder in upper, 0 lower.
	subl	d3,d1		| d1 gets revised remainder.
	cmpl	d4,d1
	blts	1f		| Branch if remainder < 8 * (c*w+d).
	addql	#8,d0		| Increment quotient.
	subl	d4,d1		| Decrement remainder.
1:
	tstl	d1
	beqs	adjexp		| Branch if exact.
	bmis	decr		| Branch if remainder negative.
	bset	#0,d0		| inexact => set sticky.
	bras	adjexp
decr:
	subql	#1,d0		| Subtract sticky bit for negative remainder.

adjexp: 			| adjust exponent, round, check extremes, pack
	addw	#127,d5
	movew	d5,d4
	JBSR(f_rcp, a2)
	| reposition and append sign
drepk:	rorl	#8,d0
	lsll	#1,d5		| sign -> x
	roxrl	#1,d0		| insert sign
dexit:	moveml	sp@+,d2-d5/a2
	RET

	| EXCEPTIONS
toperr:	bnes	2$
	|top is 0 or gu, normalize and return
1$:	subqw	#1,d4
	roll	#1,d0
	bhis	1$		| loop til normalized, fall if 0
	addqw	#1,d4
	jra	backtop		| 0 or gu

	| top is inf or nan
2$:	cmpb	d3,d4
	beqs	dinvop		| both inf/nan -> nan
	tstl	d0
	beqs	geninf		| inf/ ... = +- inf
	bras	dinvop		| nan/...  =    nan

boterr:	beqs	botlow		| .../(0|gu)
				| .../(inf|nan)
	tstl	d1
	bnes	dinvop		| .../nan = nan
	clrl	d0		| .../inf = +-0
	bras	drepk
botlow:	tstl	d1
	beqs	5$		| .../0
				| .../gu:
4$:	subqw	#1,d3
	roll	#1,d1
	bccs	4$		| loop til normalized
	addqw	#1,d3
	jra 	backbot

	| bottom is zero
5$:	tstl	d0
	beqs	d_gennan		| 0/0       =   nan
				| nonzero/0 = +-inf
	| generate infinity for answer
geninf:	movl	#0xff,d0
	bras	drepk

	| invalid operand/ operation
dinvop:	cmpl	d0,d1
	bcss	8$		| use larger nan
	tstl	d1
	beqs	d_gennan	| both are infinity, generate a nan
	exg	d1,d0		| larger nan
8$:	lsrl	#8,d0
	lsrl	#1,d0
	bras	bldnan		| return nan

d_gennan:moveq	#4,d0		| nan 4
bldnan:	orl	#0x7f800000,d0
	bras	dexit

	EXP	= d2
	TYPE	= d3
	/* type values: */
	    ZERO  = 1 | wonderful
	    GU    = 2
	    PLAIN = 3
	    INF   = 4
	    NAN   = 5

RTENTRY(Fscaleis)
	moveml	d2-d7/a2,sp@-	| state save
	movel	d1,d4		| Save argument i.
	movel	d0,d1		| What a crock!
	JBSR(f_unpk, a2)
	cmpb	#PLAIN,TYPE	| is it a funny number?
	bgts	gohome		| yes -- return argument
	cmpb	#ZERO,TYPE
	beqs	gohome		| is zero -- return arg
	| normal path through here
	movw	EXP,d7		
	extl	d7		| d7 gets long exponent.
	addl	d4,d7		| d7 gets modified exponent.
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
	JBSR(f_pack,a2)
	movel	d1,d0		| What a crock!
gone:	moveml	sp@+,d2-d7/a2
	RET
