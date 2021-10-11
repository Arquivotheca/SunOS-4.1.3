/ 
/	.data
/	.asciz	"@(#)atan2.s 1.1 92/07/30 SMI"
/	.even
/	.text
/
/	Copyright (c) 1987 by Sun Microsystems, Inc.

	.text
	.globl	atan2
atan2:
	fldl	4(%esp)			/ push y
	fldl	12(%esp)		/ push x
	fpatan				/ return atan2(y,x)
	ret
