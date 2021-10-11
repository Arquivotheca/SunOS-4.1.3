/ 
/	.data
/	.asciz	"@(#)r_hypot_.s 1.1 92/07/30 SMI"
/	.even
/	.text
/
/	Copyright (c) 1987 by Sun Microsystems, Inc.

	.text
	.globl	r_hypot_
r_hypot_:
	movl	4(%esp),%eax		/ address of x
	movl	8(%esp),%edx		/ address of y
	movl	0(%eax),%ecx		/ x
	andl	$0x7fffffff,%ecx
	cmpl	$0,%ecx
	je	Ly			/ x is zero, return y
	subl	$0x7f800000,%ecx
	cmpl	$0,%ecx
	je	Lx			/ x is inf, return +inf
L1:
	movl	0(%edx),%ecx		/ y
	andl	$0x7fffffff,%ecx
	cmpl	$0,%ecx
	je	Lx			/ return x if y is zero
	subl	$0x7f800000,%ecx
	cmpl	$0,%ecx
	je	Ly			/ return inf if y is inf
L2:
	flds	0(%edx)			/ ,y
	fmul	%st(0),%st		/ ,y*y
	flds	0(%eax)			/ x,y*y
	fmul	%st(0),%st		/ x*x,y*y
	faddp	%st,%st(1)		/ x*x+y*y
	fsqrt				/ sqrt(x*x+y*y)
	ret
Lx:
	flds	0(%eax)			/ return |x|
	fabs	
	ret
Ly:
	flds	0(%edx)			/ return |y|
	fabs
	ret
