	.data
|	.asciz	"@(#)q.s 1.1 92/07/30 Copyr 1984 Sun Micro"
	.even
	.text

|	Copyright (c) 1984 by Sun Microsystems, Inc.

| quad arithmetic: manipulates:
|	struct quad { unsigned q[2]; };

	param1	=	8
	param2	=	12
	param3	=	16
	q0	=	0
	q1	=	4

	.globl	__qcmp
|
| int
| _qcmp( a, b ) struct quad *a, *b;
| {
|	if (*a > *b) return 1;
|	if (*a <  *b) return -1;
|	return 0;
| }

__qcmp:	link	a6,#0
#if PROF
	.data
1$:	.long	0
	.text
	lea	1$,a0
	jsr	mcount
#endif
	movl	a6@(param1), a0
	movl	a6@(param2), a1
	cmpml	a0@+,a1@+
	bhis	5f
	bcss	7f
	cmpml	a0@+,a1@+
	bhis	5f
	bcss	7f
	| (*a == *b)
	clrl	d0
	bras	9f
5:	| (*a < *b)
	moveq	#-1,d0
	bras	9f
7:	| (*a > *b)
	moveq	#1,d0
9:	unlk	a6
	rts

	.globl __qtoa
|	convert a 64-bit unsigned integer into a decimal character
|	string. Do it the obvious way, by successive divides by 10.
|	parameters:
|		value pointer: get number here
|		character pointer: put characters here
|	return:
|		pointer to null dropped after last digit.
|	generalization of routine itoa.
|
|	register usage:
	ten	=	d0	| keep  value 10 here.
	zero	=	d0	| also keep value '0' here
	val	=	d1	| dividend
	loval	=	d2	| more dividend
	work	=	d3	| do division here
	xwork	=	d4
	ndigit	=	d5	| count digits
	mask	=	d6	| 0xffff
	string	=	a0	| output pointer
	stemp	=	a1	| point to collection of digits
|	work area:
	NSAVED	=	5
	frame	=	28+(4*NSAVED)
	tmpstr 	=	-4*(NSAVED+1)
	
__qtoa:
	link	a6,#-frame
#ifdef	PROF
	.data
1$:	.long	0
	.text
	lea	1$,a0
	jsr	mcount
#endif PROF
	| save registers
	movl	d2,sp@-
	movl	d3,sp@-
	movl	d4,sp@-
	movl	d5,sp@-
	movl	d6,sp@-
	moveq	#0,ndigit
	movl	a6@(param1),a0
	movl	a0@+,val	| get 64-bit value
	movl	a0@,loval
	movl	#10,ten
	lea	a6@(tmpstr),stemp | successive remainders go here.
	moveq	#0,mask
	subqw	#1,mask		| #0x0000ffff
vldiv:	| as long as "val" > 0xa0000, must do 4-word division
	movl	val,work
	swap	work
	andl	mask,work
	jeq	ldiv48		| can do 48-bit division
	divu	ten,work
	movw	work,xwork	| first quotient here
	movw	val,work
	divu	ten,work	| second division
	movw	xwork,val
	swap	val
	movw	work,val	| high-order word of quotient
	movl	loval,xwork
	swap	xwork
	movw	xwork,work
	divu	ten,work
	movw	work,xwork	| third quotient
	movw	loval,work
	divu	ten,work	| forth division
	movw	xwork,loval
	swap	loval
	movw	work,loval	| final work of quotient
	swap	work		| final remainder
	movb	work,stemp@+	| save off the remainder. it is <10
	addqw	#1,ndigit
	jra	vldiv		| go do it again.

ldiv48:	| as long as "val" > 0xa, must do 3-word division
	movl	val,work
	jeq	ldiv32		| can do 32 bit division
	divu	ten,work	| first divide
	movw	work,val	| save first quotient
	movl	loval,xwork
	swap	xwork
	movw	xwork,work
	divu	ten,work	| second divide
	movw	work,xwork	| second quotient
	movw	loval,work
	divu	ten,work	| third divide
	movw	xwork,loval
	swap	loval
	movw	work,loval	| final quotient
	swap	work		| final remainder
	movb	work,stemp@+	| save off the remainder. it is <10
	addqw	#1,ndigit
	jra	ldiv48		| go do it again.

