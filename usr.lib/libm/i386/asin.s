/ 
/	.data
/	.asciz	"@(#)asin.s 1.1 92/07/30 SMI"
/	.even
/	.text
/
/	Copyright (c) 1987 by Sun Microsystems, Inc.
	.text
	.globl	asin
asin:
	fldl	4(%esp)			/ push x
	fld1				/ push 1
	fadd	%st(1),%st		/ 1+x,x
	fld1				/ 1,1+x,x
	fsub	%st(2),%st		/ 1-x,1+x,x
	fmulp	%st,%st(1)		/ (1-x)*(1+x),x
	fsqrt				/ sqrt((1-x)/(1+x)),x
	fpatan				/ atan(x/sqrt((1-x)/(1+x)))
	ret
