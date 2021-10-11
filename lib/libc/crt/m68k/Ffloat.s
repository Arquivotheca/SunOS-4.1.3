	.data
|	.asciz	"@(#)Ffloat.s 1.1 92/07/30 SMI"
	.even
	.text

|	Copyright (c) 1987 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"

/*
 *	single-precision utility operations
 *
 *	copyright 1981, 1982 Richard E. James III
 *	translated to SUN idiom 11 March 1983 rt
 */

	NSAVED	 = 3*4	| save registers d2-d4
	SAVEMASK = 0x3c00
	RESTMASK = 0x003c

/*
 * unpack a single-precision number into the unpacked record format:
 *	d0/d1	mantissa
 *	d2.w	exponent
 *	d3.w	sign in upper bit
 *	d3.b	type ( zero, gu, plain, inf, nan )
 */

ENTER(f_unpk)
	movl	d1,d3
	swap	d3	| sign in bit 15
	lsll	#1,d1	| toss sign
	roll	#8,d1
	clrw	d2
	movb	d1,d2	| exponent
	bnes	1$	| not gu or zero
	movb	#1,d3
	tstl	d1
	beqs	3$
	movb	#2,d3	| gu -- gradual underflow unnormalized
	bras	3$
1$:	cmpb	#255,d2	| inf or nan
	bnes	2$	| nope, plain number
	movw	#0x6000,d2
	clrb	d1	| erase exponent
	movb	#4,d3	| infinity
	tstl	d1
	beqs	4$
	movb	#5,d3	| nan
	bras	4$
2$:	movb	#1,d1	| hidden bit
	subqw	#1,d2
	movb	#3,d3
3$:	subw	#(126+23),d2
4$:	rorl	#1,d1
	lsrl	#8,d1
	clrl	d0
	rts

/*
 * reconstruct a single precision number from its pieces
 * returns packed value in d0
 */

ENTER(f_pack)
	movw	d2,d4	| exponent
	cmpb	#4,d3	| is type inf or nan ?
	blts	1$	| no, then branch
	orl	d0,d1
	orl	#0x7f800000,d1	| exponent for inf/nan
	lsll	#1,d1
	bras	6$
1$:
	clrb	d2	| for sticky
	tstl	d0
	beqs	3$
	| shift from upper into lower
2$:
	orb	d1,d2	| sticky
	movb	d0,d1
	addqw	#8,d4	| adjust exponent
	rorl	#8,d1
	lsrl	#8,d0
	bnes	2$	| loop until top == 0
3$:	movl	d1,d0
	beqs	6$
	| find top bit
	bmis	5$
4$:	subqw	#1,d4	| adjust exponent
	lsll	#1,d0	| normalize
	bpls	4$
5$:	addw	#(126+23+9),d4
	tstb	d2
	beqs	7$	| Branch if no sticky.
	bset	#0,d0	| Turn on sticky bit.
7$:
	bsrs	f_rcp
	rorl	#8,d0
	movl	d0,d1
6$:	lslw	#1,d3
	roxrl	#1,d1	| append sign
	rts


/*
 * round, check for over/underflow, and pack in the exponent.
 */

ENTER(f_rcp)
	tstl	d0
	bmis	f_rcfast
	| do extra normalize ( for mul/div)
	subqw	#1,d4
	lsll	#1,d0	| do one normalize
f_rcfast:	
	tstw	d4
	bgts	2$
	| underflow
	cmpw	#-24,d4
	blts	rcpzero
	negb	d4
	addqb	#1,d4
	movl	d1,sp@-		| Save d1 on stack.
	clrl	d1		| d1 gets 0.
	bset	d4,d1		| For n bit shift, d1 gets 2**n.
	subql	#1,d1		| d1 gets 2**n -1, an n bit field.
	andl	d0,d1		| d1 gets bits to be shifted away.
	beqs	1f		| Branch if all zero.
	bset	d4,d0		| Sticky lsb for bits to be lost.
1:
	movl	sp@+,d1		| Restore d1.
	lsrl	d4,d0	| denormalize
	clrw	d4	| exp == 0
2$:	addl	#0x80,d0	| crude round
	bccs	stesteven
	| round overflowed
	roxrl	#1,d0
	addqw	#1,d4
	bras	scheckbig
stesteven:
	tstb	d0	| Check extra bits after round.
	bnes	scheckbig | Branch if round was not ambiguous.
	bclr 	#8,d0	| Force round to even.
