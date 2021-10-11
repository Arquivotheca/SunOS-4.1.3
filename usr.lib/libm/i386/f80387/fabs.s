/ 
/	.data
/	.asciz	"@(#)fabs.s 1.1 92/07/30 SMI"
/	.even
/	.text
/
/	Copyright (c) 1987 by Sun Microsystems, Inc.

	.text
	.globl	fabs
fabs:
	fldl	4(%esp)
	fabs
	ret
