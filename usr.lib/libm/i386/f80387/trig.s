/ 
/	.data
/	.asciz	"@(#)trig.s 1.1 92/07/30 SMI"
/	.even
/	.text
/
/	Copyright (c) 1987 by Sun Microsystems, Inc.
/
/    After argument reduction (calling argred()) which return n:
/       n mod 4     sin(x)      cos(x)        tan(x)
/     ----------------------------------------------------------
/          0          S           C             S/C
/          1          C          -S            -C/S
/          2         -S          -C             S/C
/          3         -C           S            -C/S
/     ----------------------------------------------------------


	.text
	.globl	sin
	.globl	cos
	.globl	tan
	.globl	sincos
	.globl	fp_pi
cos:
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

sin:	
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

tan:	
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

sincos:
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
sincos0: / n=2
	movl	16(%esp),%eax
	fstpl	0(%eax)
	movl	12(%esp),%eax
	fstpl	0(%eax)
	ret


reduction:
	movl	12(%esp),%eax		/ load the high part of arg 
	andl	$0x7ff00000,%eax	/ clear sign and mantissa 
	cmpl    $0x3fe00000,%eax        / Is arge <= (pi/4 = 0x3fe8f5c2) ?
	jle     L0
	cmpl	$0x7ff00000,%eax	/ Is arg a NaN or an Inf ? 
	je	L0
	cmpl	$0x43e00000,%eax	/ Is arg >= 2**63 ? 
	jge	L1
	movl	fp_pi,%ecx
	cmpl	$1,%ecx
	jne	L1
    / arg < 2**63 
L0:
	fldl	8(%esp)			/ push arg
	movl	$0,%eax			/ set n = 0
	ret
L1:
    / call argred to do argument reduction
	pushl	%ebp
	movl	%esp,%ebp
	subl	$16,%esp
	leal	-16(%ebp),%eax		/ address of y2
	pushl	%eax
	leal	-8(%ebp),%eax		/ address of y1
	pushl	%eax
	pushl	16(%ebp)
	pushl	12(%ebp)
	call	argred			/ call argred(x,&y1,&y2)
	fldl	-8(%ebp)		/ push y1
	fldl	-16(%ebp)		/ push y2
	faddp	%st,%st(1)		/ y1+y2
	addl	$16,%esp
	andl	$3,%eax			/ %eax  = n mod 4
	leave
	ret

    / huge argument reduction

.LL0:
        .data
	.align	4
fp_pi:
	.long	1
	.align	4
zero:
	.double	0.0000000000000000000000e+00
	.align	4
one:
	.double	1.0000000000000000000000e+00
	.align	4
p_53:
	.long	10680707
	.long	7228996
	.long	1504196
	.long	15362485
	.long	13885745
	.long	6349266
	.long	14129194
	.long	12595701
	.long	14056396
	.long	16585589
	.long	6328731
	.long	4169915
	.long	6367931
	.long	7425947
	.long	252554
	.long	12605768
	.long	13640692
	.long	5962295
	.long	5333789
	.long	13466256
	.long	12601162
	.long	16164595
	.long	1725153
	.long	6043943
	.long	3856965
	.long	5123509
	.long	6613188
	.long	2574939
	.long	12012252
	.long	1992081
	.long	2630334
	.long	4682229
	.long	1222207
	.long	8633653
	.long	2394314
	.long	2089406
	.long	4375066
	.long	6825321
	.long	2816913
	.long	6860257
	.long	4866789
	.long	10216279
	.long	12031605
	.long	13539722
	.long	8630155
	.long	15091174
	.long	14616457
	.long	10393415
	.long	11146751
	.long	3509311
	.long	11481409
	.long	7513079
	.long	2879956
	.long	15969686
	.long	8222111
	.long	2457550
	.long	2751337
	.long	11645996
	.long	12791265
	.long	9178045
	.long	3271727
	.long	9707558
	.long	8772839
	.long	6635507
	.long	1273767
	.long	883295
	.align	4
