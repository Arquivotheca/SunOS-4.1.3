
	.data
	.asciz	"@(#)log.S 1.1 92/07/30 SMI"

#define LOCORE
#include <machine/asm_linkage.h>

! Copyright (c) 1989 by Sun Microsystems, Inc.
!
! single log(x) float *x;
! Code by K.C. Ng (based on libm/C/log.c)
! April 8, 1987
! Revised on May 9, 1988
! Rewritten on Jan 17, 1989
!
! use l5 for n
	.global	_SVID_libm_err,__tbl_log_hi,__tbl_log_lo
	.seg	"text"
	ENTRY(log)
	save	%sp,-128,%sp
	std	%i0,[%fp-8]
	set	constant,%l0
	mov	0,%l5			! l5 = n <- 0
	ldd	[%fp-8],%f0		! f0 = x
	set	0x3fee0f8e,%l1
	cmp	%i0,%l1
	sethi	%hi(0x3ff10842),%l2
	bg,a	1f
	or	%l2,%lo(0x3ff10842),%l2
	sethi	%hi(0x3fb80000),%l1
	cmp	%i0,%l1
	bge	SMALL_N
	sethi	%hi(0x00100000),%l1
	cmp	%i0,%l1
	bge	LARGE_N
	sethi	%hi(0x80000000),%l1
	andn	%i0,%l1,%o0
	orcc	%i1,%o0,%g0
	be	LOG_0
	tst	%i0
	bl	LOG_NEG
	ldd	[%l0+two52],%f4
    ! subnormal x
	fmuld	%f0,%f4,%f0
	mov	-52,%l5
	std	%f0,[%fp-8]
	ba	LARGE_N
	ldd	[%fp-8],%i0
LOG_0:
	mov	%i0,%o0
	mov	%i1,%o1
	mov	%i0,%o2
	mov	%i1,%o3
	call	_SVID_libm_err
	mov	16,%o4
	ba	exit
	nop
! If no SIVD_libm_err, use the following
!	ldd	[%l0+one],%f2
!	ldd	[%l0+zero],%f4
!	fnegs	%f2,%f2
!	ba	exit
!	fdivd	%f2,%f4,%f0
LOG_NEG:
	sethi	%hi(0xfff00000),%l1
	cmp	%i0,%l1
	bge,a	exit
	fsubd	%f0,%f0,%f0	! return x-x if x is -inf of NaN
	ldd	[%l0+zero],%f2	
! log(-ve)
	mov	%i0,%o0
	mov	%i1,%o1
	mov	%i0,%o2
	mov	%i1,%o3
	call	_SVID_libm_err
	mov	17,%o4
	ba	exit
	nop
! If no SIVD_libm_err, use the following
!	ba	exit
!	fdivd	%f2,%f2,%f0
1:
	cmp	%i0,%l2
	bl,a	near1
	ldd	[%l0+one],%f2
	sethi	%hi(0x40378000),%l1
	cmp	%i0,%l1
	bl	SMALL_N
	sethi	%hi(0x7ff00000),%l1
	cmp	%i0,%l1
	bl	LARGE_N
	nop
	ba	exit
	faddd	%f0,%f0,%f0	! x now is +Inf or NaN
near1:
! Now 31/33<x<33/31 ( 0x3fee0f83 <= x < 0x3ff10842 )
	fsubd	%f0,%f2,%f4	! f4 = f = x - 1.0
	add	%i0,4,%o0
	andn	%o0,7,%o0
	sethi	%hi(0x3ff00000),%l1
	cmp	%o0,%l1
	bne,a	small
	ldd	[%l0+two],%f2	! now f2=2
	sub	%i0,%l1,%o1
	orcc	%o1,%i1,%g0
	be,a	exit
	ld	[%l0+zero],%f0
	fmuld	%f4,%f4,%f6	! f6 = z = f*f
	ldd	[%l0+one_third],%f2
	fmuld	%f2,%f4,%f8
	ldd	[%l0+half],%f10
	fsubd	%f10,%f8,%f8
	fmuld	%f6,%f8,%f8
	ba	exit
	fsubd	%f4,%f8,%f0
