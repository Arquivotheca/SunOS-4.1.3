/ 
/	.data
/	.asciz	"@(#)atan2.s 1.1 92/07/30 SMI"
/	.even
/	.text
/
/	Copyright (c) 1987 by Sun Microsystems, Inc.

	.text
	.globl	atan2
atan2:
	pushl	%ebp
	movl	%esp,%ebp

	movl	8(%ebp),%eax		/ low part of y
	movl	16(%ebp),%ecx		/ low part of x
	orl	%eax,%ecx
	cmpl	$0,%ecx
	jne	POW1
	movl	12(%ebp),%eax		/ high part of y
	movl	20(%ebp),%ecx		/ high part of x
	orl	%eax,%ecx
	andl	$0x7fffffff,%ecx	/ clear sign
	cmpl	$0,%ecx
	jne	POW1
	/ both x and y are 0's
	pushl	$3
	pushl	12(%ebp)		/ high y
	pushl	8(%ebp)			/ low y
	pushl	20(%ebp)		/ high x
	pushl	16(%ebp)		/ low x
	call	SVID_libm_err		/ report SVID result/error
	addl	$20,%esp
	leave
	ret

	/ not both x and y are 0's
POW1:
	fldl	8(%esp)			/ push y
	fldl	16(%esp)		/ push x
	fpatan				/ return atan2(y,x)
	leave
	ret