p_66:
	.long	10680707
	.long	7228996
	.long	1387008
	.long	404420
	.long	893558
	.long	12506305
	.long	16644357
	.long	6084026
	.long	7765228
	.long	2064598
	.long	12738643
	.long	3801200
	.long	1080362
	.long	16158441
	.long	6847354
	.long	12161194
	.long	3726923
	.long	9883635
	.long	9597790
	.long	9123062
	.long	1661948
	.long	2619534
	.long	11091653
	.long	3758245
	.long	4087059
	.long	1856936
	.long	5982859
	.long	10759684
	.long	1233157
	.long	3495949
	.long	16130887
	.long	4524288
	.long	12159946
	.long	16246948
	.long	10675669
	.long	6929490
	.long	15743005
	.long	1706632
	.long	16680724
	.long	8349493
	.long	12287309
	.long	13039170
	.long	11697792
	.long	6394303
	.long	11974480
	.long	5404526
	.long	14028752
	.long	6323660
	.long	6864815
	.long	15559164
	.long	7254789
	.long	13784445
	.long	2473467
	.long	4082337
	.long	8668973
	.long	9110390
	.long	16217920
	.long	13799647
	.long	1436812
	.long	4564419
	.long	6390255
	.long	12969960
	.long	14192796
	.long	16130372
	.long	2720808
	.long	7232925
	.align	4
p_inf:
	.long	10680707
	.long	7228996
	.long	1387004
	.long	2578385
	.long	16069853
	.long	12639074
	.long	9804092
	.long	4427841
	.long	16666979
	.long	11263675
	.long	12935607
	.long	2387514
	.long	4345298
	.long	14681673
	.long	3074569
	.long	13734428
	.long	16653803
	.long	1880361
	.long	10960616
	.long	8533493
	.long	3062596
	.long	8710556
	.long	7349940
	.long	6258241
	.long	3772886
	.long	3769171
	.long	3798172
	.long	8675211
	.long	12450088
	.long	3874808
	.long	9961438
	.long	366607
	.long	15675153
	.long	9132554
	.long	7151469
	.long	3571407
	.long	2607881
	.long	12013382
	.long	4155038
	.long	6285869
	.long	7677882
	.long	13102053
	.long	15825725
	.long	473591
	.long	9065106
	.long	15363067
	.long	6271263
	.long	9264392
	.long	5636912
	.long	4652155
	.long	7056368
	.long	13614112
	.long	10155062
	.long	1944035
	.long	9527646
	.long	15080200
	.long	6658437
	.long	6231200
	.long	6832269
	.long	16767104
	.long	5075751
	.long	3212806
	.long	1398474
	.long	7579849
	.long	6349435
	.long	12618859
	.align	4
two24:
	.double	1.6777216000000000000000e+07
	.align	4
twon24:
	.double	5.9604644775390625000000e-08
	.align	4
p1:
	.double	1.5707962512969970703125e+00
	.align	4
p2:
	.double	7.5497894158615963533521e-08
	.align	4
p3_inf:
	.double	5.3903025299577647655447e-15
	.align	4
p3_66:
	.double	5.3903008358918702569440e-15
	.align	4
p3_53:
	.double	5.3290705182007513940334e-15
        .text
argred:
	jmp	.L194
.L193:
	movl	$0,-300(%ebp)
	movl	$1,-304(%ebp)
	leal	-76(%ebp),%eax
	movl	%eax,-308(%ebp)
       cmpl	$1072693248,one+4
	jne	.L195
	movl	$0,-304(%ebp)
.L195:
     pushl   12(%ebp)
       pushl   8(%ebp)
	call	signbit
	addl	$8,%esp
	movl	%eax,-288(%ebp)
     pushl   12(%ebp)
       pushl   8(%ebp)
	call	fabs
	addl	$8,%esp
   fstpl 8(%ebp)
     pushl   12(%ebp)
       pushl   8(%ebp)
	call	ilogb
	addl	$8,%esp
	movl	%eax,-296(%ebp)
	movl	-296(%ebp),%eax
	subl	$26,%eax
	movl	$24,%ecx
	cltd
	idivl	%ecx
	movl	%eax,-292(%ebp)
	cmpl	$0,-292(%ebp)
	jg	.L196
	movl	$0,-292(%ebp)
	jmp	.L197
