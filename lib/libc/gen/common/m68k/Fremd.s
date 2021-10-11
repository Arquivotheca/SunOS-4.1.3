        .data
|        .asciz  "@(#)Fremd.s 1.1 92/07/30 SMI"
        .even
        .text

|       Copyright (c) 1987 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"
#include "PIC.h"

RTENTRY(Fmodd)
	moveml	d0/d1/d2/a0/a2,sp@-	| Save arguments and scratch.
	bsr	Fremd
	movel	sp@,d2			| d2 gets sign of x.
	eorl	d0,d2			| d2 gets sign of x eor sign of remainder r.
	bpls	ok			| Branch if same signs.
	movel	sp@(12),a0		| a0 gets address of y.
	movel	a0@,d2			| d2 gets sign of y.
	eorl	d0,d2			| d2 gets sign of y eor sign of r.
	bpls	sub			| Branch if same signs, implying subtract.
	JBSR(Faddd,a2)			| d0/d1 := r + y.
	bras	ok
sub:
	JBSR(Fsubd,a2)			| d0/d1 := r - y.
ok:
	addql	#8,sp			| Skip old d0/d1.
	moveml	sp@+,d2/a0/a2		| Restore d2/a0/a2.	
	RET

RTENTRY(Fremd)
#ifdef PIC
	movl	a2,sp@-			| Be transparent
	JBSR(Fremd__,a2)		| Call real routine
	movl	sp@+,a2			| Recover register
	RET
	.data
Fremd__:
#endif PIC
	JBSR(Fclass2d,a2)
	.long	drem,RETURNINVALID,drem,RETURNX			| x is normal
	.long	RETURNX,RETURNINVALID,RETURNX,RETURNX		| x is zero
	.long	drem,RETURNINVALID,drem,RETURNX			| x is subnormal
	.long	RETURNINVALID,RETURNINVALID,RETURNINVALID,RETURNINVALID	| x is inf
	.text
drem:
	bclr	#31,d7			| Clear sign of y - irrelevant.
	bsr	d_rem
	exg	d2,d0
	JBSR(setquotient,a2)		| Set remainder quotient bits.
	exg	d2,d0
	bsrs	pack
	addql	#8,sp			| Skip saved d0/d1.
	moveml	sp@+,d2-d7/a1
	addql	#4,sp			| Bypass return for "jsr Fclass2d".
	RET

pack:	
	tstl	d1
	bnes	testsub
	tstl	d0
	bne	testsub
	movl	d3,d0
	andl	#0x80000000,d0
	rts			| True zero.
testsub:
        addw	#0x3ff,d3	| Bias.
	bgts	pnormal
	subqw	#1,d3
denorm:
	lsrl	#1,d0
	roxrl	#1,d1
	addqw	#1,d3
	blts	denorm
pnormal:
	lsll    #1,d1           | Clear I bit.
        roxll   #1,d0
        andw    #0xf000,d1      | Make space for extra bits.
        movw    d0,d4
        andw    #0xfff,d4       | D6 gets 12 extra bits for d1.
        orw     d4,d1
        rorl    #4,d1           | Reposition bits.
        rorl    #8,d1
        andw    #0xf000,d0      | Make space for exponent.
	tstl	d3
	bpls	1$
	addw	#0x800,d3	| Insert sign.
1$:
	orw     d3,d0           | Insert exponent.
        rorl    #4,d0           | Reposition exponent.
        rorl    #8,d0	
	rts
	
|	Routine d_rem provides the core double precision remainder function.
| 	The arguments are X and Y; the computed results R and Q.
| They satisfy R = X - Q * Y and Q = [X/Y] rounded to the nearest integer,
| so R is an IEEE remainder and Q is the corresponding quotient.
| 	This routine only provides the core of the function.
| X must be a normalized positive or negative number, not zero, inf, or nan,
| of no more than 64 significant bits. 
| Y must be a positive normalized number of no more than 64 significant bits.  
|      	R is returned as zero or a normalized number; Q as a 32 bit integer.
| If |Q| >= 2^30 then it will have the proper sign and its 30 low order bits
| will be correct, but bit 30 will be set to 1 to indicate positive overflow,
| 0 to indicate negative overflow.
|	This routine clobbers a0 and d0-d7, so preserve registers appropriately.
| Register usage:
|	d0/d1	contain the significand of X on input and R on output.
|	   d3   contains the sign of X, then R, in bit 31, and the unbiased
|		exponent in bits 0..15.
|		during the loop d3 is a constant zero.
|	d4/d5	contain the significand of Y on input.
|	   d7	contains the exponent of Y on input, and the cycle count
|		in the main loop.
|	   d2	contains the quotient Q on output, bit 30 indicating overflow.
|	   d6	contains the extension of R during the loop.	
|	   a0	saves d3 during the loop.

	
RTENTRY(d_rem)
	clrl	d2	| Q := 0.
	clrw	d6	| Sign extension of R is positive at first.
	negw	d7
	addw	d3,d7	| d7 gets cycle count = X.exp - Y.exp.
	bmis	negcount| Branch if cycle count negative: return X.
	subw	d7,d3	| Real remainder implies R.exp = Y.exp.
	movl	d3,a0	| a0 saves sign of X and exponent of R.
	clrw	d3	| d3 gets constant zero for addxw.
	bra	posrema	| Remainder starts out positive.
