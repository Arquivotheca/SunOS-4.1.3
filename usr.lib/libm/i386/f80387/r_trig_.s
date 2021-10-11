/ 
/	.data
/	.asciz	"@(#)r_trig_.s 1.1 92/07/30 SMI"
/	.even
/	.text
/
/	Copyright (c) 1987 by Sun Microsystems, Inc.
/
/    After argument reduction (calling r_argred()) which return n:
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
	movl	8(%esp),%ecx		/ address of the float arg 
	movl	0(%ecx),%eax		/ load the float arg 
	andl	$0x7f800000,%eax	/ clear sign and mantissa 
	cmpl    $0x3f000000,%eax        / Is arg < (pi/4 = 3f47ae14)?
	jle     L0
	cmpl	$0x7f800000,%eax	/ Is arg Inf or NaN?
	je	L0
	cmpl	$0x5f000000,%eax	/ Is arg >= 2**63 ? 
	jge	L1
	movl	fp_pi,%edx
	cmpl	$1,%edx
	jne	L1
    / arg < 2**63 
L0:
	flds	0(%ecx)			/ push arg
	movl	$0,%eax			/ set n = 0
	ret
L1:
    / call r_argred to do argument reduction
	movl	8(%esp),%ecx		/ address of the float arg 
	pushl	%ebp
	movl	%esp,%ebp
	subl	$8,%esp
	leal	-8(%ebp),%eax		/ address of y
	pushl	%eax
	flds	0(%ecx)			/ load float arg
	fstpl	-8(%ebp)		/ convert it to double
	pushl	-4(%ebp)
	pushl	-8(%ebp)
	call	r_argred		/ call r_argred(x,&y)
	fldl	-8(%ebp)		/ push y1
	addl	$8,%esp
	andl	$3,%eax			/ %eax  = n mod 4
	leave
	ret

     / huge argument reduction

.LL0:
        .data
	.align	4
pio2:
	.double	1.5707963267948965579990e+00
	.align	4
fv_53:
	.double	6.3661974668502807617188e-01
	.double	2.5682552973194106016308e-08
	.double	3.1852589278203238820630e-16
	.double	1.9390182117670744872737e-22
	.double	1.0446473474853647777497e-29
	.double	2.8471093425247276750834e-37
	.double	3.7764011105246390170275e-44
	.double	2.0066109378142940278225e-51
	.align	4
fv_66:
	.double	6.3661974668502807617188e-01
	.double	2.5682552973194106016308e-08
	.double	2.9371036852632315117262e-16
	.double	5.1044980366317055095138e-24
	.double	6.7223832392451941186914e-31
	.double	5.6080211170808900346172e-37
	.double	4.4486450011776007248209e-44
	.double	9.6924126077195290972029e-52
	.align	4
fv_inf:
	.double	6.3661974668502807617188e-01
	.double	2.5682552973194106016308e-08
	.double	2.9370952149337589687228e-16
	.double	3.2543794001732456382344e-23
	.double	1.2089613708828537201148e-29
	.double	5.6675567957400713586778e-37
	.double	2.6204031112097214596761e-44
	.double	7.0539576808806286227718e-52
        .text
r_argred:
	jmp	.L186
.L185:
	movl	$fv_66,-68(%ebp)
	cmpl	$0,fp_pi
	jne	.L187
	movl	$fv_inf,-68(%ebp)
.L187:
       cmpl	$2,fp_pi
	jne	.L188
	movl	$fv_53,-68(%ebp)
.L188:
     pushl   12(%ebp)
       pushl   8(%ebp)
	call	signbit
	addl	$8,%esp
	movl	%eax,-80(%ebp)
     pushl   12(%ebp)
       pushl   8(%ebp)
	call	fabs
	addl	$8,%esp
   fstpl 8(%ebp)
     pushl   12(%ebp)
       pushl   8(%ebp)
	call	ilogb
	addl	$8,%esp
	movl	%eax,-88(%ebp)
	movl	-88(%ebp),%eax
	subl	$26,%eax
	movl	$24,%ecx
	cltd
	idivl	%ecx
	movl	%eax,-100(%ebp)
	cmpl	$0,-100(%ebp)
	jge	.L189
	movl	$0,-100(%ebp)
.L189:
	movl	-88(%ebp),%eax
	leal	60(%eax),%eax
	movl	$24,%ecx
	cltd
	idivl	%ecx
	movl	%eax,-104(%ebp)
	movl	-104(%ebp),%eax
	subl	-100(%ebp),%eax
	movl	%eax,-84(%ebp)
	movl	$0,-96(%ebp)
.L192:
	movl	-84(%ebp),%eax
	cmpl	%eax,-96(%ebp)
	jg	.L191
	leal	-64(%ebp),%eax
	movl	-96(%ebp),%edx
	leal	(,%edx,8),%edx
	addl	%edx,%eax
	movl	-96(%ebp),%edx
	addl	-100(%ebp),%edx
	leal	(,%edx,8),%edx
	movl	-68(%ebp),%ecx
	addl	%edx,%ecx
       fldl  (%ecx)
       fmull 8(%ebp)
       fstpl   (%eax)