.L196:
	imull	$-24,-292(%ebp),%eax
	pushl	%eax
     pushl   12(%ebp)
       pushl   8(%ebp)
	call	scalbn
	addl	$12,%esp
   fstpl 8(%ebp)
.L197:
       movl    12(%ebp),%eax
       movl    %eax,-72(%ebp)
       movl    8(%ebp),%eax
       movl    %eax,-76(%ebp)
	movl	-308(%ebp),%eax
	movl	-304(%ebp),%edx
	andl	$-32,(%eax,%edx,4)
       movl    -72(%ebp),%eax
       movl    %eax,-24(%ebp)
       movl    -76(%ebp),%eax
       movl    %eax,-28(%ebp)
	movl	-308(%ebp),%eax
	movl	-304(%ebp),%edx
	andl	$-536870912,(%eax,%edx,4)
       movl    -72(%ebp),%eax
       movl    %eax,-8(%ebp)
       movl    -76(%ebp),%eax
       movl    %eax,-12(%ebp)
       fldl  -28(%ebp)
       fsubl -76(%ebp)
   fstpl -20(%ebp)
       fldl  8(%ebp)
       fsubl -28(%ebp)
   fstpl -28(%ebp)
	movl	$p_inf,-4(%ebp)
       movl    p3_inf+4,%eax
       movl    %eax,-64(%ebp)
       movl    p3_inf,%eax
       movl    %eax,-68(%ebp)
       cmpl	$1,fp_pi
	jne	.L198
	movl	$p_66,-4(%ebp)
       movl    p3_66+4,%eax
       movl    %eax,-64(%ebp)
       movl    p3_66,%eax
       movl    %eax,-68(%ebp)
.L198:
       cmpl	$2,fp_pi
	jne	.L199
	movl	$p_53,-4(%ebp)
       movl    p3_53+4,%eax
       movl    %eax,-64(%ebp)
       movl    p3_53,%eax
       movl    %eax,-68(%ebp)
.L199:
       cmpl	$1,-292(%ebp)
	jl	.L200
	movl	-4(%ebp),%eax
	movl	-292(%ebp),%edx
	decl	%edx
       fildl   (%eax,%edx,4)
   fstpl -92(%ebp)
       cmpl	$1,-292(%ebp)
	jle	.L201
	movl	-4(%ebp),%eax
	movl	-292(%ebp),%edx
	subl	$2,%edx
       fildl   (%eax,%edx,4)
   fmull two24
   fstpl -100(%ebp)
.L201:
.L200:
       movl    twon24+4,%eax
       movl    %eax,-80(%ebp)
       movl    twon24,%eax
       movl    %eax,-84(%ebp)
	movl	-4(%ebp),%eax
	movl	-292(%ebp),%edx
       fildl   (%eax,%edx,4)
   fmull -84(%ebp)
   fstpl -260(%ebp)
       fldl  -84(%ebp)
       fmull twon24
   fstpl -84(%ebp)
	movl	-4(%ebp),%eax
	movl	-292(%ebp),%edx
	incl	%edx
       fildl   (%eax,%edx,4)
   fmull -84(%ebp)
   fstpl -252(%ebp)
       fldl  -84(%ebp)
       fmull twon24
   fstpl -84(%ebp)
	movl	-4(%ebp),%eax
	movl	-292(%ebp),%edx
	leal	2(%edx),%edx
       fildl   (%eax,%edx,4)
   fmull -84(%ebp)
   fstpl -244(%ebp)
       fldl  -84(%ebp)
       fmull twon24
   fstpl -84(%ebp)
	movl	-4(%ebp),%eax
	movl	-292(%ebp),%edx
	leal	3(%edx),%edx
       fildl   (%eax,%edx,4)
   fmull -84(%ebp)
   fstpl -236(%ebp)
       fldl  -84(%ebp)
       fmull twon24
   fstpl -84(%ebp)
	movl	-4(%ebp),%eax
	movl	-292(%ebp),%edx
	leal	4(%edx),%edx
       fildl   (%eax,%edx,4)
   fmull -84(%ebp)
   fstpl -228(%ebp)
       fldl  zero
       fldl  -28(%ebp)
	fucompp
       fstsw   %ax
       sahf
	jp	.L202
	jne	.L202
       fldl  zero
       fldl  -20(%ebp)
	fucompp
       fstsw   %ax
       sahf
	jp	.L203
	jne	.L203
       fldl  -12(%ebp)
       fmull -260(%ebp)
   fstpl -180(%ebp)
       fldl  -12(%ebp)
       fmull -252(%ebp)
   fstpl -172(%ebp)
       fldl  -12(%ebp)
       fmull -244(%ebp)
   fstpl -164(%ebp)
       fldl  -12(%ebp)
       fmull -236(%ebp)
   fstpl -156(%ebp)
       fldl  -12(%ebp)
       fmull -228(%ebp)
   fstpl -148(%ebp)
	jmp	.L204
