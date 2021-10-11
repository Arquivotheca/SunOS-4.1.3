/ 
/	.data
/	.asciz	"@(#)rint.s 1.1 92/07/30 SMI"
/	.even
/	.text
/
/	Copyright (c) 1987 by Sun Microsystems, Inc.

	.text
	.globl	aint
	.globl	anint
	.globl	ceil
	.globl	floor
	.globl	irint
	.globl	nint
	.globl	rint
aint:
	movl	%esp,%eax
	subl	$8,%esp
	fnstcw	-8(%eax)
	movw	-8(%eax),%cx
	orw	$0x0c00,%cx
	movw	%cx,-4(%eax)
	fldcw	-4(%eax)		/ set RD = rounded to zero
	fldl	4(%eax)
	frndint
	fldcw	-8(%eax)		/ restore RD
	addl	$8,%esp
	ret

ceil:
	movl	%esp,%eax
	subl	$8,%esp
	fnstcw	-8(%eax)
	movw	-8(%eax),%cx
	orw	$0x0c00,%cx
	xorw	$0x0400,%cx
	movw	%cx,-4(%eax)
	fldcw	-4(%eax)		/ set RD = rounded to +inf
	fldl	4(%eax)
	frndint
	fldcw	-8(%eax)		/ restore RD
	addl	$8,%esp
	ret

floor:
	movl	%esp,%eax
	subl	$8,%esp
	fnstcw	-8(%eax)
	movw	-8(%eax),%cx
	orw	$0x0c00,%cx
	xorw	$0x0800,%cx
	movw	%cx,-4(%eax)
	fldcw	-4(%eax)		/ set RD = rounded to -inf
	fldl	4(%eax)
	frndint
	fldcw	-8(%eax)		/ restore RD
	addl	$8,%esp
	ret

rint:
	fldl	4(%esp)			/ load x
	frndint				/ [x]
	ret

irint:
	movl	%esp,%ecx
	subl	$8,%esp
	fldl	4(%ecx)			/ load x
	frndint
	fistpl	-8(%ecx)		/ [x]
	movl	-8(%ecx),%eax
	addl	$8,%esp
	ret

	.data
	.align	8
half:	.double	0.5
	.text
anint:
	movl	%esp,%ecx
	subl	$8,%esp
	fnstcw	-8(%ecx)
	movw	-8(%ecx),%dx
	andw	$0xf3ff,%dx
	movw	%dx,-4(%ecx)
	fldcw	-4(%ecx)	/ set RD = rounded to nearest
	fldl	4(%ecx)
	fld	%st(0)
	frndint			/ [x],x
	fldcw	-8(%ecx)	/ restore RD
	fucom			/ check if x is already an integer
	fstsw	%ax
	sahf
	jp	L0
	je	L0
	fxch			/ x,[x]
	fsub	%st(1),%st	/ x-[x],[x]
	fabs			/ |x-[x]|,[x]
	fcoml	half
	fnstsw	%ax
	sahf
	jb	L0		/ if |x-[x]| < 0.5 goto L0, this 
				/ eleminates most cases.
    / x = n+0.5, recompute anint(x) by x+sign(x)*0.5
	fldl	4(%ecx)		/ x, 0.5, [x]
	movl	8(%ecx),%eax	/ high part of x
	andl	$0x80000000,%eax
	jz	L1
    / x is negative, return x-0.5
	fsubp	%st,%st(1)	/ x-0.5,[x]
	fxch
	jmp	L0
L1:
	fadd
	fxch
L0:
	addl	$8,%esp
	fstp	%st(0)
	ret

nint:
	pushl	%ebp
	movl	%esp,%ebp
	subl	$8,%esp
	pushl	12(%ebp)
	pushl	8(%ebp)
	call	anint
	fistl	-8(%ebp)
	movl	-8(%ebp),%eax
	fstp	%st(0)
	leave
	ret