negcount:		| R < Y but might be > Y/2.
	cmpw	#-1,d7
	jne	normret	| Branch if R < Y/2.
	cmpl	d0,d4
	bnes	1$
	cmpl	d1,d5
1$:
	jcc	normret | Branch if R < Y/2.
	subw	d7,d3	| Exponent of R will be Y's.
	subqw	#1,d3	| But subtract 1 since Y will be left shifted.
	lsll	#1,d5   | Double Y since we can't halve R without
	roxll	#1,d4   | possibly losing the least sig bit.
	movl	d3,d7	| Sign of Q will be X's.
	bras	roundup

| Following main loop is a nonrestoring divide which accumulates the
| quotient Q in d2.
| The invariant assertion at the end of the loop is -Y <= R < Y.

toploop:
	lsll	#1,d1	| R := 2*R.
	roxll	#1,d0
	roxlw	#1,d6
	bpls	posrem
			| R < 0 so R := 2*R + Y.
	addl	d5,d1
	addxl	d4,d0
	addxw	d3,d6
	bras	botloop
posrem:
	addql	#1,d2	| Positive R means 1 bit in Q.
posrema:
	subl	d5,d1	| R >= 0 so R := 2*R - Y.
	subxl	d4,d0
	subxw	d3,d6
botloop:
	lsll	#1,d2	| Q := 2*Q.
	bccs	1$	| Branch if no overflow.
	cmpw	#32,d7
	blts	4$	| Branch if count has less than	32 more cycles to go.
	bset	#0,d2	| Turn on least significant bit to cycle out later.
	bras	1$
4$:	bset	#30,d2	| Turn on significant bit.
1$:
	dbf	d7,toploop | At this point -Y <= R < +Y.

	movl	a0,d3	| Restore sign of X and exponent of R.
	movl	d3,d7	| d7 gets sign of Q = sign of X.
	tstw	d6
	bpls	rpos	| Branch if R is positive.
	addl	d5,d1
	addxl	d4,d0	| R := R + Y so 0 <= R < Y.
	bras	check0:
rpos:
	addql	#1,d2	| Final bit of Q.
check0:
	tstl	d1	| At this point 0 <= R < Y.
	bnes	check	| Testing for R = 0.
	tstl	d0
	bnes	check
	tstw	d6
	beqs	fixq	| Branch on exact zero result.

check:			| At this point 0 < R < Y.
			| Remainder is not exact zero so check rounding of Q.
			| If 2*R < Y then Q is OK.
			| If 2*R = Y then round Q to even; R = +- Y/2.
			| If 2*R > Y then round Q up; R := R - Y.
	
	tstl	d0
	bmis	roundup	| R >= 0.5 so 2*R > Y for sure.
	lsll	#1,d1
	roxll	#1,d0	| Double R.
	cmpl	d0,d4
	movw	cc,d6	| d6 gets compare result.
	bnes	1$
	cmpl	d1,d5
	movw	cc,d6
1$:
	lsrl	#1,d0
	roxrl	#1,d1	| Undo the doubling of R.
	movw	d6,cc	| Restore condition codes.
	bhis	fixq	| Y > 2R so Q is ok.
	bcss	roundup	| Y < 2R so Q increment Q.
	btst	#0,d2	| Y = 2R so check Q for even.
	beqs	fixq	| Branch if Q is already even.
roundup:		| Increment Q, decrement R.
	addql	#1,d2	| Increment Q.
	bccs	2$ 
	bset	#31,d2	| Set overflow indicator.
2$:
	subl	d5,d1	| Y/2 <= R < Y so let R := R - Y.
	subxl	d4,d0	| Thus -Y/2 <= R < 0.
	negl	d1
	negxl	d0	| Negate so 0 < -R <= Y/2.
	bchg	#31,d3  | Reverse sign of remainder.
	
fixq:			| At this point -Y/2 <= R <= Y/2.
	cmpl	#0x3fffffff,d2
	bls	qsign   | Branch if Q <= 2**30 -1.
	bset	#30,d2	| Set overflow bit.
	bclr	#31,d2	| Clear sign bit.
qsign:
	tstl	d7
	bpls	1$	| Branch if quotient is positive.
	negl	d2	| Negate negative quotient.
1$:			| Normalize remainder.
	tstl 	d0
	bnes	normtest
	subw	#32,d3
	movl	d1,d0	| 32 bit normalize.
	beqs	normret	| But exact zero needn't be normalized.
	clrl	d1
normtest:
	tstl	d0
	bmis	normret | Branch if normalized.
normloop:
	subqw	#1,d3
	lsll	#1,d1
	roxll	#1,d0
	bpls	normloop | Branch if not normalized.
normret:
	RET