.L203:
	cmpl	$0,-292(%ebp)
	jle	.L205
       fldl  -20(%ebp)
       fmull -92(%ebp)
       fldl  -12(%ebp)
       fmull -260(%ebp)
       faddp	%st,%st(1)
   fstpl -180(%ebp)
	jmp	.L206
.L205:
       fldl  -12(%ebp)
       fmull -260(%ebp)
   fstpl -180(%ebp)
.L206:
       fldl  -20(%ebp)
       fmull -260(%ebp)
       fldl  -12(%ebp)
       fmull -252(%ebp)
       faddp	%st,%st(1)
   fstpl -172(%ebp)
       fldl  -20(%ebp)
       fmull -252(%ebp)
       fldl  -12(%ebp)
       fmull -244(%ebp)
       faddp	%st,%st(1)
   fstpl -164(%ebp)
       fldl  -20(%ebp)
       fmull -244(%ebp)
       fldl  -12(%ebp)
       fmull -236(%ebp)
       faddp	%st,%st(1)
   fstpl -156(%ebp)
       fldl  -20(%ebp)
       fmull -236(%ebp)
       fldl  -12(%ebp)
       fmull -228(%ebp)
       faddp	%st,%st(1)
   fstpl -148(%ebp)
.L204:
	jmp	.L207
.L202:
       cmpl	$1,-292(%ebp)
	jl	.L208
       cmpl	$1,-292(%ebp)
	jle	.L209
       fldl  -20(%ebp)
       fmull -92(%ebp)
       fldl  -28(%ebp)
       fmull -100(%ebp)
       fldl  -12(%ebp)
       fmull -260(%ebp)
       faddp	%st,%st(2)
	fxch	%st(1)
       faddp	%st,%st(1)
   fstpl -180(%ebp)
	jmp	.L210
.L209:
       fldl  -20(%ebp)
       fmull -92(%ebp)
       fldl  -12(%ebp)
       fmull -260(%ebp)
       faddp	%st,%st(1)
   fstpl -180(%ebp)
.L210:
       fldl  -20(%ebp)
       fmull -260(%ebp)
       fldl  -28(%ebp)
       fmull -92(%ebp)
       fldl  -12(%ebp)
       fmull -252(%ebp)
       faddp	%st,%st(2)
	fxch	%st(1)
       faddp	%st,%st(1)
   fstpl -172(%ebp)
	jmp	.L211
.L208:
       fldl  -12(%ebp)
       fmull -260(%ebp)
   fstpl -180(%ebp)
       fldl  -20(%ebp)
       fmull -260(%ebp)
       fldl  -12(%ebp)
       fmull -252(%ebp)
       faddp	%st,%st(1)
   fstpl -172(%ebp)
.L211:
       fldl  -20(%ebp)
       fmull -252(%ebp)
       fldl  -28(%ebp)
       fmull -260(%ebp)
       fldl  -12(%ebp)
       fmull -244(%ebp)
       faddp	%st,%st(2)
	fxch	%st(1)
       faddp	%st,%st(1)
   fstpl -164(%ebp)
       fldl  -20(%ebp)
       fmull -244(%ebp)
       fldl  -28(%ebp)
       fmull -252(%ebp)
       fldl  -12(%ebp)
       fmull -236(%ebp)
       faddp	%st,%st(2)
	fxch	%st(1)
       faddp	%st,%st(1)
   fstpl -156(%ebp)
       fldl  -20(%ebp)
       fmull -236(%ebp)
       fldl  -28(%ebp)
       fmull -244(%ebp)
       fldl  -12(%ebp)
       fmull -228(%ebp)
       faddp	%st,%st(2)
	fxch	%st(1)
       faddp	%st,%st(1)
   fstpl -148(%ebp)
