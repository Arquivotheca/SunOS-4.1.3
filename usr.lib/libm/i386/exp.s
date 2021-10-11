/ 
/	.data
/	.asciz	"@(#)exp.s 1.1 92/07/30 SMI"
/	.even
/	.text
/
/	Copyright (c) 1987 by Sun Microsystems, Inc.

	.text
	.globl	exp
exp:
	fldl	4(%esp)			/ push arg
	fldl2e				/ push log2(e)
	fmulp	%st,%st(1)		/ z = arg*log2(e) 
	fld	%st(0)			/ duplicate stack top
	frndint				/ [z],z
	fxch				/ z,[z]
	fucom
	fnstsw	%ax
	sahf
	je	L1
	fsub    %st(1),%st		/ z-[z],[z]
	f2xm1				/ 2**(z-[z])-1, [z]
	fld1				/ 1,2**(z-[z])-1, [z]
	faddp	%st,%st(1)		/ 2**(z-[z]),[z]
	fscale				/ 2**z = 2**(arg*log2e) = exp(arg)
	fstp	%st(1)
	ret
L1:
	fstp	%st(0)
	fld1
	fscale
	fstp	%st(1)
	ret