small:
	faddd	%f2,%f4,%f8
	set	0x3fefe000,%o0
	fdivd	%f4,%f8,%f8	! f8 = s
	set	0x3ff00fff,%o1
	sub	%i0,%o0,%o2
	sub	%o1,%i0,%o3
	orcc	%o2,%o3,%o4
	fmuld	%f8,%f8,%f6	! f6 = z = s*s
	bge,a	quick_cran
	ldd	[%l0+B2],%f10
	ldd	[%l0+B4],%f10
	fmuld	%f6,%f10,%f10
	ldd	[%l0+B3],%f14
	faddd	%f14,%f10,%f10
	fmuld	%f6,%f10,%f10
	ldd	[%l0+B2],%f12
	faddd	%f12,%f10,%f10
quick_cran:
	fmuld	%f6,%f10,%f10
	ldd	[%l0+B1],%f14
	faddd	%f14,%f10,%f10
	fmuld	%f6,%f10,%f10
	fsubd	%f4,%f10,%f10
	fmuld	%f8,%f10,%f10
	ba	exit
	fsubd	%f4,%f10,%f0
LARGE_N:	! recall %l5 is n
	sethi	%hi(0x4000),%l3	! l3 = 0x4000
	add	%i0,%l3,%l2	! l2 = ix+0x4000
	srl	%l2,20,%o0
	sub	%o0,0x3ff,%o0
	add	%l5,%o0,%l5	! n += ((ix+0x4000)>>20)-0x3ff
	sethi	%hi(0xfff00000),%l1
	andn	%i0,%l1,%i0
	sethi	%hi(0x3ff00000),%l1
	or	%i0,%l1,%i0	! ix = (ix&0x000fffff)|0x3ff00000
	st	%i0,[%fp-8]
	add	%i0,%l3,%l2	! l2 = ix+0x4000
	sethi	%hi(0xffff8000),%l1
	ld	[%fp-8],%f0	! f0 = x scaled
	and	%l2,%l1,%o0
	mov	0,%o1
	std	%o0,[%fp-16]
	ldd	[%fp-16],%f2	! z
	fsubd	%f0,%f2,%f4		! f4 = x-z
	st	%l5,[%fp-24]
	faddd	%f0,%f2,%f6		! f6 = x+z
	srl	%l2,12,%l2
	fdivd	%f4,%f6,%f6		! f6 = s = (x-z)/(x+z)
	ld	[%fp-24],%f16
	and	%l2,0xf8,%l2
	set	TBL,%o0
	fmuld	%f6,%f6,%f8		! f8 = z = s*s
	fitod	%f16,%f16		! f16 = (double)n;
	ldd	[%l0+A3],%f10
	fmuld	%f8,%f10,%f10
	ldd	[%l0+A2],%f12
	faddd	%f12,%f10,%f10
	fmuld	%f8,%f10,%f10
	ldd	[%l0+A1],%f14
	faddd	%f14,%f10,%f10
	fmuld	%f8,%f10,%f10
	ldd	[%l0+two],%f12
	faddd	%f12,%f10,%f10
	fmuld	%f6,%f10,%f10
	ldd	[%l0+ln2lo],%f14
	fmuld	%f16,%f14,%f14
	faddd	%f10,%f14,%f14
	ldd	[%o0+%l2],%f12
	faddd	%f12,%f14,%f14
	ldd	[%l0+ln2hi],%f10
	fmuld	%f16,%f10,%f10
	ba	exit
	faddd	%f14,%f10,%f0