.L207:
        .data
	.align	4
.L212:
	.double	8.0000000000000000000000e+00
	.align	4
.L213:
	.double	1.2500000000000000000000e-01
        .text
       fldl  -180(%ebp)
       fmull .L213
       subl    $8,%esp
       fstpl   0(%esp)
	call	aint
	addl	$8,%esp
   fmull .L212
   fsubrl        -180(%ebp)
   fstpl -180(%ebp)
        .data
	.align	4
.L214:
	.double	8.0000000000000000000000e+00
	.align	4
.L215:
	.double	1.2500000000000000000000e-01
        .text
       fldl  -172(%ebp)
       fmull .L215
       subl    $8,%esp
       fstpl   0(%esp)
	call	aint
	addl	$8,%esp
   fmull .L214
   fsubrl        -172(%ebp)
   fstpl -172(%ebp)
        .data
	.align	4
.L216:
	.double	1.2500000000000000000000e-01
        .text
       fldl  -164(%ebp)
       fmull .L216
	fstcw	-312(%ebp)
	movw	-312(%ebp),%ax
	orw	$0x0c00,%ax
	movw	%ax,-310(%ebp)
	fldcw	-310(%ebp)
     fistpl  -320(%ebp)
	fldcw	-312(%ebp)
	movl	-320(%ebp),%eax
	movl	%eax,-296(%ebp)
	cmpl	$0,-296(%ebp)
	je	.L217
	movl	-296(%ebp),%eax
	leal	(,%eax,8),%eax
	movl	%eax,-316(%ebp)
       fildl   -316(%ebp)
   fsubrl        -164(%ebp)
   fstpl -164(%ebp)
.L217:
       movl    -144(%ebp),%eax
       movl    %eax,-32(%ebp)
       movl    -148(%ebp),%eax
       movl    %eax,-36(%ebp)
       fldl  -36(%ebp)
       faddl -156(%ebp)
   fstpl -36(%ebp)
       fldl  -36(%ebp)
       faddl -164(%ebp)
   fstpl -36(%ebp)
       fldl  -36(%ebp)
       faddl -172(%ebp)
   fstpl -36(%ebp)
       fldl  -36(%ebp)
       faddl -180(%ebp)
   fstpl -36(%ebp)
       fldl  -180(%ebp)
       fsubl -36(%ebp)
   fstpl -52(%ebp)
       fldl  -52(%ebp)
       faddl -172(%ebp)
   fstpl -52(%ebp)
       fldl  -52(%ebp)
       faddl -164(%ebp)
   fstpl -52(%ebp)
       fldl  -52(%ebp)
       faddl -156(%ebp)
   fstpl -52(%ebp)
       fldl  -52(%ebp)
       faddl -148(%ebp)
   fstpl -52(%ebp)
	fstcw	-312(%ebp)
	movw	-312(%ebp),%ax
	orw	$0x0c00,%ax
	movw	%ax,-310(%ebp)
	fldcw	-310(%ebp)
     fldl  -36(%ebp)
       fistpl  -320(%ebp)
	fldcw	-312(%ebp)
	movl	-320(%ebp),%eax
	movl	%eax,-300(%ebp)
   fldl  -36(%ebp)
       fisubl        -300(%ebp)
   fstpl -36(%ebp)
       fldl  -36(%ebp)
       faddl -52(%ebp)
   fstpl -44(%ebp)
       fldl  -36(%ebp)
       fsubl -44(%ebp)
   faddl -52(%ebp)
   fstpl -52(%ebp)
        .data
	.align	4
.L219:
	.double	5.0000000000000000000000e-01
	.align	4
