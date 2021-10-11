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
	fldl	4(%esp)			/ push x
	fld1				/ push 1
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
	ret
