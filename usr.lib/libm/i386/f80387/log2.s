/ 
/	.data
/	.asciz	"@(#)log2.s 1.1 92/07/30 SMI"
/	.even
/	.text
/
/	Copyright (c) 1987 by Sun Microsystems, Inc.

	.text
	.globl	log2
log2:
	fldl	4(%esp)		/ push x
	fld1			/ push 1.0
	fxch			/ st = arg, st(1) = log(2)
	fyl2x			/ st = 1.0*log2(arg)
	ret