.L220:
	.double	5.0000000000000000000000e-01
        .text
	fldl	.L219
	fldl  -44(%ebp)
	fcompp
	fstsw	%ax
	sahf
	jp	.L221
	ja	.L221
       fldl  .L220
       fldl  -44(%ebp)
	fucompp
       fstsw   %ax
       sahf
	jp	.L218
	jne	.L218
	fldl	zero
	fldl  -52(%ebp)
	fcompp
	fstsw	%ax
	sahf
	jp	.L218
	jbe	.L218
.L221:
	incl	-300(%ebp)
        .data
	.align	4
.L222:
	.double	1.0000000000000000000000e+00
        .text
       fldl  -44(%ebp)
       fsubl .L222
   fstpl -36(%ebp)
       fldl  -36(%ebp)
       faddl -52(%ebp)
   fstpl -44(%ebp)
       fldl  -36(%ebp)
       fsubl -44(%ebp)
   faddl -52(%ebp)
   fstpl -52(%ebp)
.L218:
     pushl   -40(%ebp)
       pushl   -44(%ebp)
	call	ilogb
	addl	$8,%esp
	movl	%eax,-312(%ebp)
     pushl   -152(%ebp)
       pushl   -156(%ebp)
	call	ilogb
	addl	$8,%esp
	movl	-312(%ebp),%edx
	subl	%eax,%edx
	cmpl	$20,%edx
	jge	.L223
       movl    -168(%ebp),%eax
       movl    %eax,-48(%ebp)
       movl    -172(%ebp),%eax
       movl    %eax,-52(%ebp)
   fldl  -180(%ebp)
       fisubl        -300(%ebp)
   fstpl -36(%ebp)
       fldl  -52(%ebp)
       faddl -36(%ebp)
   fstpl -44(%ebp)
       fldl  -44(%ebp)
       fsubl -36(%ebp)
   fsubl -52(%ebp)
   fsubrl        -164(%ebp)
   fstpl -52(%ebp)
       movl    -40(%ebp),%eax
       movl    %eax,-32(%ebp)
       movl    -44(%ebp),%eax
       movl    %eax,-36(%ebp)
       fldl  -52(%ebp)
       faddl -36(%ebp)
   fstpl -44(%ebp)
       fldl  -44(%ebp)
       fsubl -36(%ebp)
   fsubl -52(%ebp)
   fsubrl        -156(%ebp)
   fstpl -52(%ebp)
       movl    -40(%ebp),%eax
       movl    %eax,-32(%ebp)
       movl    -44(%ebp),%eax
       movl    %eax,-36(%ebp)
       fldl  -52(%ebp)
       faddl -36(%ebp)
   fstpl -44(%ebp)
       fldl  -44(%ebp)
       fsubl -36(%ebp)
   fsubl -52(%ebp)
   fsubrl        -148(%ebp)
   fstpl -52(%ebp)
       fldl  -84(%ebp)
       fmull twon24
   fstpl -84(%ebp)
	movl	-4(%ebp),%eax
	movl	-292(%ebp),%edx
	leal	5(%edx),%edx
       fildl   (%eax,%edx,4)
   fmull -84(%ebp)
   fstpl -220(%ebp)
       fldl  zero
       fldl  -28(%ebp)
	fucompp
       fstsw   %ax
       sahf
	jp	.L224
	jne	.L224
       fldl  zero
       fldl  -20(%ebp)
	fucompp
       fstsw   %ax
       sahf
	jp	.L225
	jne	.L225
       fldl  -12(%ebp)
       fmull -220(%ebp)
   fstpl -140(%ebp)
	jmp	.L226
.L225:
       fldl  -20(%ebp)
       fmull -228(%ebp)
       fldl  -12(%ebp)
       fmull -220(%ebp)
       faddp	%st,%st(1)
   fstpl -140(%ebp)
.L226:
	jmp	.L227
.L224:
       fldl  -20(%ebp)
       fmull -228(%ebp)
       fldl  -28(%ebp)
       fmull -236(%ebp)
       fldl  -12(%ebp)
       fmull -220(%ebp)
       faddp	%st,%st(2)
	fxch	%st(1)
       faddp	%st,%st(1)
   fstpl -140(%ebp)
