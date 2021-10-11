/ 
/	.data
/	.asciz	"@(#)fmod.s 1.1 92/07/30 SMI"
/	.even
/	.text
/
/	Copyright (c) 1987 by Sun Microsystems, Inc.

	.text
	.globl	fmod
fmod:
	fldl	12(%esp)		/ load arg y
	fldl	4(%esp)			/ load arg x
L1:
	fprem				/ partial fmod
	fstsw	%ax			/ load status word
	andw	$0x400,%ax		/ check if the reduction is completed
	jne	L1			/ if not, do fprem again
	fstp	%st(1)
	ret
