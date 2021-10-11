/ 
/	.data
/	.asciz	"@(#)hypot.s 1.1 92/07/30 SMI"
/	.even
/	.text
/
/	Copyright (c) 1987 by Sun Microsystems, Inc.

	.text
	.globl	hypot
hypot:
	movl	8(%esp),%eax		/ high part of x
	movl	4(%esp),%ecx		/ low part of x
	andl	$0x7fffffff,%eax
	subl	$0x7ff00000,%eax
	orl	4(%esp),%eax
	cmpl	$0,%eax
	jne	L1
	fldl	4(%esp)
	fabs	
	ret				/ return inf if x is inf
L1:
	movl	16(%esp),%eax		/ high part of y
	movl	12(%esp),%ecx		/ low part of y
	andl	$0x7fffffff,%eax
	subl	$0x7ff00000,%eax
	orl	12(%esp),%eax
	cmpl	$0,%eax
	jne	L2
	fldl	12(%esp)
	fabs	
	ret				/ return inf if y is inf
L2:
	fldl	12(%esp)		/ ,y
	fmul	%st(0),%st		/ ,y*y
	fldl	4(%esp)			/ x,y*y
	fmul	%st(0),%st		/ x*x,y*y
	faddp	%st,%st(1)		/ x*x+y*y
	fsqrt				/ sqrt(x*x+y*y)
	ret