.L227:
     pushl   -40(%ebp)
       pushl   -44(%ebp)
	call	ilogb
	addl	$8,%esp
	movl	%eax,-312(%ebp)
     pushl   -144(%ebp)
       pushl   -148(%ebp)
	call	ilogb
	addl	$8,%esp
	movl	-312(%ebp),%edx
	subl	%eax,%edx
	cmpl	$20,%edx
	jl	.L228
       fldl  -52(%ebp)
       faddl -140(%ebp)
   fstpl -52(%ebp)
	jmp	.L229
.L228:
       movl    -40(%ebp),%eax
       movl    %eax,-32(%ebp)
       movl    -44(%ebp),%eax
       movl    %eax,-36(%ebp)
       fldl  -52(%ebp)
       faddl -36(%ebp)
   fstpl -44(%ebp)
       fldl  -44(%ebp)
       fsubl -36(%ebp)
   fsubl -52(%ebp)
   fsubrl        -140(%ebp)
   fstpl -52(%ebp)
       fldl  -84(%ebp)
       fmull twon24
   fstpl -84(%ebp)
	movl	-4(%ebp),%eax
	movl	-292(%ebp),%edx
	leal	6(%edx),%edx
       fildl   (%eax,%edx,4)
   fmull -84(%ebp)
   fstpl -212(%ebp)
       fldl  zero
       fldl  -28(%ebp)
	fucompp
       fstsw   %ax
       sahf
	jp	.L230
	jne	.L230
       fldl  zero
       fldl  -20(%ebp)
	fucompp
       fstsw   %ax
       sahf
	jp	.L231
	jne	.L231
       fldl  -12(%ebp)
       fmull -212(%ebp)
   fstpl -132(%ebp)
	jmp	.L232
.L231:
       fldl  -20(%ebp)
       fmull -220(%ebp)
       fldl  -12(%ebp)
       fmull -212(%ebp)
       faddp	%st,%st(1)
   fstpl -132(%ebp)
.L232:
	jmp	.L233
.L230:
       fldl  -20(%ebp)
       fmull -220(%ebp)
       fldl  -28(%ebp)
       fmull -228(%ebp)
       fldl  -12(%ebp)
       fmull -212(%ebp)
       faddp	%st,%st(2)
	fxch	%st(1)
       faddp	%st,%st(1)
   fstpl -132(%ebp)
.L233:
     pushl   -40(%ebp)
       pushl   -44(%ebp)
	call	ilogb
	addl	$8,%esp
	movl	%eax,-312(%ebp)
     pushl   -136(%ebp)
       pushl   -140(%ebp)
	call	ilogb
	addl	$8,%esp
	movl	-312(%ebp),%edx
	subl	%eax,%edx
	cmpl	$20,%edx
	jl	.L234
       fldl  -52(%ebp)
       faddl -132(%ebp)
   fstpl -52(%ebp)
	jmp	.L229
.L234:
       movl    -40(%ebp),%eax
       movl    %eax,-32(%ebp)
       movl    -44(%ebp),%eax
       movl    %eax,-36(%ebp)
       fldl  -52(%ebp)
       faddl -36(%ebp)
   fstpl -44(%ebp)
       fldl  -44(%ebp)
       fsubl -36(%ebp)
   fsubl -52(%ebp)
   fsubrl        -132(%ebp)
   fstpl -52(%ebp)
       fldl  -84(%ebp)
       fmull twon24
   fstpl -84(%ebp)
	movl	-4(%ebp),%eax
	movl	-292(%ebp),%edx
	leal	7(%edx),%edx
       fildl   (%eax,%edx,4)
   fmull -84(%ebp)
   fstpl -204(%ebp)
       fldl  zero
       fldl  -28(%ebp)
	fucompp
       fstsw   %ax
       sahf
	jp	.L235
	jne	.L235
       fldl  zero
       fldl  -20(%ebp)
	fucompp
       fstsw   %ax
       sahf
	jp	.L236
	jne	.L236
       fldl  -12(%ebp)
       fmull -204(%ebp)
   fstpl -124(%ebp)
	jmp	.L237
