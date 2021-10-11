/ 
/	.data
/	.asciz	"@(#)expm1.s 1.1 92/07/30 SMI"
/	.even
/	.text
/
/	Copyright (c) 1988 by Sun Microsystems, Inc.

	.data
	.align	8
mhundred:	.double	-100.0

	.text
	.globl	expm1
expm1:
	movl	8(%esp),%eax		/ get high part of x
	movl	%eax,%ecx
	andl	$0x7fffffff,%ecx
	cmpl	$0x3fe00000,%ecx
	jge	L4
    / |x| < 0.5, goto L0 with st(0)=x,st(1)=0
	fldz
	fldl	4(%esp)			/ push x
	fldl2e				/ push log2e
	fmulp	%st,%st(1)		/ z = x*log2e
	jmp	L0
L4:
	movl	4(%esp),%edx		/ get low part of x
	cmpl	$0,%edx
	jne	L2
	cmpl	$0x7ff00000,%ecx
	jne	L2
	cmpl	$0x7ff00000,%eax
	je	L3
    / x = -inf, return -1
	fld1	
	fchs
	ret

    / x = +inf, return +inf
L3:
	fldl	4(%esp)
	ret

    / input x is not +-inf
L2:
	fldl	4(%esp)			/ push x
	fldl2e				/ push log2e
	fmulp	%st,%st(1)		/ z = x*log2e
	fld	%st(0)			/ duplicate stack top
	frndint				/ [z],z
	fxch				/ z,[z]
	fsub    %st(1),%st		/ z-[z],[z]
L0:
	f2xm1				/ 2**(z-[z])-1,[z]
	fldz				/ 0,2**(z-[z])-1,[z]
	fucomp	%st(2)			/ see if [z] is zero
	fstsw	%ax
	sahf
	je	L1
    / [z] != 0, compute exp(x) and then subtract one to get expm1(x)
    / But first avoid spurious underflow in exp 
	fldl	mhundred
	fucom	%st(2)			/ if -100 !< [z], then use -100
	fstsw	%ax
	sahf
	jb	L5
	fxch	%st(2)
L5:
	fstp	%st(0)
	fld1				/ 1,2**(z-[z])-1,[z]
	faddp	%st,%st(1)		/ 2**(z-[z]),[z]
	fscale				/ ,exp(x),[z]
	fld1				/ 1,exp(x),[z]
	fsubrp	%st,%st(1)		/ ,exp(x)-1,[z]
L1:
	fstp	%st(1)
	ret
