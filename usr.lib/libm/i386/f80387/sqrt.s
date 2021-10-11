/ 
/	.data
/	.asciz	"@(#)sqrt.s 1.1 92/07/30 SMI"
/	.even
/	.text
/
/	Copyright (c) 1987 by Sun Microsystems, Inc.

	.text
	.globl	sqrt
sqrt:
	pushl	%ebp
	movl	%esp,%ebp

	fldl	8(%ebp)

	ftst
	fnstsw	%ax
	sahf
	jp	xnotspecial
	jb	illegal
xnotspecial:
	fsqrt
	leave
	ret

illegal:
	/ x < 0
	fstp	%st(0)		/ empty stack
	pushl	$26
	pushl	12(%ebp)
	pushl	8(%ebp)
	pushl	12(%ebp)
	pushl	8(%ebp)
	call	SVID_libm_err
	addl	$20,%esp
	leave
	ret
