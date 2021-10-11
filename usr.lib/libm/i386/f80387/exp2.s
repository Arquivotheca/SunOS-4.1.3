/ 
/	.data
/	.asciz	"@(#)exp2.s 1.1 92/07/30 SMI"
/	.even
/	.text
/
/	Copyright (c) 1987 by Sun Microsystems, Inc.

	.text
	.globl	exp2
exp2:
	fldl	4(%esp)			/ push arg
	fld	%st(0)			/ duplicate stack top
	frndint				/ [z],z
	fxch				/ z,[z]
	fucom
	fnstsw  %ax
	sahf
	je      L1
	fsub    %st(1),%st		/ z-[z],[z]
	f2xm1				/ 2**(z-[z])-1, [z]
	fld1				/ 1,2**(z-[z])-1, [z]
	faddp	%st,%st(1)		/ 2**(z-[z]),[z]
	fscale				/ 2**z = 2**(arg)
	fstp	%st(1)
	ret
L1:
	fstp	%st(0)
	fld1
	fscale
	fstp	%st(1)
	ret
