
	.data
	.asciz	"@(#)r_log_.S 1.1 92/07/30 SMI"

#define LOCORE
#include <machine/asm_linkage.h>

! Copyright (c) 1989 by Sun Microsystems, Inc.
!
! single log(x) float *x;
! Code by K.C. Ng (based on libm/R/r_log_.c)
! April 8, 1987
! Revised on May 9, 1988
! Rewritten on Jan 12, 1989
!
! use l5 for n
	.global	__tbl_r_log_hi,__tbl_r_log_lo
	.seg	"text"
	ENTRY(r_log_)
	save	%sp,-128,%sp
	set	constant,%l0
	ld	[%i0],%f0		! f0 = x
	mov	0,%l5			! l5 = n <- 0
	ld	[%i0],%i0		! i0 = ix
	sethi	%hi(0x3f6c0000),%l1
	cmp	%i0,%l1
	bge,a	1f
	sethi	%hi(0x3f8a0000),%l2
	sethi	%hi(0x3dc00000),%l1
	cmp	%i0,%l1
	bge	SMALL_N
	sethi	%hi(0x00800000),%l1
	cmp	%i0,%l1
	bge	LARGE_N
	sethi	%hi(0x80000000),%l1
	andncc	%i0,%l1,%g0
	be	RLOG_0
	tst	%i0
	bl	RLOG_NEG
	ld	[%l0+two24],%f4
    ! subnormal x
	fmuls	%f0,%f4,%f0
	mov	-24,%l5
	st	%f0,[%fp-4]
	ba	LARGE_N
	ld	[%fp-4],%i0
RLOG_0:
	ld	[%l0+one],%f2
	ld	[%l0+zero],%f4
	fnegs	%f2,%f2
	ba	exit
	fdivs	%f2,%f4,%f0
RLOG_NEG:
	sethi	%hi(0xff800000),%l1
	cmp	%i0,%l1
	bge,a	exit
	fsubs	%f0,%f0,%f0	! return x-x if x is -inf of NaN
	ld	[%l0+zero],%f2	
! log(-ve)
	ba	exit
	fdivs	%f2,%f2,%f0
1:
	cmp	%i0,%l2
	bl,a	near1
	ld	[%l0+one],%f2
	sethi	%hi(0x41bc0000),%l1
	cmp	%i0,%l1
	bl	SMALL_N
	sethi	%hi(0x7f800000),%l1
	cmp	%i0,%l1
	bl	LARGE_N
	nop
	ba	exit
	fadds	%f0,%f0,%f0	! x now is +Inf or NaN
near1:
! Now |1-x|<0.078125 ( 0x3f6c0000 <= x < 0x3f8a0000 )
	fsubs	%f0,%f2,%f4	! f4 = f = x - 1.0
	sethi	%hi(0x3f800000),%l1
	cmp	%i0,%l1
	be,a	exit
	ld	[%l0+zero],%f0
	fmuls	%f4,%f4,%f6	! f6 = z = f*f
	bl,a	less_1
	sethi	%hi(0x3f7f8000),%l1
    ! x > 1
	sethi	%hi(0x3f804000),%l1
	cmp	%i0,%l1
	bge,a	psmall
	ld	[%l0+Bp4],%f8
	sethi	%hi(0x3f800400),%l1
	cmp	%i0,%l1
	bge,a	ptiny
	ld	[%l0+Bp2],%f8
	ba	pvtiny
	ld	[%l0+Bp1],%f8
psmall:				! for 0 <= x < 1+0.078125
	fmuls	%f4,%f8,%f8
	ld	[%l0+Bp3],%f10
	fadds	%f10,%f8,%f8
	fmuls	%f4,%f8,%f8
	ld	[%l0+Bp2],%f12
	fadds	%f12,%f8,%f8
ptiny:				! for 0 <= x < 2**-9
	fmuls	%f4,%f8,%f8
	ld	[%l0+Bp1],%f10
	fadds	%f10,%f8,%f8
pvtiny: 			! for 0 <= x < 2**-13
	fmuls	%f6,%f8,%f8
	ba	exit
	fadds	%f4,%f8,%f0
less_1:
	cmp	%i0,%l1
	ble,a	nsmall
	ld	[%l0+Bn4],%f8
	sethi	%hi(0x3f7ff800),%l1
	cmp	%i0,%l1
	ble,a	ntiny
	ld	[%l0+Bn2],%f8
	ba	nvtiny
	ld	[%l0+Bn1],%f8
nsmall:				! for 1-0.078125 < x <= 0
	fmuls	%f4,%f8,%f8
	ld	[%l0+Bn3],%f10
	fadds	%f10,%f8,%f8
	fmuls	%f4,%f8,%f8
	ld	[%l0+Bn2],%f12
	fadds	%f12,%f8,%f8
ntiny:				! for -2**-9 < x <= 0
	fmuls	%f4,%f8,%f8
	ld	[%l0+Bn1],%f10
	fadds	%f10,%f8,%f8
nvtiny: 			! for -2**-13 < x <= 0
	fmuls	%f6,%f8,%f8
	ba	exit
	fadds	%f4,%f8,%f0
