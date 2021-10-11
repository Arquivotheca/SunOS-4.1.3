/ 
/	.data
/	.asciz	"@(#)exp.s 1.1 92/07/30 SMI"
/	.even
/	.text
/
/	Copyright (c) 1987 by Sun Microsystems, Inc.

	.data
	.align	4
inf:
	.long	0x7f800000

	.text
	.globl	exp
exp:
	pushl	%ebp
	movl	%esp,%ebp

	fldl	8(%ebp)			/ push arg

	fxam
	fnstsw	%ax                     / store status in %ax
	movw	%ax,%cx
	andw	$0x4500,%cx
	cmpw	$0x0100,%cx
	jne	xnotnan
	fld	%st(0)			/ x , x
	faddp	%st,%st(1)		/ x
	leave
	ret

xnotnan:
	andw	$0x4700,%ax
	cmpw	$0x0700,%ax
	jne	xnotninf
	fstp	%st(0)			/ stack empty
	fldz				/ 0
	leave
	ret

xnotninf:
	cmpw	$0x0500,%ax
	jne	xnotspecial
	leave				/ x = +inf
	ret

xnotspecial:
	fldl2e				/ push log2(e)
	fmulp	%st,%st(1)		/ z = arg*log2(e) 
	fld	%st(0)			/ duplicate stack top
	frndint				/ [z],z
	fxch				/ z,[z]
	fucom
	fnstsw	%ax
	sahf
	je	L1
	fsub    %st(1),%st		/ z-[z],[z]
	f2xm1				/ 2**(z-[z])-1, [z]
	fld1				/ 1,2**(z-[z])-1, [z]
	faddp	%st,%st(1)		/ 2**(z-[z]),[z]
	jmp	merge
L1:
	fstp	%st(0)
	fld1
merge:
	fscale
	fstp	%st(1)

	fstpl	-8(%esp)		/ round to double
	fldl	-8(%esp)		/ exp(x) rounded to double
	fxam				/ determine class of exp(x)
	fnstsw	%ax			/ store status in ax
	andw	$0x4500,%ax
	cmpw	$0x0500,%ax
	jne	notoverflow
	fstp	%st(0)			/ stack empty
	pushl	$6
	jmp	error

notoverflow:
	cmpw	$0x4000,%ax
	jne	notunderflow
	fstp	%st(0)			/ stack empty
	pushl	$7
	jmp	error

notunderflow:
	leave
	ret

error:
	pushl	12(%ebp)		/ high x
	pushl	8(%ebp)			/ low x
	pushl	12(%ebp)		/ high x
	pushl	8(%ebp)			/ low x
	call	SVID_libm_err
	addl	$20,%esp
	leave
	ret
