        .data
|        .asciz  "@(#)Faintd.s 1.1 92/07/30 SMI"
        .even
        .text

|       Copyright (c) 1987 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"
#include "PIC.h"

RTENTRY(Farintd)
	moveml	d2-d4,sp@-
	movel	d0,d2		| d2 gets top(x).
	bclr	#31,d2		| d2 gets top(abs(x)).
	cmpl	#0x3ff00000,d2
	bges	2f		| Branch if abs(x) >= 1.0.
	andl	#0x80000000,d0	| Isolate sign of x.
	cmpl	#0x3fe00000,d2
	blts	6f		| Branch if abs(x) < 0.5.
	bgts	8f		| Branch if abs(x) definitely > 0.5.
	tstl	d1
	beqs	7f		| Branch if abs(x) = 0.5 exactly.
8:
	orl	#0x3ff00000,d0	| Make signed 1.0.
6:
	clrl	d1		| Clear lower part in any case.
7:
	jra	Farintddone
2:
	cmpl	#0x43300000,d2
	blts	3f		| Branch if 1 <= |x| < 2**52.
Farintdbig:
	cmpl	#0x7ff00000,d2
	blts	Farintddone	| Branch if x is finite.
	bgts	1f		| Branch if x is nan.
	tstl	d1
	beqs	Farintddone	| Branch if x is infinity.
1:
	bset	#19,d0		| Convert quiet to signalling.
	bras	Farintddone
3:
	roll	#8,d2
	roll	#4,d2
	andw	#0xfff,d2	| d2 gets biased exponent.
	cmpw	#0x3ff+20,d2
	bges	4f		| Branch if exp >= 20.
	subw	#0x3ff+19,d2	| d2 gets unbiased exponent - 19.
	negw	d2		| d2 gets 19-exp = bit number.
	clrl	d3
	bset	d2,d3		| Set round bit.
	addl	d3,d0		| Add round bit.
	lsll	#1,d3		| Make 2**0 bit.
	subql	#1,d3		| Fraction bit mask.
	tstl	d1
	bnes	2f		| Branch if not ambiguous case.
	movel	d0,d4
	andl	d3,d4		| d4 gets rounded fraction field.
	bnes	2f		| Branch if not ambiguous case.
	addqw	#1,d2		| Make bit number for 2**0.
	bclr	d2,d0		| Force round to even.
2:
	notl	d3
	andl	d3,d0		| Clear fraction field.
1:
	clrl	d1		| Lower half is all fraction bits.
	bras	Farintddone
4:
	bnes	7f		| Branch if exponent > 20.
	addl	#0x80000000,d1  | Add round bit.
	bccs	8f		| Branch if no carry out.
	addql	#1,d0		| Propagate carry.
8:
	tstl	d1
	bnes	1b		| Branch if not ambiguous.
	bclr	#0,d0		| Force round to even.
	bras	Farintddone
7:
	subw	#0x3ff+51,d2	| d2 gets exp-51.
	negw	d2		| d2 gets bit number 51-exp.
	clrl	d3
	bset	d2,d3		| Set round bit.
	addl	d3,d1		| Add round.
	bccs	9f
	addql	#1,d0		| Propagate carry.
9:
	movel	d3,d4
	subql	#1,d4
	orl	d4,d3		| d3 gets fraction bit mask.
	movel	d3,d4
	andl	d1,d4
	bnes	1f		| Branch if not ambiguous.
	addqw	#1,d2
	bclr	d2,d1		| Force round to even.
1:
	notl	d3
	andl	d3,d1		| Clear fraction bits.
Farintddone:
	moveml	sp@+,d2-d4
	RET

RTENTRY(Fanintd)
	moveml	d2-d4,sp@-
	movel	d0,d2		| d2 gets top(x).
	bclr	#31,d2		| d2 gets top(abs(x)).
	cmpl	#0x3ff00000,d2
	bges	2f		| Branch if abs(x) >= 1.0.
	andl	#0x80000000,d0	| Isolate sign of x.
	cmpl	#0x3fe00000,d2
	blts	6f		| Branch if abs(x) < 0.5.
	orl	#0x3ff00000,d0	| Make signed 1.0.
6:
	clrl	d1		| Clear lower part in any case.
	jra	Farintddone
