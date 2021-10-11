/ 
/	.data
/	.asciz	"@(#)pow.s 1.1 92/07/30 SMI"
/	.even
/	.text
/
/	Copyright (c) 1987 by Sun Microsystems, Inc.

	.text
	.globl	pow
pow:
	pushl	%ebp
	movl	%esp,%ebp
	sub	$16,%esp		/ need space for st(2)
	fstp	%st(0)			
	fstp	%st(0)			
	fstpt	-16(%ebp)		/ three registers stack space for pow
	fldl	16(%ebp)		/ y
	fldl	8(%ebp)			/ st = x, st(1) = y
	fyl2x				/ st = log(x) = y*log2(x)
	fld	%st(0)			/ duplicate stack top
	frndint				/ st = [st] rounded to integral
	fxch				/ st <-> st(1)
	fsub    %st(1),%st		/ st = st - st(1)  ( = t - [t] )
	f2xm1				/ 2**(t-[t]) - 1
	fld1
	faddp	%st,%st(1)		/ st = 2**(t-[t])
	fscale				/ st = 2**t = 2**(x*log2e)
	fldt	-16(%ebp)		/ restoring st(2)
	fxch	%st(2)
	fxch				/ result put back on the stack
	leave
	ret
