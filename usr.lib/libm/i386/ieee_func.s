/ 
/	.data
/	.asciz	"@(#)ieee_func.s 1.1 92/07/30 SMI"
/	.even
/	.text
/
/	Copyright (c) 1987 by Sun Microsystems, Inc.

	.text
	.globl	copysign
	.globl	finite
	.globl	ilogb
	.globl	isinf
	.globl	isnan
	.globl	isnormal
	.globl	issubnormal
	.globl	iszero
	.globl	nextafter
	.globl	scalbn
	.globl	signbit
copysign:
	fldl	4(%esp)
	movl	16(%esp),%eax		/ high part of y
	movl	8(%esp),%ecx		/ high part of x
	xor	%ecx,%eax
	cmpl	$0,%eax
	jge	L1
	fchs	
L1:
	ret

finite:
	movl	$0,%eax
	movl	8(%esp),%ecx		/ high part of x
	andl	$0x7ff00000,%ecx
	cmpl	$0x7ff00000,%ecx
	je	L2
	movl	$1,%eax
L2:
	ret

	.data
	.align	8
two52:	.long	0x0,0x43300000		/ 2**52
	.text
ilogb:
	movl	8(%esp),%ecx		/ high part of x
	movl	%ecx,%eax
	andl	$0x7ff00000,%ecx
	cmpl	$0,%ecx			/ test if x is 0 or subnormal
	jne	L4
	andl	$0x7fffffff,%eax
	orl	4(%esp),%eax		/ test whether x is 0
	cmpl	$0,%eax
	jne	L3
    / x is +-0 return 0x80000001
	movl	$0x80000001,%eax
	ret
L3:	/ subnormal input
	fldl	4(%esp)			/ push x
	push	%ecx
	movl	%esp,%ecx
	subl	$8,%esp
	fmull	two52			/ x*2**52
	fstpl	-8(%ecx)
	movl	-4(%ecx),%eax
	andl	$0x7ff00000,%eax
	shrl	$20,%eax		/ extract exponent of x*2*852
	subl	$1075,%eax		/ unbias it by 1075
	addl	$8,%esp
	leave
	ret
L4:	/ exponent is non-zero
	movl	$0x7fffffff,%eax
	cmpl	$0x7ff00000,%ecx	/ if x is NaN/inf, return 0x7fffffff
	je	L5
	shrl	$20,%ecx
	subl	$1023,%ecx
	movl	%ecx,%eax		/ unbias exponent by 1023
L5:
	ret

isinf:
	movl	$0,%eax
	movl	8(%esp),%ecx		/ high part of x
	andl	$0x7fffffff,%ecx
	cmpl	$0x7ff00000,%ecx
	jne	L6
	cmpl	$0,4(%esp)
	jne	L6
	movl	$1,%eax
L6:
	ret

isnan:
	movl	$0,%eax
	movl	8(%esp),%ecx		/ high part of x
	andl	$0x7fffffff,%ecx
	cmpl	$0x7ff00000,%ecx
	jl	L7
	andl	$0x000fffff,%ecx
	orl	4(%esp),%ecx
	cmpl	$0,%ecx
	je	L7
	movl	$1,%eax
L7:
	ret

isnormal:
	movl	$0,%eax
	movl	8(%esp),%ecx		/ high part of x
	andl	$0x7fffffff,%ecx
	cmpl	$0x00100000,%ecx
	jl	L8
	cmpl	$0x7ff00000,%ecx
	jge	L8
	movl	$1,%eax
L8:
	ret

issubnormal:
	movl	$0,%eax
	movl	8(%esp),%ecx		/ high part of x
	andl	$0x7fffffff,%ecx
	cmpl	$0x00100000,%ecx
	jge	L9
	orl	4(%esp),%ecx
	cmpl	$0,%ecx
	je	L9
	movl	$1,%eax
L9:
	ret

iszero:
	movl	8(%esp),%ecx		/ high part of x
	andl	$0x7fffffff,%ecx
	movl	$1,%eax
	orl	4(%esp),%ecx
	cmpl	$0,%ecx
	je	L10
	movl	$0,%eax
L10:
	ret


	.data
	.align	8
fmax:	.long	0xffffffff,0x7fefffff
fmin:	.long	0x1,0x0
ftmp:	.long	0,0

	.text
nextafter:
	pushl	%ebp
	movl	%esp,%ebp
	subl	$8,%esp
	fldl	16(%ebp)	/ y
	fldl	8(%ebp)		/ load x
	fucom			/ x : y
	fstsw	%ax
	sahf
	jp	NaN
	fstp	%st(1)
	je	return
	ja	bigger
    / x < y
	ftst
	movl	fmin,%ecx
	movl	%ecx,-8(%ebp)
	movl	fmin+4,%ecx
	movl	%ecx,-4(%ebp)
	fstsw	%ax
	sahf
	je	final
	ja	addulp
	jb	subulp
bigger:
    / x > y
	ftst
	movl	fmin,%ecx
	movl	%ecx,-8(%ebp)
	movl	fmin+4,%ecx
	xorl	$0x80000000,%ecx
	movl	%ecx,-4(%ebp)
	fstsw	%ax
	sahf
	je	final
	ja	subulp
	jb	addulp
subulp:
	movl	8(%ebp),%eax	/ low x
	movl	12(%ebp),%ecx	/ high x
	movl	%ecx,-4(%ebp)
	subl	$1,%eax		/ low x - ulp
	movl	%eax,-8(%ebp)
	cmpl	$0xffffffff,%eax
	jne	final
	subl	$1,%ecx
	movl	%ecx,-4(%ebp)
	jmp	final
addulp:
	movl	8(%ebp),%eax	/ low x
	movl	12(%ebp),%ecx	/ high x
	movl	%ecx,-4(%ebp)
	addl	$1,%eax		/ low x - ulp
	movl	%eax,-8(%ebp)
	cmpl	$0x0,%eax
	jne	final
	addl	$1,%ecx
	movl	%ecx,-4(%ebp)

final:
	fstp	%st(0)
	fldl	-8(%ebp)
	andl	$0x7ff00000,%ecx
	je	underflow
	cmpl	$0x7ff00000,%ecx
	je	overflow
	jmp	return
overflow:
	fldl	fmax
	fmul	%st(0),%st
	fstpl	ftmp		/ create overflow signal
	jmp	return
underflow:
	fldl	fmin
	fmul	%st(0),%st
	fstpl	ftmp		/ create underflow signal
	jmp	return
NaN:
	faddp	%st,%st(1)	/ x+y,x
return:
	leave
	ret

scalbn:
	fildl	12(%esp)		/ convert N to extended
	fldl	4(%esp)			/ push x
	fscale
	fstp	%st(1)
	ret

signbit:
	movl	8(%esp),%eax		/ high part of x
	shrl	$31,%eax
	ret