scheckbig:
	cmpw	#0xff,d4	| adjust exponent
	bges	rcpinf
	lsll	#1,d0	| toss hidden
	|scs	d0	| no hidden implies zero or denormalized
	|andb	d4,d0
	|rts
	bccs	rcpsubnorm | Branch if no i bit found, implying zero or subnorm.
	movb	d4,d0	| d0 gets exponent.
	bnes	rcprts	| Branch if exp was not zero, implying normal result.
	movb	#1,d0	| Result was subnormal before round, normal after,
			| so adjust exponent accordingly.
rcprts:
	rts
rcpsubnorm:		| No carry out from shift implies zero or subnormal
			| result after rounding.
	clrb	d0	| Set minimum exponent.
	rts

rcpzero:	
	clrl	d0
	rts
rcpinf:	
	movl	#0xff,d0	| infinity
	rts

	NSAVED	 = 3*4	| save registers d2-d4
	SAVEMASK = 0x3c00
	RESTMASK = 0x003c
	ARG2PTR	 = a0

/*
 * split a double floating number into its pieces
 * input:
 *	d0/d1	double number
 * output: (format of an unpacked record)
 *	d0/d1	mantissa: array[1..2] of integer;
 *	d2.w	exponent:	-32768..32767;
 *	d3.w	sign: (bit 15)  0..1;
 *	d3.b	type: 1..5; (zero, gu, plain, inf, nan )
 */

ENTER(d_unpk)
	movl	#0xfff00000,d2	| mask for sign and exponent
	movl	d0,d3
	swap	d3		| sign
	andl	d0,d2		| extract exponent
	eorl	d2,d0		| top of mantissa cleared out
	movl	d1,d4
	orl	d0,d4		| non-zero iff mantissa non-zero
	lsll	#1,d2		| toss sign
	bnes	1$		| not 0 or gu
	movb	#1,d3	
	tstl	d4
	beqs	3$		| zero
	movb	#2,d3
	bras	3$		| gu
1$:	swap	d2
	lsrw	#(16-11),d2	| exp to bottom of register
	cmpw	#0x7ff,d2	| inf or nan
	bnes	2$		| plain
	movw	#0x6000,d2
	movb	#4,d3
	tstl	d4
	beqs	4$		| inf
	movb	#5,d3		| nan
	bras	4$
2$:	bset	#20,d0		| hidden bit
	subqw	#1,d2
	movb	#3,d3
3$:	subw	#(1022+52), d2
4$:	rts

/*
 * reconstruct a double precision number from a record containing its pieces.
 *
 * input:
 *	d2	upper
 *	d3	lower 
 *	d6	exponent
 * output:
 *	d0/d1	result
 */

ENTER(d_pack)
	cmpb	#4,d3	| type
	blts	1$
	orl	d1,d0
	lsll	#1,d0
	orl	#0xffe00000,d0
	bras	2$	| nan or inf
1$:	addw	#(1022+52+12),d2
	exg	d0,d2
	exg	d0,d6
	exg	d1,d3
	bsrs	d_norm
	bsr	d_rcp
	movl	d0,d6
	movl	d2,d0
	exg	d3,d1
2$:	lslw	#1,d3
	roxrl	#1,d0
	rts

/*
 * normalize a double-precision number
 *
 * input:
 *	d2/d3 mantissa
 *	d6    exponent
 */

ENTER(d_norm)
	tstl	d2	| upper is non-zero
	bnes	1$
	cmpw	#32,d6
	blts	2$	| about to be denormalized
	subw	#32,d6
	exg	d3,d2	| shift 32
	tstl	d2
	beqs	4$	| if result == 0
1$:	bmis	3$	| if already normalized
2$:	lsll	#1,d3	| normalize
	roxll	#1,d2
	dbmi	d6,2$	| loop until normalized
	dbpl	d6,3$	| make sure d6 decremented
3$:	rts
4$:	movw	#-2222,d6	| exp == 0 for zero
	rts


/*
 * round, check for over/underflow, and pack in the exponent.
 * d_rcp rounds the double value and packs the exponent in,
 * catching infinity, zero, and denormalized numbers.
 * d_usel puts together the larger argument.
 *
 * input:
 *	d2/d3	mantissa (- if norm)
 *	d6	biased exponent
 *	(need sign, sticky)
 * output:
 *	d2/d3	most of number,	no sign or hidden bit,
 *		waiting to shift sign in.
 * other:
 *	d4	lost
 *	d5	unchanged
 */

ENTER(d_rcp)
	tstw	d6
	bgts	2$
	| exponent is negative; denormalize before rounding
	cmpw	#-53,d6
	blts	dsigned0| go all the way with zero
	negw	d6
