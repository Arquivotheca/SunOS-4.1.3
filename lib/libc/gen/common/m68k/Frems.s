        .data
|        .asciz  "@(#)Frems.s 1.1 92/07/30 SMI"
        .even
        .text

|       Copyright (c) 1987 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"
#include "PIC.h"

RTENTRY(Fmods)
	moveml	d0/d1/a2,sp@-		| Save arguments and scratch.
	JBSR(Frems,a2)
	movel	sp@+,d1			| d1 gets x.
	eorl	d0,d1			| d1 gets sign of x eor sign of remainder r.
	bpls	okskip			| Branch if same signs.
	movel	sp@,d1			| d1 gets sign of y.
	eorl	d0,d1			| d1 gets sign of y eor sign of r.
	bpls	sub			| Branch if same signs, implying subtract.
	movel	sp@+,d1
	JBSR(Fadds,a2)			| d0/d1 := r + y.
	bras	ok
sub:
	movel	sp@+,d1
	JBSR(Fsubs,a2)			| d0/d1 := r - y.
	bras	ok
okskip:
	addql	#4,sp			| Skip old d1.
ok:
	movl	sp@+,a2			| Transparent to a2
	RET

RTENTRY(Frems)
#ifdef PIC
	movl	a2,sp@-			| Be transparent
	JBSR(Frems__,a2)
	movl	sp@+,a2
	RET
	.data
Frems__:
#endif
	JBSR(Fclass2s,a2)
	.long	srem,RETURNINVALID,srem,RETURNX			| x is normal
	.long	RETURNX,RETURNINVALID,RETURNX,RETURNX		| x is zero
	.long	srem,RETURNINVALID,srem,RETURNX			| x is subnormal
	.long	RETURNINVALID,RETURNINVALID,RETURNINVALID,RETURNINVALID	| x is inf
	.text
srem:
	bclr	#31,d7			| Clear sign of y - irrelevant.
	bsr	s_rem
	exg	d2,d0
	JBSR(setquotient,a2)		| Set remainder quotient bits.
	exg	d2,d0
	bsrs	s_pack
	addql	#8,sp			| Skip saved d0/d1.
	moveml	sp@+,d2-d7/a1
	addql	#4,sp			| Bypass return for "jsr Fclass2d".
	RET

s_pack:                 | Pack d0/d3 into d0.
        tstl	d0
	beqs	s_sign		| Branch if zero.
        addw    #127,d3         | Add in bias.
	bgts	s_normal	| Branch if normalized.
	negw	d3		| d3 gets -127-exp = shift count - 1.
	addqw	#1,d3
	lsrl	d3,d0		| Shift right to subnormalize.
	clrw	d3		| Set up exponent.
	
s_normal:
	roll    #1,d0           | Move implicit bit to bit 0.
        movb    d3,d0           | Insert exponent in d0.
        rorl    #8,d0           | Rotate exponent.
        
s_sign:
	roxll   #1,d3           | X bit gets sign.
        roxrl   #1,d0           | Insert sign.
        rts

|	Routine s_rem provides the core double precision remainder function.
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
|	d0	contain the significand of X on input and R on output.
|	   d3   contains the sign of X, then R, in bit 31, and the unbiased
|		exponent in bits 0..15.
|		during the loop d3 is a constant zero.
|	d4	contain the significand of Y on input.
|	   d7	contains the exponent of Y on input, and the cycle count
|		in the main loop.
|	   d2	contains the quotient Q on output, bit 30 indicating overflow.
|	   d6	contains the extension of R during the loop.	
|	   a0	saves d3 during the loop.

	
RTENTRY(s_rem)
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
	jcc	normret | Branch if R < Y/2.
	subw	d7,d3	| Exponent of R will be Y's.
	subqw	#1,d3	| But subtract 1 since Y will be left shifted.
	lsll	#1,d4   | Double Y since we can't halve R without
	movl	d3,d7	| Sign of Q will be X's.
	bras	roundup

| Following main loop is a nonrestoring divide which accumulates the
| quotient Q in d2.
| The invariant assertion at the end of the loop is -Y <= R < Y.

toploop:
	lsll	#1,d0	| R := 2*R.
	roxlw	#1,d6
	bpls	posrem
			| R < 0 so R := 2*R + Y.
	addl	d4,d0
	addxw	d3,d6
	bras	botloop
posrem:
	addql	#1,d2	| Positive R means 1 bit in Q.
posrema:
	subl	d4,d0
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
	addl	d4,d0	| R := R + Y so 0 <= R < Y.
	bras	check0:
rpos:
	addql	#1,d2	| Final bit of Q.
check0:
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
	lsll	#1,d0
	cmpl	d0,d4
	movw	cc,d6	| d6 gets compare result.
	lsrl	#1,d0
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
	subl	d4,d0	| Thus -Y/2 <= R < 0.
	negl	d0	| Negate so 0 < -R <= Y/2.
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
	beqs	normret	| But exact zero needn't be normalized.
	bmis	normret | Branch if normalized.
normloop:
	subqw	#1,d3
	lsll	#1,d0
	bpls	normloop | Branch if not normalized.
normret:
	RET
