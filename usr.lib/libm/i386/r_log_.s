/ 
/	.data
/	.asciz	"@(#)r_log_.s 1.1 92/07/30 SMI"
/	.even
/	.text
/
/	Copyright (c) 1987 by Sun Microsystems, Inc.
	.text
	.globl	r_log_
r_log_:
	movl	4(%esp),%eax
	flds	0(%eax) 	/ push arg
	fldln2			/ push log(2)
	fxch			/ x,log(2)
	fyl2x			/ st = log(arg) = log(2)*log2(arg)
	ret

