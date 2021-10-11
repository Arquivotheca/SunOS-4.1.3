/ 
/	.data
/	.asciz	"@(#)log.s 1.1 92/07/30 SMI"
/	.even
/	.text
/
/	Copyright (c) 1987 by Sun Microsystems, Inc.

	.text
	.globl	log
log:
	fldl	4(%esp)		/ push x
	fldln2			/ push log(2)
	fxch			/ x,log(2)
	fyl2x			/ st = log(arg) = log(2)*log2(arg)
	ret