LARGE_N:	! recall %l5 is n
	sethi	%hi(0x20000),%l3! l3 = 0x20000
	add	%i0,%l3,%l2	! l2 = ix+0x20000
	srl	%l2,23,%o0
	sub	%o0,0x7f,%o0
	add	%l5,%o0,%l5	! n += ((ix+0x20000)>>23)-0x7f
	sethi	%hi(0x8f000000),%l1
	andn	%i0,%l1,%i0
	sethi	%hi(0x04000000),%l1
	or	%i0,%l1,%i0	! ix = (ix&0x70ffffff)|0x04000000
	st	%i0,[%fp-8]
	add	%i0,%l3,%l2	! l2 = ix+0x20000
	sethi	%hi(0xfffc0000),%l1
	ld	[%fp-8],%f0	! f0 = x
	and	%l2,%l1,%o0
	st	%o0,[%fp-16]
	ld	[%fp-16],%f2
	fsubs	%f0,%f2,%f4		! f4 = x-z
	st	%l5,[%fp-24]
	fadds	%f0,%f2,%f6		! f6 = x+z
	srl	%l2,16,%l2
	fdivs	%f4,%f6,%f6		! f6 = s = (x-z)/(x+z)
	ld	[%fp-24],%f16
	and	%l2,0x7c,%l2
	set	TBL,%o0
	fmuls	%f6,%f6,%f8		! f8 = z = s*s
	fitos	%f16,%f16		! f16 = (float)n;
	ld	[%l0+A1],%f10
	fmuls	%f10,%f8,%f10
	ld	[%l0+two],%f12
	fadds	%f12,%f10,%f10
	fmuls	%f6,%f10,%f10
	ld	[%l0+ln2lo],%f14
	fmuls	%f16,%f14,%f14
	fadds	%f10,%f14,%f14
	ld	[%o0+%l2],%f12
	fadds	%f12,%f14,%f14
	ld	[%l0+ln2hi],%f10
	fmuls	%f16,%f10,%f10
	ba	exit
	fadds	%f14,%f10,%f0
SMALL_N:
	sethi	%hi(0x20000),%o0
	sethi	%hi(0xfffc0000),%o1
	add	%i0,%o0,%o2
	and	%o2,%o1,%o2
	st	%o2,[%fp-8]
	ld	[%fp-8],%f2
	fsubs	%f0,%f2,%f4
	sethi	%hi(0x3dbe0000),%o0
	fadds	%f0,%f2,%f6
	sub	%i0,%o0,%o0
	fdivs	%f4,%f6,%f6		! f6 = s = (x-z)/(x+z)
	srl	%o0,16,%o0
	andn	%o0,0x3,%o0
	set	__tbl_r_log_lo,%o1
	set	__tbl_r_log_hi,%o2
	fmuls	%f6,%f6,%f8		! f8 = z = s*s
	ld	[%l0+A1],%f10
	fmuls	%f8,%f10,%f10
	ld	[%l0+two],%f12
	fadds	%f12,%f10,%f10
	fmuls	%f6,%f10,%f10
	ld	[%o1+%o0],%f4
	fadds	%f4,%f10,%f10
	ld	[%o2+%o0],%f2
	fadds	%f2,%f10,%f0
exit:	ret
	restore

	.seg	"data"
        .align 8
constant:
zero	= 0x0
	.word	0
one	= 0x4
	.word	0x3f800000
two	= 0x8
	.word	0x40000000
two24	= 0xc
	.word	0x4b800000	! 16777216 = 2^24
ln2hi	= 0x10
	.word	0x3f317200	!  6.931457519531250e-1
ln2lo	= 0x14
	.word	0x35bfbe8e	!  1.428606820286227e-6
Bp1	= 0x18
	.word	0xBEFFFFF4	! -4.999996375804686e-1
Bp2	= 0x1c
	.word	0x3EAAA4AC	!  3.332876052778868e-1
Bp3	= 0x20
	.word	0xBE7E2702	! -2.481956765240821e-1
Bp4	= 0x24
	.word	0x3E2F921D	!  1.714558145040048e-1
Bn1	= 0x28
	.word	0xBEFFFFEE	! -4.999994759413435e-1
Bn2	= 0x2c
	.word	0x3EAAB315	!  3.333975294998483e-1
Bn3	= 0x30
	.word	0xBE7D8329	! -2.475706542840484e-1
Bn4	= 0x34
	.word	0x3E7140AD	!  2.355982881354680e-1
A1	= 0x38
	.word	0x3F2AB03B	!  6.667515563167809e-1
TBL:
	.word	0x00000000,0x3cfc14d8,0x3d785186,0x3db78694,0x3df1383b
	.word	0x3e14aa98,0x3e2ff984,0x3e4a92d5,0x3e647fbe,0x3e7dc8c3
	.word	0x3e8b3ae5,0x3e974716,0x3ea30c5e,0x3eae8dee,0x3eb9cec0
	.word	0x3ec4d19c,0x3ecf991f,0x3eda27bc,0x3ee47fbe,0x3eeea350
	.word	0x3ef8947b,0x3f012a95,0x3f05f397,0x3f0aa61f,0x3f0f42fb
	.word	0x3f13caf1,0x3f183eba,0x3f1c9f07,0x3f20ec7f,0x3f2527c3
	.word	0x3f295169,0x3f2d6a02
