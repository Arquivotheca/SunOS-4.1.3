        .data
|        .asciz  "@(#)Faints.s 1.1 92/07/30 Copyr 1985 Sun Micro"
        .even
        .text

|       Copyright (c) 1985 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"
#include "PIC.h"

/*
 	Faints: single precision round integral toward zero.

*/

RTENTRY(Farints)
	movel	d0,d1		| d1 gets (x).
	bclr	#31,d1		| d1 gets (abs(x)).
	cmpl	#0x3f800000,d1
	bges	2f		| Branch if abs(x) >= 1.0.
	andl	#0x80000000,d0	| Isolate sign of x.
	cmpl	#0x3f000000,d1
	bles	6f		| Branch if abs(x) =< 0.5.
	orl	#0x3f800000,d0	| Make signed 1.0.
6:
	jra	Farintsdone
2:
	cmpl	#0x4b000000,d1
	blts	3f		| Branch if 1 <= |x| < 2**23.
Farintsbig:
	cmpl	#0x7f800000,d1
	bles	Farintsdone	| Branch if x is finite or infinity.
	bset	#22,d0		| Convert quiet to signalling.
	bras	Farintsdone
3:
	roll	#8,d1
	roll	#1,d1
	andw	#0xff,d1	| d1 gets biased exponent.
	subw	#0x7f+22,d1	| d1 gets unbiased exponent - 22.
	negw	d1		| d1 gets 22-exp = bit number.
	movel	d2,a0
	clrl	d2
	bset	d1,d2		| Set round bit.
	addl	d2,d0		| Add round bit.
	lsll	#1,d2		| Make 2**0 bit.
	subql	#1,d2		| Fraction bit mask.
	andl	d0,d2		| d1 gets rounded fraction field.
	bnes	2f		| Branch if not ambiguous case.
	addqw	#1,d1		| Make bit number for 2**0.
	bclr	d1,d0		| Force round to even.
2:
	eorl	d2,d0 		| Clear fraction field.
Farintsrestored2:
	movel	a0,d2
Farintsdone:
	RET

RTENTRY(Fanints)
	movel	d0,d1		| d1 gets (x).
	bclr	#31,d1		| d1 gets (abs(x)).
	cmpl	#0x3f800000,d1
	bges	2f		| Branch if abs(x) >= 1.0.
	andl	#0x80000000,d0	| Isolate sign of x.
	cmpl	#0x3f000000,d1
	blts	6f		| Branch if abs(x) < 0.5.
	orl	#0x3f800000,d0	| Make signed 1.0.
6:
	jra	Farintsdone
2:
	cmpl	#0x4b000000,d1
	bges	Farintsbig	| Branch if 1 <= |x| < 2**23.
	roll	#8,d1
	roll	#1,d1
	andw	#0xff,d1	| d1 gets biased exponent.
	subw	#0x7f+22,d1	| d1 gets unbiased exponent - 22.
	negw	d1		| d1 gets 22-exp = bit number.
	movel	d2,a0
	clrl	d2
	bset	d1,d2		| Set round bit.
	addl	d2,d0		| Add round bit.
	lsll	#1,d2		| Make 2**0 bit.
	subql	#1,d2		| Fraction bit mask.
	notl	d2
	andl	d2,d0
	bras	Farintsrestored2

RTENTRY(Faints)
	movel	d0,d1		| d1 gets (x).
	bclr	#31,d1		| d1 gets (abs(x)).
	cmpl	#0x3f800000,d1
	bges	2f		| Branch if abs(x) >= 1.0.
	andl	#0x80000000,d0	| Isolate sign of x.
	jra	Farintsdone
2:
	cmpl	#0x4b000000,d1
	jge	Farintsbig	| Branch if 1 <= |x| < 2**23.
	roll	#8,d1
	roll	#1,d1
	andw	#0xff,d1	| d1 gets biased exponent.
	subw	#0x7f+23,d1	| d1 gets unbiased exponent - 23.
	negw	d1		| d1 gets 23-exp = bit number.
	movel	d2,a0
	clrl	d2
	bset	d1,d2		| Set 2**0 bit.
	subql	#1,d2		| Fraction bit mask.
	notl	d2
	andl	d2,d0
	jra	Farintsrestored2

RTENTRY(Ffloors)
	movel	d0,sp@-		| Save x.
	JBSR(Faints,a1)		| d0 gets aint(x).
	movel	sp@+,d1		| d1 gets x.
	bpls	Ffloorsdone	| Branch if x is positive.
	cmpl	d0,d1
	beqs	Ffloorsdone	| Return x if x = aint(x).
	movel	#0xbf800000,d1	| d1 gets -1.
	JBSR(Fadds,a1)		| d0 gets aint(x)-1.
Ffloorsdone:
	RET

RTENTRY(Fceils)
	movel	d0,sp@-		| Save x.
	JBSR(Faints, a1)		| d0 gets aint(x).
	movel	sp@+,d1		| d1 gets x.
	bmis	Fceilsdone	| Branch if x is negative.
	cmpl	d0,d1
	beqs	Fceilsdone	| Return x if x = aint(x).
	movel	#0x3f800000,d1	| d1 gets +1.
	JBSR(Fadds, a1)		| d0 gets aint(x)+1.
Fceilsdone:
	RET
