/ 
/	.data
/	.asciz	"@(#)remainder.s 1.1 92/07/30 SMI"
/	.even
/	.text
/
/	Copyright (c) 1987 by Sun Microsystems, Inc.

	.text
	.globl	remainder
	.globl	drem
remainder:
drem:
	fldl	12(%esp)		/ load arg y
	fldl	4(%esp)			/ load arg x
L1:
	fprem1				/ partial remainder
	fstsw	%ax			/ load status word
	andw	$0x400,%ax		/ check if the reduction is completed
	jne	L1			/ if not, do fprem1 again
	fstp	%st(1)
	ret
