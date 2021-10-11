/ 
/	.data
/	.asciz	"@(#)r_trig_.s 1.1 92/07/30 SMI"
/	.even
/	.text
/
/	Copyright (c) 1987 by Sun Microsystems, Inc.
/
/    After argument reduction (calling _arg_red()) which return n:
/       n mod 4     sin(x)      cos(x)        tan(x)
/     ----------------------------------------------------------
/          0          S           C             S/C
/          1          C          -S            -C/S
/          2         -S          -C             S/C
/          3         -C           S            -C/S
/     ----------------------------------------------------------

	.text
	.globl	r_sin_
	.globl	r_cos_
	.globl	r_tan_
	.globl	r_sincos_
	.globl	fp_pi
	.globl	_arg_red
r_cos_:
	call	reduction
	cmpl	$1,%eax
	jl	cos0
	je	cos1
	cmpl	$2,%eax
	je	cos2
	fsin
	ret
cos2:
	fcos
	fchs
	ret
cos1:
	fsin
	fchs
	ret
cos0:
	fcos
	ret

r_sin_:	
	call	reduction
	cmpl	$1,%eax
	jl	sin0
	je	sin1
	cmpl	$2,%eax
	je	sin2
	fcos
	fchs
	ret
sin2:
	fsin
	fchs
	ret
sin1:
	fcos
	ret
sin0:
	fsin
	ret

r_tan_:	
	call	reduction
	andl	$1,%eax
	cmpl	$0,%eax
	je	tan1
	fptan
	fdivp	%st,%st(1)
	fchs
	ret
tan1:
	fptan
	fstp	%st(0)
	ret

r_sincos_:
	call	reduction
	fsincos
	cmpl	$1,%eax
	jl	sincos0
	je	sincos1
	cmpl	$2,%eax
	je	sincos2
/ n=3
	fchs
	movl	12(%esp),%eax
	fstpl	0(%eax)
	movl	16(%esp),%eax
	fstpl	0(%eax)
	ret
sincos2: / n=2
	fchs
	movl	16(%esp),%eax
	fstpl	0(%eax)
	fchs
	movl	12(%esp),%eax
	fstpl	0(%eax)
	ret
sincos1: / n=1
	movl	12(%esp),%eax
	fstpl	0(%eax)
	fchs
	movl	16(%esp),%eax
	fstpl	0(%eax)
	ret
sincos0: / n=0
	movl	16(%esp),%eax
	fstpl	0(%eax)
	movl	12(%esp),%eax
	fstpl	0(%eax)
	ret



reduction:
	movl	fp_pi,%ecx
	cmpl	$1,%ecx
	jne	L1
	movl	8(%esp),%ecx		/ address of the float arg 
	movl	0(%ecx),%eax		/ load the float arg 
	andl	$0x7f800000,%eax	/ clear sign and mantissa 
	cmpl	$0x5f000000,%eax	/ Is arg >= 2**63 ? 
	jge	L1
    / arg < 2**63 
	flds	0(%ecx)			/ push arg
	movl	$0,%eax			/ set n = 0
	ret
L1:
    / call _arg_red to do argument reduction
	movl	8(%esp),%ecx		/ address of the float arg 
	pushl	%ebp
	movl	%esp,%ebp
	subl	$16,%esp
	leal	-16(%ebp),%eax		/ address of y2
	pushl	%eax
	leal	-8(%ebp),%eax		/ address of y1
	pushl	%eax
	flds	0(%ecx)			/ load float arg
	fstpl	-8(%ebp)		/ convert it to double
	pushl	-4(%ebp)
	pushl	-8(%ebp)
	call	_arg_red		/ call _arg_red(x,&y1,&y2)
	fldl	-8(%ebp)		/ push y1
	fldl	-16(%ebp)		/ push y2
	faddp	%st,%st(1)		/ y1+y2
	addl	$16,%esp
	andl	$3,%eax			/ %eax  = n mod 4
	leave
	ret
