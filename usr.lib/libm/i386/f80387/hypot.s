/ 
/	.data
/	.asciz	"@(#)hypot.s 1.1 92/07/30 SMI"
/	.even
/	.text
/
/	Copyright (c) 1987 by Sun Microsystems, Inc.

	.data
	.align	4
inf:
	.long	0x7f800000

	.text
	.globl	hypot
hypot:
	pushl	%ebp
	movl	%esp,%ebp

	movl	12(%ebp),%eax		/ high part of x
	movl	8(%ebp),%ecx		/ low part of x
	andl	$0x7fffffff,%eax
	movl	%ecx,%edx
	orl	%eax,%edx
	cmpl	$0,%edx
	je	Ly			/ if x = 0 return y
	subl	$0x7ff00000,%eax
	orl	8(%ebp),%eax
	cmpl	$0,%eax
	jne	L1
Lx:
	fldl	8(%ebp)
	fabs	
	leave
	ret				/ return inf if x is inf
L1:
	movl	20(%ebp),%eax		/ high part of y
	movl	16(%ebp),%ecx		/ low part of y
	andl	$0x7fffffff,%eax
	movl	%ecx,%edx
	orl	%eax,%edx
	cmpl	$0,%edx
	je	Lx			/ if y = 0 return x
	subl	$0x7ff00000,%eax
	orl	16(%ebp),%eax
	cmpl	$0,%eax
	jne	L2
Ly:
	fldl	16(%ebp)
	fabs	
	leave
	ret				/ return inf if y is inf
L2:
	fldl	16(%ebp)		/ ,y
	fmul	%st(0),%st		/ ,y*y
	fldl	8(%ebp)			/ x,y*y
	fmul	%st(0),%st		/ x*x,y*y
	faddp	%st,%st(1)		/ x*x+y*y
	fsqrt				/ sqrt(x*x+y*y)
	fstpl	-8(%esp)		/ round to double
	fldl	-8(%esp)		/ sqrt(x*x+y*y) rounded to double
	flds	inf			/ inf , sqrt(x*x+y*y)
	fucomp
	fnstsw	%ax			/ store status in %ax
	sahf				/ 80387 flags in %ax to 80386 flags
	jp	done
	ja	done
	/	overflow occurred
	fstp	%st(0)			/ stack empty
	pushl	$4
	pushl	20(%ebp)		/ high y
	pushl	16(%ebp)		/ low y
	pushl	12(%ebp)		/ high x
	pushl	8(%ebp)			/ low x
	call	SVID_libm_err
	addl	$20,%esp
done:
	leave
	ret