.L190:
	incl	-96(%ebp)
	jmp	.L192
.L191:
        .data
	.align	4
.L193:
	.double	8.0000000000000000000000e+00
	.align	4
.L194:
	.double	1.2500000000000000000000e-01
        .text
       fldl  -64(%ebp)
       fmull .L194
       subl    $8,%esp
       fstpl   0(%esp)
	call	aint
	addl	$8,%esp
   fmull .L193
   fsubrl        -64(%ebp)
   fstpl -64(%ebp)
        .data
	.align	4
.L195:
	.double	8.0000000000000000000000e+00
	.align	4
.L196:
	.double	1.2500000000000000000000e-01
        .text
       fldl  -56(%ebp)
       fmull .L196
       subl    $8,%esp
       fstpl   0(%esp)
	call	aint
	addl	$8,%esp
   fmull .L195
   fsubrl        -56(%ebp)
   fstpl -56(%ebp)
        .data
	.align	4
.L197:
	.double	1.2500000000000000000000e-01
        .text
       fldl  -48(%ebp)
       fmull .L197
	fstcw	-108(%ebp)
	movw	-108(%ebp),%ax
	orw	$0x0c00,%ax
	movw	%ax,-106(%ebp)
	fldcw	-106(%ebp)
     fistpl  -116(%ebp)
	fldcw	-108(%ebp)
	movl	-116(%ebp),%eax
	movl	%eax,-88(%ebp)
	cmpl	$0,-88(%ebp)
	je	.L198
	movl	-88(%ebp),%eax
	leal	(,%eax,8),%eax
	movl	%eax,-112(%ebp)
       fildl   -112(%ebp)
   fsubrl        -48(%ebp)
   fstpl -48(%ebp)
.L198:
	leal	-64(%ebp),%eax
	movl	-84(%ebp),%edx
	leal	(,%edx,8),%edx
	addl	%edx,%eax
       fldl    (%eax)
   fstpl   -76(%ebp)
	movl	-84(%ebp),%eax
	decl	%eax
	movl	%eax,-96(%ebp)
.L201:
	cmpl	$0,-96(%ebp)
	jl	.L200
	leal	-64(%ebp),%eax
	movl	-96(%ebp),%edx
	leal	(,%edx,8),%edx
	addl	%edx,%eax
       fldl  -76(%ebp)
       faddl (%eax)
   fstpl -76(%ebp)
.L199:
	decl	-96(%ebp)
	jmp	.L201
.L200:
	fstcw	-108(%ebp)
	movw	-108(%ebp),%ax
	orw	$0x0c00,%ax
	movw	%ax,-106(%ebp)
	fldcw	-106(%ebp)
     fldl  -76(%ebp)
       fistpl  -116(%ebp)
	fldcw	-108(%ebp)
	movl	-116(%ebp),%eax
	movl	%eax,-92(%ebp)
   fldl  -76(%ebp)
       fisubl        -92(%ebp)
   fstpl -76(%ebp)
        .data
	.align	4
.L203:
	.double	5.0000000000000000000000e-01
        .text
	fldl	.L203
	fldl  -76(%ebp)
	fcompp
	fstsw	%ax
	sahf
	jp	.L202
	jbe	.L202
	incl	-92(%ebp)
        .data
	.align	4
.L204:
	.double	1.0000000000000000000000e+00
        .text
       fldl  -76(%ebp)
       fsubl .L204
   fstpl -76(%ebp)
.L202:
        .data
	.align	4
.L206:
	.double	1.9073486328125000000000e-06
        .text
     pushl   -72(%ebp)
       pushl   -76(%ebp)
	call	fabs
	addl	$8,%esp
	fldl	.L206
	fxch	%st(1)
	fcompp
	fstsw	%ax
	sahf
	jp	.L205
	jae	.L205
   fldl  -64(%ebp)
       fisubl        -92(%ebp)
   fstpl -76(%ebp)
	movl	$1,-96(%ebp)
.L209:
	movl	-84(%ebp),%eax
	cmpl	%eax,-96(%ebp)
	jg	.L208
	leal	-64(%ebp),%eax
	movl	-96(%ebp),%edx
	leal	(,%edx,8),%edx
	addl	%edx,%eax
       fldl  -76(%ebp)
       faddl (%eax)
   fstpl -76(%ebp)
.L207:
	incl	-96(%ebp)
	jmp	.L209
.L208:
.L205:
	movl	16(%ebp),%eax
       fldl  -76(%ebp)
       fmull pio2
       fstpl   (%eax)
	cmpl	$0,-80(%ebp)
	je	.L210
	movl	16(%ebp),%eax
	movl	16(%ebp),%edx
       fldl    (%edx)
       fchs
       fstpl   (%eax)
	movl	-92(%ebp),%eax
	negl	%eax
	movl	%eax,-92(%ebp)
.L210:
	movl	-92(%ebp),%eax
	andl	$7,%eax
	jmp	.L184
.L184:
	leave
	ret
.L186:
	pushl	%ebp
	movl	%esp,%ebp
	subl	$116,%esp
	jmp	.L185
/FUNCEND
        .data
