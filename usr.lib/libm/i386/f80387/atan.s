/ 
/	.data
/	.asciz	"@(#)atan.s 1.1 92/07/30 SMI"
/	.even
/	.text
/
/	Copyright (c) 1987 by Sun Microsystems, Inc.

	.text
	.globl	atan
atan:
	fldl	4(%esp)			/ push arg
	fld1				/ push 1.0
	fpatan				/ atan(arg/1.0)
	ret