ldiv32:	| as long as "loval" > 0xa0000, must do 2-word division
	movl	loval,work
	swap	work
	andl	mask,work
	jeq	sdiv		| can do 16-bit division
	divu	ten, work
	movw	work, xwork
	movw	loval,work
	divu	ten,work
	movw	xwork,loval
	swap	loval
	movw	work,loval	| quotient of 2 divides
	swap	work
	movb	work, stemp@+
	addqw	#1,ndigit
	jra	ldiv32

sdiv:	| as long as "loval" > 0, do 16-bit divides
	divu	ten,loval
	movl	loval,work
	swap	work
	movb	work, stemp@+
	addqw	#1, ndigit
	andl	mask, loval
	jne	sdiv

	| DONE DIVIDING!! shovel result, then split
	movb	#0x30,zero	| '0'
	subqw	#1, ndigit
	movl	a6@(param2),string	| pointer to output area
copyloop:
	movb	stemp@-, work
	addb	zero,work
	movb	work, string@+
	dbra	ndigit,copyloop

	movb	#0,string@	| null terminate

	movl	string,d0	| return value.
	movl	sp@+,d6
	movl	sp@+,d5
	movl	sp@+,d4
	movl	sp@+,d3
	movl	sp@+,d2
	unlk	a6
	rts


	.globl	__atoq
|
|	convert a character string into a 64-bit unsigned number.
|	parameters:
|	    string  -- the 1st digit of the character string (need not be 
|			null terminated!)
|	    strnlen -- how many characters to convert
|	    result  -- address of the quad where we want the answer.
|	does not return a value.
|	register usage:
|	zero	=	d0	| keep value '0' here
|	val	=	d1	| accumulated value
|	loval	=	d2	| more accumulated value
|	work	=	d3	| do multiplies here
|	xwork	=	d4
|	ndigit	=	d5	| count digits
	charval	=	d6	| incoming character
	zedval	=	d7	| contains zero in long loop
|	string	=	a0	| input pointer, later output pointer

__atoq:
	link	a6,#0
#ifdef	PROF
	.data
1$:	.long	0
	.text
	lea	1$,a0
	jsr	mcount
#endif PROF
	| save registers
	movl	d2,sp@-
	movl	d3,sp@-
	movl	d4,sp@-
	movl	d5,sp@-
	movl	d6,sp@-
	movl	d7,sp@-
	moveq	#0x30,zero	| value '0'
	moveq	#0,charval
	movl	a6@(param1), string
	movl	a6@(param2), ndigit
	moveq	#0,val
	moveq	#0,loval
	moveq	#0,zedval
	| first 9 digits are cheap, since they can fit in a single word.
	| set ndigit = min( ndigit, 9) -1
	| and keep difference around (in high halfword ) for later.
	moveq	#9,work
	cmpl	ndigit,work
	blts	1f
	    movw	ndigit,work
1:	| work = min( ndigit, 9 )
	subw	work,ndigit	| difference
	swap	ndigit		| in high bits
	movw	work,ndigit	| count in low bits
	jeq	atoqed		| oops -- count was zero
	subqw	#1,ndigit	| base zero -- will be using dbra
	bras	1f		| jump into middle of loop
shortl:	| short accumulation loop
	addl	loval,loval
	movl	loval,work
	lsll	#2,loval
	addl	work,loval	| loval *= 10
1:	movb	a0@+,charval
	subl	zero,charval
	addl	charval,loval	| loval += *ch++ - '0'
	dbra	ndigit, shortl

	swap	ndigit		| get residual
	subqw	#1, ndigit	| base zero for dbra
	jlt	atoqed		| may be done
longl:	| long accumulation loop
	addl	loval,loval
	addxl	val,val
	movl	val,work
	movl	loval,xwork
	addl	loval,loval
	addxl	val,val
	addl	loval,loval
	addxl	val,val
	addl	xwork,loval
	addxl	work,val	| (val,loval) *= 10
	movb	a0@+,charval
	subl	zero,charval
	addl	charval,loval
	addxl	zedval,val	| (val,loval) += *ch++ - '0'
	dbra	ndigit,longl

atoqed:	| all done
	movl	a6@(param3),a0
	movl	val,a0@+
	movl	loval,a0@
	movl	sp@+,d7
	movl	sp@+,d6
	movl	sp@+,d5
	movl	sp@+,d4
	movl	sp@+,d3
	movl	sp@+,d2
	unlk	a6
	rts
