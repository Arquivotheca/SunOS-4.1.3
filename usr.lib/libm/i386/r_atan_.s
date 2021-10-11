/ 
/	.data
/	.asciz	"@(#)r_atan_.s 1.1 92/07/30 SMI"
/	.even
/	.text
/
/	Copyright (c) 1987 by Sun Microsystems, Inc.
	.text
	.globl	r_atan_
r_atan_:
	movl	4(%esp),%eax		/ address of arg
	flds	0(%eax)			/ push arg
	fld1				/ push 1.0
	fpatan				/ atan(arg/1.0)
	ret