.L236:
       fldl  -20(%ebp)
       fmull -212(%ebp)
       fldl  -12(%ebp)
       fmull -204(%ebp)
       faddp	%st,%st(1)
   fstpl -124(%ebp)
.L237:
	jmp	.L238
.L235:
       fldl  -20(%ebp)
       fmull -212(%ebp)
       fldl  -28(%ebp)
       fmull -220(%ebp)
       fldl  -12(%ebp)
       fmull -204(%ebp)
       faddp	%st,%st(2)
	fxch	%st(1)
       faddp	%st,%st(1)
   fstpl -124(%ebp)
.L238:
       fldl  -52(%ebp)
       faddl -124(%ebp)
   fstpl -52(%ebp)
.L229:
       movl    -40(%ebp),%eax
       movl    %eax,-32(%ebp)
       movl    -44(%ebp),%eax
       movl    %eax,-36(%ebp)
       fldl  -52(%ebp)
       faddl -36(%ebp)
   fstpl -44(%ebp)
       fldl  -44(%ebp)
       fsubl -36(%ebp)
   fsubrl        -52(%ebp)
   fstpl -52(%ebp)
.L223:
   fldl    -44(%ebp)
   fstl  -76(%ebp)
   fstpl -84(%ebp)
       movl    -48(%ebp),%eax
       movl    %eax,-32(%ebp)
       movl    -52(%ebp),%eax
       movl    %eax,-36(%ebp)
	movl	-308(%ebp),%eax
	movl	-304(%ebp),%edx
	andl	$-32,(%eax,%edx,4)
       movl    -72(%ebp),%eax
       movl    %eax,-56(%ebp)
       movl    -76(%ebp),%eax
       movl    %eax,-60(%ebp)
	movl	-308(%ebp),%eax
	movl	-304(%ebp),%edx
	andl	$-536870912,(%eax,%edx,4)
       movl    -72(%ebp),%eax
       movl    %eax,-40(%ebp)
       movl    -76(%ebp),%eax
       movl    %eax,-44(%ebp)
       fldl  -60(%ebp)
       fsubl -76(%ebp)
   fstpl -52(%ebp)
       fldl  -60(%ebp)
       fsubl -84(%ebp)
   fsubrl        -36(%ebp)
   fstpl -60(%ebp)
       fldl  -68(%ebp)
       fmull -44(%ebp)
       fldl  p2
       fmull -52(%ebp)
       faddp	%st,%st(1)
   fstpl -284(%ebp)
       fldl  p1
       fmull -60(%ebp)
   faddl -284(%ebp)
   fstpl -284(%ebp)
       fldl  p2
       fmull -44(%ebp)
       fldl  p1
       fmull -52(%ebp)
       faddp	%st,%st(1)
   fstpl -276(%ebp)
       fldl  p1
       fmull -44(%ebp)
   fstpl -268(%ebp)
       fldl  -268(%ebp)
       faddl -276(%ebp)
   fstpl -44(%ebp)
       fldl  -44(%ebp)
       fsubl -268(%ebp)
   fsubl -276(%ebp)
   fsubrl        -284(%ebp)
   fstpl -52(%ebp)
	movl	16(%ebp),%eax
       fldl  -44(%ebp)
       faddl -52(%ebp)
       fstpl   (%eax)
	movl	20(%ebp),%eax
	movl	16(%ebp),%edx
       fldl  (%edx)
       fsubl -44(%ebp)
   fsubrl        -52(%ebp)
       fstpl   (%eax)
       cmpl	$1,-288(%ebp)
	jne	.L239
	movl	16(%ebp),%eax
	movl	16(%ebp),%edx
       fldl    (%edx)
       fchs
       fstpl   (%eax)
	movl	20(%ebp),%eax
	movl	20(%ebp),%edx
       fldl    (%edx)
       fchs
       fstpl   (%eax)
	movl	-300(%ebp),%eax
	negl	%eax
	movl	%eax,-300(%ebp)
.L239:
	movl	-300(%ebp),%eax
	andl	$7,%eax
	jmp	.L192
.L192:
	leave
	ret
.L194:
	pushl	%ebp
	movl	%esp,%ebp
	subl	$324,%esp
	jmp	.L193
/FUNCEND
        .data