2:
	cmpl	#0x43300000,d2
	jge	Farintdbig	| Branch if not 1 <= |x| < 2**52.
	roll	#8,d2
	roll	#4,d2
	andw	#0xfff,d2	| d2 gets biased exponent.
	cmpw	#0x3ff+20,d2
	bges	4f		| Branch if exp >= 20.
	subw	#0x3ff+19,d2	| d2 gets unbiased exponent - 19.
	negw	d2		| d2 gets 19-exp = bit number.
	clrl	d3
	bset	d2,d3		| Set round bit.
	addl	d3,d0		| Add round bit.
	lsll	#1,d3		| Make 2**0 bit.
	subql	#1,d3		| Fraction bit mask.
	notl	d3
	andl	d3,d0		| Clear fraction field.
1:
	clrl	d1		| Lower half is all fraction bits.
	bras	Farintddone
4:
	bnes	7f		| Branch if exponent > 20.
	addl	#0x80000000,d1  | Add round bit.
	bccs	1b		| Branch if no carry out.
	addql	#1,d0		| Propagate carry.
	bras	1b
7:
	subw	#0x3ff+51,d2	| d2 gets exp-51.
	negw	d2		| d2 gets bit number 51-exp.
	clrl	d3
	bset	d2,d3		| Set round bit.
	addl	d3,d1		| Add round.
	bccs	9f
	addql	#1,d0		| Propagate carry.
9:
	lsll	#1,d3
	subql	#1,d3
	notl	d3
	andl	d3,d1		| Clear fraction bits.
	jra	Farintddone

RTENTRY(Faintd)
	moveml	d2-d4,sp@-
	movel	d0,d2		| d2 gets top(x).
	bclr	#31,d2		| d2 gets top(abs(x)).
	cmpl	#0x3ff00000,d2
	bges	2f		| Branch if abs(x) >= 1.0.
	andl	#0x80000000,d0	| Isolate sign of x.
	clrl	d1		| Clear lower part in any case.
	jra	Farintddone
2:
	cmpl	#0x43300000,d2
	jge	Farintdbig	| Branch if not 1 <= |x| < 2**52.
	roll	#8,d2
	roll	#4,d2
	andw	#0xfff,d2	| d2 gets biased exponent.
	cmpw	#0x3ff+20,d2
	bges	4f		| Branch if exp >= 20.
	subw	#0x3ff+20,d2	| d2 gets unbiased exponent - 20.
	negw	d2		| d2 gets 20-exp = bit number.
	clrl	d3
	bset	d2,d3		| Set round bit.
	subql	#1,d3		| Fraction bit mask.
	notl	d3
	andl	d3,d0		| Clear fraction field.
1:
	clrl	d1		| Lower half is all fraction bits.
	jra	Farintddone
4:
	beqs	1b		| Branch if exponent not > 20.
7:
	subw	#0x3ff+52,d2	| d2 gets exp-52.
	negw	d2		| d2 gets bit number 52-exp.
	clrl	d3
	bset	d2,d3		| Set round bit.
	subql	#1,d3
	notl	d3
	andl	d3,d1		| Clear fraction bits.
	jra	Farintddone

RTENTRY(Ffloord)
	moveml	d0-d3/a2,sp@-	| Save x and d2/d3.
	JBSR(Faintd,a2)		| d0 gets aint(x).
	moveml	sp@+,d2-d3	| d2/d3 gets x.
	tstl	d2
	bpls	Ffloorddone	| Branch if x is positive.
	cmpl	d0,d2
	bnes	1f
	cmpl	d1,d3
	beqs	Ffloorddone	| Return x if x = aint(x).
1:
	lea	one,a0
	JBSR(Vsubd,a2)		| d0 gets aint(x)-1.
Ffloorddone:
	moveml	sp@+,d2-d3/a2
	RET

one:	.double 0r1.0

RTENTRY(Fceild)
	moveml	d0-d3/a2,sp@-	| Save x and d2/d3.
	JBSR(Faintd,a2)		| d0 gets aint(x).
	moveml	sp@+,d2-d3	| d2/d3 gets x.
	tstl	d2
	bmis	Fceilddone	| Branch if x is negative.
	cmpl	d0,d2
	bnes	1f
	cmpl	d1,d3
	beqs	Fceilddone	| Return x if x = aint(x).
1:
	lea	one,a0
	JBSR(Vaddd,a2)		| d0 gets aint(x)+1.
Fceilddone:
	moveml	sp@+,d2-d3/a2
	RET