1$:	lsrl	#1,d2	| denormalize
	roxrl	#1,d3
	bccs	1f	| Check for bit passing out forever.
	bset	#0,d3	| Stick it back on the end.
1:	dbra	d6,1$
	clrw	d6
	| round
2$:	addl	#0x400,d3
	bccs	testeven | Branch if round did not overflow lower part.
	addql	#1,d2	| carry
	bccs	testeven | Branch if round did not overflow significand.
	roxrl	#1,d2
	roxrl	#1,d3
	addqw	#1,d6
	bras	checkbig

testeven:		| Test for ambiguous case to force round to even.
	movw	#0x7ff,d4 | d4 gets rounding mask.
	andw	d3,d4	| d4 gets extra bits left after rounding.
	bnes	checkbig | Branch if it wasn't ambiguous case.
	bclr	#11,d3	| Ambiguous case: force round to even.

checkbig:
	cmpw	#0x7ff,d6
	bges	drcpbig
ENTER(d_usel)
	| rebuild answer
	movl	#0xfffff800,d4
	andl	d4,d3
	andl	d2,d4
	eorl	d4,d2
	orl	d2,d3
	movl	d4,d2
	lsll	#1,d2
	|bcss	4$
	bcss	cout			| Branch if carry out occurred.
	cmpw	#0x7ff,d6
	beqs	4$
	clrw	d6
4$:	
dshiftright:			| Double shift right to pack.
	moveq	#11,d4
	rorl	d4,d3
	orw	d6,d2
	rorl	d4,d2
	rts
dsigned0:	clrl	d2
	clrl	d3
	rts
drcpbig:movl	#0xffe00000,d2	| infinity
	clrl	d3
	rts

cout:
	tstw	d6
	bnes	dshiftright	| Branch if number was not subnormal.
	movw	#1,d6		| Subnormal rounded to normal so fix exp.
	bras	dshiftright

| utilities used for floating point  routines

ENTER(f_snan)
	movl	#0x7fffffff,d0	| Standard 68881 error nan.
	rts

	SAVED0D1 = 0x0003
	RESTD0D1 = 0x0003
	SAVEALL	 = 0x3F00	| registers d2-d4
	RESTALL	 = 0x00fc	

| decode type of arg in d1 and return  1-5 n d0 for zero gu plain inf and nan

ENTER(f_tst)
	lsll	#1,d1		| toss sign
	roll	#8,d1		| exp in low byte
	tstb	d1
	bnes	1$			| branch if not gu or zero
	movl	#1,d0		| assume zero
	tstl	d1	
	beqs	3$			| it is
	movl	#2,d0		| else it's a gu
	bras	3$
1$:	cmpb	#255,d1		| inf or nan ?
	bnes	2$			| nope - plain
	movl	#4,d0		| assume inf
	andl	#0xFFFFFF00,d1  | Clear exponent field.
	beqs	3$			| it is
	movl	#5,d0		| else it'a a nan
	rts
2$:	movl	#3,d0
3$:	rts

| 	Single and double unbiased exponent, d0 to d0.

|	Fexpos(1.0) = 0 = Fexpod(1.0)

RTENTRY(Fexpos)
	andl	#0x7f800000,d0	| Clear out extra bits.
	roll	#1,d0
	roll	#8,d0
	subl	#0x7f,d0
	RET

RTENTRY(Fexpod)
	andl	#0x7ff00000,d0	| Clear out extra bits.
	roll	#4,d0
	roll	#8,d0
	subl	#0x3ff,d0
	RET

|	Switch mode and status.

RTENTRY(Fmode)
#ifdef PIC
	movl	a0,sp@-
	PIC_SETUP(a0)
#endif PIC
	movl	d0,sp@-
#ifdef PIC
	movl	a0@(_Fmode:w),a0 
	movl	a0@,d0
	movl	sp@+,a0@
	movl	sp@+,a0
#else
	movl	_Fmode,d0
	movl	sp@+,_Fmode
#endif PIC
	RET

RTENTRY(Fstatus)
#ifdef PIC
	movl	a0,sp@-
	PIC_SETUP(a0)
#endif PIC
	movl	d0,sp@-
#ifdef PIC
	movl	a0@(_Fstatus:w),a0
	movl	a0@,d0
	movl	sp@+,a0@
	movl	sp@+,a0
#else
	movl	_Fstatus,d0
	movl	sp@+,_Fstatus
#endif
	RET
