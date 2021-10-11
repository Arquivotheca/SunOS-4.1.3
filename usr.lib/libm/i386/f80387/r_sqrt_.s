/ 
/	.data
/	.asciz	"@(#)r_sqrt_.s 1.1 92/07/30 SMI"
/	.even
/	.text
/
/	Copyright (c) 1987 by Sun Microsystems, Inc.
	.text
	.globl	r_sqrt_
r_sqrt_:
	movl	4(%esp),%eax
	flds	0(%eax)
	fsqrt
	ret
