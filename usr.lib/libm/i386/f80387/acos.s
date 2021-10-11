/ 
/	.data
/	.asciz	"@(#)acos.s 1.1 92/07/30 SMI"
/	.even
/	.text
/
/	Copyright (c) 1987 by Sun Microsystems, Inc.

	.text
	.globl	acos
acos:
	pushl   %ebp
	movl    %esp,%ebp
		 
	fldl	8(%esp)			/ push x
	fld1				/ push 1
	fld	%st(1)			/ x , 1 , x
	fabs				/ |x| , 1 , x
	fucomp
	fstsw   %ax
	sahf
	ja	ERR
	fadd	%st(1),%st		/ 1+x,x
	fldz
	fucomp	
	fstsw	%ax
	sahf
	jp	L1
	jne	L1
	/ x is -1 
	fxch	%st(1)
	fchs
	fxch	%st(1)			/ 0,+1
	fpatan
	fadd	%st(0),%st
	leave
	ret
L1:
	fxch	%st(1)			/ x,1+x
	fld1				/ 1,x,1+x
	fsubp	%st,%st(1)		/ 1-x,1+x
	fdivp	%st,%st(1)		/ (1-x)/(1+x)
	fsqrt
	fld1				/ 1,sqrt((1-x)/(1+x))
	fpatan
	fadd	%st(0),%st
	leave
	ret

ERR:
	/ |x| > 1
	fstp	%st(0)			/ x
	fstp	%st(0)			/ empty NPX stack
	pushl   $1
	pushl   12(%ebp)                / high x
	pushl   8(%ebp)                 / low x
	pushl   12(%ebp)                / high x
	pushl   8(%ebp)                 / low x
	call    SVID_libm_err		/ report SVID result/error
	addl    $20,%esp
	leave
	ret
