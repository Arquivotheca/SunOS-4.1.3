/ 
/	.data
/	.asciz	"@(#)log10.s 1.1 92/07/30 SMI"
/	.even
/	.text
/
/	Copyright (c) 1987 by Sun Microsystems, Inc.

	.text
	.globl	log10
log10:
	pushl	%ebp
	movl	%esp,%ebp

	fldl	8(%ebp)
	ftst
	fnstsw	%ax
	sahf
	jp	xnotspecial
	ja	xnotspecial
	fstp	%st(0)		/ empty stack
	je	xiszero
	/ x < 0
	pushl	$19
	jmp	merge
xiszero:
	/ x = 0
	pushl	$18
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
	fldlg2	
	fxch			/ st = arg, st(1) = log10(2)
	fyl2x			/ st = log10(arg) = log10(2)*log2(arg)
	leave
	ret
