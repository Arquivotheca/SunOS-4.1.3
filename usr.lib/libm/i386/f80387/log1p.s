/ 
/	.data
/	.asciz	"@(#)log1p.s 1.1 92/07/30 SMI"
/	.even
/	.text
/
/	Copyright (c) 1987 by Sun Microsystems, Inc.

	.data
	.align	4
L1:
	.double 0.3

	.text
	.globl	log1p
log1p:
	pushl	%ebp
	movl	%esp,%ebp
	movl	12(%ebp),%eax		/ high part of x
	andl	$0x7fffffff,%eax	/ clear the sign
	cmp	$0x3ff59905,%eax
	jge	L2
    / |x| < 0.3, use fyl2xp1
	fstp	%st(0)			
	fstp	%st(0)			
	fldln2
	fldl	8(%ebp)			/ st = arg, st(1) = ln2
	fyl2xp1				/ st = ln2 * log2(1+x)
	fld	%st(0)
	leave
	ret
L2:
    / |x| >= 0.3, return log(1+x)
	fstp	%st(0)			
	fstp	%st(0)			
	fld1
	fldl	8(%ebp)			/ st = arg, st(1) = 1.0
	faddp	%st,%st(1)		/ st = 1+arg
	fldln2
	fxch
	fyl2x				/ st = log(arg+1) = log(2)*log2(arg+1)
	fld	%st(0)
	leave
	ret
