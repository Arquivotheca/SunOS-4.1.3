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
	pushl   %ebp
	movl    %esp,%ebp
	 
	fldl	8(%esp)			/ push x
	fld1				/ push 1
	fld	%st(1)			/ x , 1 , x
	fabs				/ |x| , 1 , x
	fucomp
	fnstsw  %ax
	sahf     
	ja	ERR
	fadd	%st(1),%st		/ 1+x,x
	fld1				/ 1,1+x,x
	fsub	%st(2),%st		/ 1-x,1+x,x
	fmulp	%st,%st(1)		/ (1-x)*(1+x),x
	fsqrt				/ sqrt((1-x)/(1+x)),x
	fpatan				/ atan(x/sqrt((1-x)/(1+x)))
	leave
	ret

ERR:
	/ |x| > 1
	fstp	%st(0)			/ x
	fstp	%st(0)			/ empty NPX stack
	pushl   $2
	pushl   12(%ebp)                / high x
	pushl   8(%ebp)                 / low x
	pushl   12(%ebp)                / high x
	pushl   8(%ebp)                 / low x
	call    SVID_libm_err		/ report SVID result/error
	addl    $20,%esp
	leave
	ret
