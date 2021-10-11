/ 
/	.data
/	.asciz	"@(#)sqrt.s 1.1 92/07/30 SMI"
/	.even
/	.text
/
/	Copyright (c) 1987 by Sun Microsystems, Inc.

	.text
	.globl	sqrt
sqrt:
	fldl	4(%esp)
	fsqrt
	ret
