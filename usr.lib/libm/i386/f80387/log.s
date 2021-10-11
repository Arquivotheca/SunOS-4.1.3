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
	pushl	%ebp
	movl	%esp,%ebp

	fldl	8(%ebp)		/ push x

	ftst
	fnstsw	%ax
	sahf
	jp	xnotspecial
	ja	xnotspecial
	fstp	%st(0)		/ empty stack
	je	xiszero
	/ x < 0
	pushl	$17
	jmp	merge
xiszero:
	/ x = 0
	pushl	$16
merge:
	pushl	12(%ebp)
	pushl	8(%ebp)
	pushl	12(%ebp)
	pushl	8(%ebp)
	call	SVID_libm_err
	addl	$20,%esp
	leave
	ret

xnotspecial:
	fldln2			/ push log(2)
	fxch			/ x,log(2)
	fyl2x			/ st = log(arg) = log(2)*log2(arg)
	leave
	ret