SMALL_N:
	sethi	%hi(0x4000),%o0
	sethi	%hi(0xffff8000),%o1
	add	%i0,%o0,%o2
	and	%o2,%o1,%o2
	mov	0,%o3
	std	%o2,[%fp-16]
	ldd	[%fp-16],%f2
	fsubd	%f0,%f2,%f4
	sethi	%hi(0x3fb7c000),%o0
	faddd	%f0,%f2,%f6
	sub	%i0,%o0,%o0
	fdivd	%f4,%f6,%f6		! f6 = s = (x-z)/(x+z)
	srl	%o0,12,%o0
	andn	%o0,0x7,%o0
	set	__tbl_log_lo,%o1
	set	__tbl_log_hi,%o2
	fmuld	%f6,%f6,%f8		! f8 = z = s*s
	ldd	[%l0+A3],%f10
	fmuld	%f8,%f10,%f10
	ldd	[%l0+A2],%f12
	faddd	%f12,%f10,%f10
	fmuld	%f8,%f10,%f10
	ldd	[%l0+A1],%f14
	faddd	%f14,%f10,%f10
	fmuld	%f8,%f10,%f10
	ldd	[%l0+two],%f12
	faddd	%f12,%f10,%f10
	fmuld	%f6,%f10,%f10
	ldd	[%o1+%o0],%f4
	faddd	%f4,%f10,%f10
	ldd	[%o2+%o0],%f2
	faddd	%f2,%f10,%f0
exit:	ret
	restore

	.seg	"data"
        .align 8
constant:
zero	= 0x0
	.word	0,0
one_third = 0x8
	.word	0x3fd55555,0x55555555
half	= 0x10
	.word	0x3fe00000,0
one	= 0x18
	.word	0x3ff00000,0
two	= 0x20
	.word	0x40000000,0
two52	= 0x28
	.word	0x43300000,0x0
ln2hi	= 0x30
	.word	0x3fe62e42,0xfee00000
ln2lo	= 0x38
	.word	0x3dea39ef,0x35793c76
A1	= 0x40
	.word	0x3fe55555,0x555571da
A2	= 0x48
	.word	0x3fd99999,0x8702be3a
A3	= 0x50
	.word	0x3fd24af7,0x3f4569b1
B1	= 0x58
	.word	0x3fe55555,0x555554ed
B2	= 0x60
	.word	0x3fd99999,0x99b93aad
B3	= 0x68
	.word	0x3fd24923,0x144fabc5
B4	= 0x70
	.word	0x3fcc7ffa,0x5aafbc63
TBL:
	.word	0x00000000,0x00000000,0x3f9f829b,0x0e783300
	.word	0x3faf0a30,0xc01162a6,0x3fb6f0d2,0x8ae56b4c
	.word	0x3fbe2707,0x6e2af2e6,0x3fc29552,0xf81ff523
	.word	0x3fc5ff30,0x70a793d4,0x3fc9525a,0x9cf456b4
	.word	0x3fcc8ff7,0xc79a9a22,0x3fcfb918,0x6d5e3e2b
	.word	0x3fd1675c,0xababa60e,0x3fd2e8e2,0xbae11d31
	.word	0x3fd4618b,0xc21c5ec2,0x3fd5d1bd,0xbf5809ca
	.word	0x3fd739d7,0xf6bbd007,0x3fd89a33,0x86c1425b
	.word	0x3fd9f323,0xecbf984c,0x3fdb44f7,0x7bcc8f63
	.word	0x3fdc8ff7,0xc79a9a22,0x3fddd46a,0x04c1c4a1
	.word	0x3fdf128f,0x5faf06ed,0x3fe02552,0xa5a5d0ff
	.word	0x3fe0be72,0xe4252a83,0x3fe154c3,0xd2f4d5ea
	.word	0x3fe1e85f,0x5e7040d0,0x3fe2795e,0x1289b11b
	.word	0x3fe307d7,0x334f10be,0x3fe393e0,0xd3562a1a
	.word	0x3fe41d8f,0xe84672ae,0x3fe4a4f8,0x5db03ebb
	.word	0x3fe52a2d,0x265bc5ab,0x3fe5ad40,0x4c359f2d
	.seg	"text"
