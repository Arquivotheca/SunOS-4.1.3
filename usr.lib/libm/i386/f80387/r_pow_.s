/ 
/	.data
/	.asciz	"@(#)r_pow_.s 1.1 92/07/30 SMI"
/	.even
/	.text
/
/	Copyright (c) 1988 by Sun Microsystems, Inc.

/     Note :  0^NaN should not signal "invalid" but this implementation
/             does because y is placed on the NPX stack.

/ Algorithm (SVID msg only applies to double precision) :
/
/ x ** 0 is 1				(SVID DOMAIN err if 0**0)
/ x ** NaN (except 0) is NaN		(i flag)
/ NaN ** y (except 0) is NaN		(i flag)
/ x ** 1 is x
/ +-(|x| > 1) **  +inf is +inf
/ +-(|x| > 1) **  -inf is +0
/ +-(|x| < 1) **  +inf is +0
/ +-(|x| < 1) **  -inf is +inf
/ +-(|x| = 1) ** +-inf is NaN		(i flag)
/ +0 ** +y (except 0, NaN)              is +0
/ -0 ** +y (except 0, NaN, odd int)     is +0
/ +0 ** -y (except 0, NaN)              is +inf (z flag & SVID SING err)
/ -0 ** -y (except 0, NaN, odd int)     is +inf (z flag & SING err)
/ -0 ** y (odd int)			is - (+0 ** x)
/ +inf ** +y (except 0, NaN)    	is +inf
/ +inf ** -y (except 0, NaN)    	is +0
/ -inf ** +-y (except 0, NaN)   	is -0 ** -+y (NO z flag or SVID msg)
/ x ** -1 is 1/x
/ x ** 2 is square(x)
/ -x ** y (an integer) is (-1)**(y) * (+x)**(y)
/ x ** y (x negative & y not integer) is NaN (i flag & SVID neg#**non-int msg)
/ if x and y are finite and x**y = 0	print SVID underflow message
/ if x and y are finite and x**y = inf	print SVID overflow message

	.data
	.align	4
negzero:
	.float	-0.0
half:
	.float	0.5
one:
	.float	1.0
negone:
	.float	-1.0
two:
	.float	2.0
Snan:
	.long	0x7f800001
pinfinity:
	.long	0x7f800000
ninfinity:
	.long	0xff800000

	.text
	.globl	r_pow_

r_pow_:
	pushl	%ebp
	movl	%esp,%ebp
	subl	$8,%esp

	movl	8(%ebp),%eax
	flds	0(%eax)			/ x
	fxam				/ determine class of x
	fnstsw	%ax			/ store status in %ax
	movb	%ah,%dh			/ %dh <- condition code of x

	movl	12(%ebp),%eax
	flds	0(%eax)			/ y , x
	fxam				/ determine class of y
	fnstsw	%ax			/ store status in %ax
	movb	%ah,%dl			/ %dl <- condition code of y

	call	_pow_main_
	leave
	ret

_pow_main_:
	/ x ** 0 is 1
	movb	%dl,%cl
	andb	$0x45,%cl
	cmpb	$0x40,%cl		/ C3=1 C2=0 C1=? C0=0 when +-0
	jne	POWYNOTZERO
	fstp	%st(0)			/ x
	fstp	%st(0)			/ stack empty
	fld1				/ 1
	ret

POWYNOTZERO:
	/ x ** NaN (except 0) is NaN
	movb	%dl,%cl
	andb	$0x45,%cl
	cmpb	$0x01,%cl		/ C3=0 C2=0 C1=? C0=1 when +-NaN
	jne	POWYNOTNAN
	fstp	%st(1)			/ y
	ret

POWYNOTNAN:
	/ NaN ** y (except 0) is NaN
	movb	%dh,%cl
	andb	$0x45,%cl
	cmpb	$0x01,%cl		/ C3=0 C2=0 C1=? C0=1 when +-NaN
	jne	POWXNOTNAN
	fstp	%st(0)			/ x
	ret

POWXNOTNAN:
	/ x ** 1 is x
	fcoms	one			/ y , x
	fnstsw	%ax			/ store status in %ax
	sahf				/ 80387 flags in %ax to 80386 flags
	je	retx

	/ +-(|x| > 1) **  +inf is +inf
	/ +-(|x| > 1) **  -inf is +0
	/ +-(|x| < 1) **  +inf is +0
	/ +-(|x| < 1) **  -inf is +inf
	/ +-(|x| = 1) ** +-inf is NaN
	movb	%dl,%cl
	andb	$0x47,%cl
	cmpb	$0x05,%cl		/ C3=0 C2=1 C1=0 C0=1 when +inf
	je	yispinf
	cmpb	$0x07,%cl		/ C3=0 C2=1 C1=1 C0=1 when -inf
	je	yisninf

	/ +0 ** +y (except 0, NaN)		is +0
	/ -0 ** +y (except 0, NaN, odd int)	is +0
	/ +0 ** -y (except 0, NaN)		is +inf (z flag)
	/ -0 ** -y (except 0, NaN, odd int)	is +inf (z flag)
	/ -0 ** y (odd int)			is - (+0 ** x)
	movb	%dh,%cl
	andb	$0x47,%cl
	cmpb	$0x40,%cl		/ C3=1 C2=0 C1=0 C0=0 when +0
	je	xispzero
	cmpb	$0x42,%cl		/ C3=1 C2=0 C1=1 C0=0 when -0
	je	xisnzero

	/ +inf ** +y (except 0, NaN)	is +inf
	/ +inf ** -y (except 0, NaN)	is +0
	/ -inf ** +-y (except 0, NaN)	is -0 ** -+y (NO z flag)
	movb	%dh,%cl
	andb	$0x47,%cl
	cmpb	$0x05,%cl		/ C3=0 C2=1 C1=0 C0=1 when +inf
	je	xispinf
	cmpb	$0x07,%cl		/ C3=0 C2=1 C1=1 C0=1 when -inf
	je	xisninf

	/ x ** -1 is 1/x
	fcoms	negone			/ y , x
	fnstsw	%ax			/ store status in %ax
	sahf				/ 80387 flags in %ax to 80386 flags
	jne	POWYNOTN1
	fstp	%st(0)			/ x
	fdivrs	one			/ 1/x
	ret

POWYNOTN1:
	/ x ** 2 is square(x)
	fcoms	two			/ y , x
	fnstsw	%ax			/ store status in %ax
	sahf				/ 80387 flags in %ax to 80386 flags
	jne	POWYNOT2
	fstp	%st(0)			/ x
	fld	%st(0)			/ x , x
	fmul				/ x^2
	ret

POWYNOT2:
	/ -x ** y (an integer) is (-1)**(y) * (+x)**(y)
	/ x ** y (x negative & y not integer) is  NaN
	/ if x < 0 and y is an odd int then	%ecx = $1
	/ else					%ecx = $0
	/ px = |x|
	movl	$0,%ecx
	fld	%st(1)			/ x , y , x
	ftst				/ compare %st(0) with 0
	fnstsw	%ax			/ store status in %ax
	sahf				/ 80387 flags in %ax to 80386 flags
	fstp	%st(0)			/ y , x
	ja	merge			/ x > 0
	/ x < 0
	fld	%st(0)			/ y , y , x
	frndint				/ [y] , y , x
	fucomp				/ y , x
	fnstsw	%ax			/ store status in %ax
	sahf				/ 80387 flags in %ax to 80386 flags
	je	POWXNEGYINT		/ x < 0 & y = int
	/ x < 0 & y != int so x**y = NaN (i flag)
	fstp	%st(0)			/ x
	fstp	%st(0)			/ empty stack
	flds	Snan			/ Q_nan - raise invalid flag
	ret
POWXNEGYINT:
	/ x < 0 & y = int
	fxch				/ x , y
	fchs				/ px = -x , y
	fxch				/ y , px
	fld	%st(0)			/ y , y , px
	fmuls	half			/ y/2 , y , px
	fld	%st(0)			/ y/2 , y/2 , y , px
	frndint				/ [y/2] , y/2 , y , px
	fucompp				/ y , px
	fnstsw	%ax			/ store status in %ax
	sahf				/ 80387 flags in %ax to 80386 flags
	je	merge			/ y = even
	movl	$1,%ecx
merge:
	/ px > 0
	fxch				/ px , y

	/ x**y   =   exp(y*ln(x))
	fyl2x				/ t=y*log2(px)
	fld	%st(0)			/ t , t
	frndint				/ [t] , t
	fxch				/ t , [t]
	fucom
	fnstsw	%ax			/ store status in %ax
	sahf				/ 80387 flags in %ax to 80386 flags
	je	xisint			/ px = int
	fsub    %st(1),%st		/ t-[t] , [t]
	f2xm1				/ 2**(t-[t])-1 , [t]
	fadds	one			/ 2**(t-[t]) , [t]
	fscale				/ 2**t = px**y , [t]
	jmp	done
xisint:
	fstp    %st(0)                  / t=[t]
	fld1                            / 1 , t
	fscale                          / 1*2**t = x**y , t
done:
	fstp	%st(1)			/ x**y
	cmpl	$1,%ecx
	jne	signok
	fchs				/ change sign since x<0 & y=-int
signok:
	ret

/ ------------------------------------------------------------------------

xispinf:
	ftst				/ compare %st(0) with 0
	fnstsw	%ax			/ store status in %ax
	sahf				/ 80387 flags in %ax to 80386 flags
	ja	retpinf			/ y > 0
	jmp	retpzero		/ y < 0

xisninf:
	/ -inf ** +-y is -0 ** -+y
	fchs				/ -y , x
	flds	negzero			/ -0 , -y , x
	fstp	%st(2)			/ -y , -0
	jmp	xisnzero

yispinf:
	fld	%st(1)			/ x , y , x
	fabs				/ |x| , y , x
	fcomps	one			/ y , x
	fnstsw	%ax			/ store status in %ax
	sahf				/ 80387 flags in %ax to 80386 flags
	je	retinvalid		/ |x| = 1
	ja	retpinf			/ |x| > 1
	jmp	retpzero		/ |x| < 1

yisninf:
	fld	%st(1)			/ x , y , x
	fabs				/ |x| , y , x
	fcomps	one			/ y , x
	fnstsw	%ax			/ store status in %ax
	sahf				/ 80387 flags in %ax to 80386 flags
	je	retinvalid		/ |x| = 1
	ja	retpzero		/ |x| > 1
	jmp	retpinf			/ |x| < 1

xispzero:
	/ y cannot be 0 or NaN ; stack has	y , x
	ftst				/ compare %st(0) with 0
	fnstsw	%ax			/ store status in %ax
	sahf				/ 80387 flags in %ax to 80386 flags
	ja	retpzero		/ y > 0
	/ x = +0 & y < 0 so x**y = +inf
	jmp	retpinfzflag		/ ret +inf & z flag

xisnzero:
	/ y cannot be 0 or NaN ; stack has	y , x
	fld	%st(0)			/ y , y , x
	frndint				/ [y] , y , x
	fucomp				/ y , x
	fnstsw	%ax			/ store status in %ax
	sahf				/ 80387 flags in %ax to 80386 flags
	jne	ynotoddint		/ y != int
	fld	%st(0)			/ y , y , x
	fmuls	half			/ y/2 , y , x
	fld	%st(0)			/ y/2 , y/2 , y , x
	frndint				/ [y/2] , y/2 , y , x
	fucompp				/ y , x
	fnstsw	%ax			/ store status in %ax
	sahf				/ 80387 flags in %ax to 80386 flags
	je	ynotoddint		/ y = even
	/ y is an odd integer
	ftst				/ compare %st(0) with 0
	fnstsw	%ax			/ store status in %ax
	sahf				/ 80387 flags in %ax to 80386 flags
	ja	retnzero		/ y > 0
	/ x = -0 & y < 0 (odd int)	return -inf (z flag)
	/ x = -inf & y != 0 or NaN	return -inf (NO z flag)
	movb	%dh,%cl
	andb	$0x45,%cl
	cmpb	$0x05,%cl		/ C3=0 C2=1 C1=? C0=1 when +-inf
	je	POWXINF1
	jmp	retninfzflag            / ret -inf & z flag
POWXINF1:
	jmp	retninf			/ return -inf (NO d flag)
ynotoddint:
	ftst				/ compare %st(0) with 0
	fnstsw	%ax			/ store status in %ax
	sahf				/ 80387 flags in %ax to 80386 flags
	ja	retpzero		/ y > 0
	/ x = -0 & y < 0 (not odd int)	return +inf (z flag)
	/ x = -inf & y not 0 or NaN 	return +inf (NO z flag)
	movb	%dh,%cl
	andb	$0x45,%cl
	cmpb	$0x05,%cl		/ C3=0 C2=1 C1=? C0=1 when +-inf
	je	POWXINF2
	jmp	retpinfzflag		/ ret +inf & z flag
POWXINF2:
	jmp	retpinf			/ return +inf (NO z flag)

retx:
	fstp	%st(0)			/ x
	ret

retpzero:
	fstp	%st(0)			/ x
	fstp	%st(0)			/ stack empty
	fldz				/ +0
	ret

retnzero:
	fstp	%st(0)			/ x
	fstp	%st(0)			/ stack empty
	flds	negzero			/ -0
	ret

retpone:
	fstp	%st(0)			/ x
	fstp	%st(0)			/ stack empty
	fld1				/ 1
	ret

retinvalid:
	fstp	%st(0)			/ x
	fstp	%st(0)			/ stack empty
	flds	Snan			/ Q NaN (i flag)
	ret

retpinf:
	fstp	%st(0)			/ x
	fstp	%st(0)			/ stack empty
	flds	pinfinity		/ +inf
	ret

retninf:
	fstp	%st(0)			/ x
	fstp	%st(0)			/ stack empty
	flds	ninfinity		/ -inf
	ret

retpinfzflag:
	fstp	%st(0)			/ x
	fstp	%st(0)			/ stack empty
	fldz				/ 0
	fdivrs	one			/ 1/0 - raise z flag
	ret

retninfzflag:
	fstp	%st(0)			/ x
	fstp	%st(0)			/ stack empty
	fldz				/ 0
	fdivrs	negone			/ -1/0 - raise z flag
	ret
