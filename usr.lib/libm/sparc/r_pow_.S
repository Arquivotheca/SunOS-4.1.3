 
        .seg    "data"
        .asciz  "@(#)r_pow_.S 1.1 92/07/30 SMI"

#define LOCORE
#include <machine/asm_linkage.h>

! Copyright (c) 1989 by Sun Microsystems, Inc.
!
! single precision r_pow_(x)
! Algorithm see libm/R/r_pow_.c.
!                                       -- K.C. NG (kcng@kcng)
!

! local variable using fp index
tmp     = -0x8
x	= -0x10

! local usage of register
! l0 = address of constant
! i0 = ix
! i1 = iy
! i2 = jx = |ix|
! i3 = jy = |iy|
! i4 = 0x7f800000
! i5 = 0x80000000
! l1 = k
! l2 = iw
! f0 = x
! f1 = y

	.seg	"text"
	.global	_r_sqrt_
	ENTRY(r_pow_)
	save	%sp,-120,%sp
	ld	[%i0],%f0
	ld	[%i1],%f1
	ld	[%i0],%i0
	ld	[%i1],%i1
	set	constant,%l0
	sethi	%hi(0x80000000),%i5
	andn	%i0,%i5,%i2		! jx = |x|
	andncc	%i1,%i5,%i3		! jy = |y|
	be,a	exit
	ld	[%l0+one],%f0		! x**0 = 1
	sethi	%hi(0x7f800000),%i4
	sub	%i4,%i2,%o0
	sub	%i4,%i3,%o1
	orcc	%o0,%o1,%g0
	bneg,a	exit
	fadds	%f0,%f1,%f0		! NaN: return x+y
	tst	%i0
	bpos,a	next1
	andncc	%i3,%i4,%g0		! jy&(~inf) == 0?
    ! negative x, determine k and iw for checking whether y is an odd int
	sethi	%hi(0x4b800000),%o0
	cmp	%i3,%o0
	bl,a	1f
	fstoi	%f1,%f2
    ! |y| >= 2**24, y must be an even integer, set k=0 and iw = iy
	mov	0,%l1
	ba	2f
	mov	%i1,%l2
1:
	fitos	%f2,%f3
	st	%f2,[%fp+tmp]
	ld	[%fp+tmp],%l1
	st	%f3,[%fp+tmp]
	ld	[%fp+tmp],%l2
2:
	andncc	%i3,%i4,%g0		! jy&(~inf) == 0?
next1:
	bne,a	next2
	andncc	%i2,%i4,%g0		! jx&(~inf) == 0?
    ! special value of y 
	cmp	%i3,%i4
	bne	2f
	sethi	%hi(0x3f800000),%o0
    ! y is +-inf 
	cmp	%i2,%o0
	be,a	exit
	fsubs	%f1,%f1,%f0		!  1**inf = NaN = inf - inf
	bg	1f
	tst	%i1
    ! (|x|<1)**+-inf
	bneg,a	exit
	fnegs	%f1,%f0
	ba	exit
	ld	[%l0+zero],%f0
1:
    ! (|x|>1)**+-inf
	bpos,a	exit
	fmovs	%f1,%f0
	ba	exit
	ld	[%l0+zero],%f0
2:
	cmp	%i3,%o0
	bne,a	2f
	sethi	%hi(0x40000000),%o0
    ! y is +-1
	tst	%i1
	bg	exit			! x**1 = x
	ld	[%l0+one],%f2
	ba	exit
	fdivs	%f2,%f0,%f0		! x**-1 = 1/x
2:
	cmp	%i1,%o0
	be,a	exit
	fmuls	%f0,%f0,%f0
	sethi	%hi(0x3f000000),%o0
	cmp	%i1,%o0
	bne,a	next2
	andncc	%i2,%i4,%g0		! jx&(~inf) == 0?
    ! y is 0.5
	tst	%i2
	be,a	next2
	andncc	%i2,%i4,%g0		! jx&(~inf) == 0?
	cmp	%i2,%i4
	be,a	next2
	andncc	%i2,%i4,%g0		! jx&(~inf) == 0?
	st	%i0,[%fp+x]
	call	_r_sqrt_
	add	%fp,x,%o0
	ba	exit
	nop
next2:
	bne,a	next3
	tst	%i0
    ! special value of x 
	tst	%i2
	be	1f
	cmp	%i2,%i4
	be	1f
	sethi	%hi(0x3f800000),%o0
	cmp	%i2,%o0
	bne,a	next3
	tst	%i0
1:
    ! x is +-0,+-inf,+-1
	fabss	%f0,%f0
	tst	%i1
	bpos	1f
	tst	%i0
	ld	[%l0+one],%f2
	fdivs	%f2,%f0,%f0
1:
	bpos	exit
	cmp	%i2,%o0
	bne	1f
	cmp	%i1,%l2			! iy ? iw
	be	1f
	ld	[%l0+zero],%f2
	ba	exit
	fdivs	%f2,%f2,%f0		! (-1)**non-int is NaN
1:
	cmp	%i1,%l2
	bne	exit
	andcc	%l1,1,%g0
	bne,a	exit
	fnegs	%f0,%f0
	ba	exit
	nop
next3:
	bpos,a	next4
	fabss	%f0,%f0
	cmp	%i1,%l2
	be,a	next4
	fabss	%f0,%f0
	ld	[%l0+zero],%f2
	ba	exit
	fdivs	%f2,%f2,%f0		! (x<0)**non-int is NaN 
next4:
    ! compute exp(y*log(|x|)
	fstod	%f0,%f16
	std	%f16,[%l0+x]
	ld	[%l0+x],%o3
! o3 = i
! o4 = n
	set	0x4000,%o0
	add	%o0,%o3,%o3
	srl	%o3,20,%o1
	sub	%o1,0x3ff,%o4
	set	0xffff8000,%o0
	and	%o3,%o0,%o0
	st	%o0,[%fp+tmp]
	st	%g0,[%fp+tmp+4]
	ldd	[%fp+tmp],%f2		! f2 = dz
	fsubd	%f16,%f2,%f4		! f4 = dx - dz
	faddd	%f16,%f2,%f6		! f6 = dx + dz
	fdivd	%f4,%f6,%f4		! f4 = ds = (dx-dz)/(dx+dz)
	srl	%o3,12,%o0
	and	%o0,0xf8,%o3
	fmuld	%f4,%f4,%f6		! f6 = dz = ds*ds
	ldd	[%l0+A1],%f8
	fmuld	%f6,%f8,%f8		! f8 = dz*A1
	ldd	[%l0+A0],%f10
	faddd	%f10,%f8,%f8		! f8 = A0+dz*A1
	set	TBL,%o1
	fmuld	%f4,%f8,%f8
	ldd	[%o1+%o3],%f10
	faddd	%f10,%f8,%f8
	ldd	[%l0+invln2],%f12
	fmuld	%f12,%f8,%f8
	fstod	%f1,%f18
	tst	%o4
	be,a	1f
	fmuld	%f18,%f8,%f8		! f8 = dz
	st	%o4,[%fp+tmp]
	ld	[%fp+tmp],%f10
	fitod	%f10,%f10
	faddd	%f10,%f8,%f8
	fmuld	%f18,%f8,%f8		! f8 = dz
1:
	std	%f8,[%fp+tmp]
	ld	[%fp+tmp],%o0
	andn	%o0,%i5,%o1
	set	0x40640000,%o2
	cmp	%o1,%o2
	bl	2f
	tst	%o0
    ! |z| >= 160.0, either overflow or underflow
	bpos,a	1f
	ldd	[%l0+dhuge],%f20
	ldd	[%l0+dtiny],%f20
1:
	ba	next5
	fdtos	%f20,%f0			
2:
	ldd	[%l0+d32],%f6
	fmuld	%f6,%f8,%f6
	ldd	[%l0+dhalf],%f4
	bpos,a	1f
	faddd	%f4,%f6,%f6
	fsubd	%f6,%f4,%f6
1:
	fdtoi	%f6,%f4
	fitod	%f4,%f6
	st	%f4,[%fp+tmp]
	ldd	[%l0+d1_32],%f2
	fmuld	%f6,%f2,%f2
	ld	[%fp+tmp],%o0
	sra	%o0,5,%o1		! o1 = m = n>>5
	fsubd	%f8,%f2,%f2
	ldd	[%l0+ln2],%f0
	fmuld	%f0,%f2,%f2		! f2 = dy
	and	%o0,0x1f,%o3
	sll	%o3,3,%o3		! o3 = i
	ldd	[%l0+t1],%f6
	fmuld	%f2,%f6,%f6		! dy*t1
	ldd	[%l0+done],%f4
	fsubd	%f4,%f6,%f6
	fmuld	%f2,%f6,%f6
	ldd	[%l0+t0],%f0
	fsubd	%f6,%f0,%f0		! f0 = dy*(done-dy*t1)-t0)
	ldd	[%l0+dtwo],%f4
	fmuld	%f2,%f4,%f4		! f4 = 2*dy
	set	S,%o2
	fdivd	%f4,%f0,%f0
	ldd	[%l0+done],%f6
	fsubd	%f6,%f0,%f0
	ldd	[%o2+%o3],%f4
	fmuld	%f4,%f0,%f0
	tst	%o1
	be,a	next5
	fdtos	%f0,%f0
	std	%f0,[%fp+tmp]
	ld	[%fp+tmp],%o0
	sll	%o1,20,%o1
	add	%o1,%o0,%o0
	st	%o0,[%fp+tmp]
	ld	[%fp+tmp],%f0
	fdtos	%f0,%f0
next5:
	tst	%i0
	bpos	exit
	andcc	%l1,1,%g0
	bne,a	exit
	fnegs	%f0,%f0
exit:
	ret
	restore

	.seg	"data"
	.align	8
constant:
ln2	= 0x0
	.word	0x3fe62e42,0xfefa39ef
invln2	= 0x8
	.word	0x3ff71547,0x652b82fe
dtwo	= 0x10
	.word	0x40000000,0x0
A0	= 0x18
	.word   0x3fffffff,0xfff332fc
A1	= 0x20
	.word   0x3fe55588,0x891ad374
t0	= 0x28
	.word	0x40000000,0x2a06
t1	= 0x30
	.word	0x3fc55552,0x999d79e7
done	= 0x38
	.word	0x3ff00000,0
dhuge	= 0x40
	.double	0r1e100
dtiny	= 0x48
	.double 0r1e-100
one	= 0x50
	.word	0x3f800000
zero	= 0x54
	.word	0x0
d32	= 0x58
	.double	0r32.0
d1_32	= 0x60
	.double 0r0.03125
dhalf	= 0x68
	.double	0r0.5
S:
	.word	0x3ff00000,0x0
	.word	0x3ff059b0,0xd3158574
	.word	0x3ff0b558,0x6cf9890f
	.word	0x3ff11301,0xd0125b51
	.word	0x3ff172b8,0x3c7d517b
	.word	0x3ff1d487,0x3168b9aa
	.word	0x3ff2387a,0x6e756238
	.word	0x3ff29e9d,0xf51fdee1
	.word	0x3ff306fe,0xa31b715
	.word	0x3ff371a7,0x373aa9cb
	.word	0x3ff3dea6,0x4c123422
	.word	0x3ff44e08,0x6061892d
	.word	0x3ff4bfda,0xd5362a27
	.word	0x3ff5342b,0x569d4f82
	.word	0x3ff5ab07,0xdd485429
	.word	0x3ff6247e,0xb03a5585
	.word	0x3ff6a09e,0x667f3bcd
	.word	0x3ff71f75,0xe8ec5f74
	.word	0x3ff7a114,0x73eb0187
	.word	0x3ff82589,0x994cce13
	.word	0x3ff8ace5,0x422aa0db
	.word	0x3ff93737,0xb0cdc5e5
	.word	0x3ff9c491,0x82a3f090
	.word	0x3ffa5503,0xb23e255d
	.word	0x3ffae89f,0x995ad3ad
	.word	0x3ffb7f76,0xf2fb5e47
	.word	0x3ffc199b,0xdd85529c
	.word	0x3ffcb720,0xdcef9069
	.word	0x3ffd5818,0xdcfba487
	.word	0x3ffdfc97,0x337b9b5f
	.word	0x3ffea4af,0xa2a490da
	.word	0x3fff5076,0x5b6e4540
TBL:
	.word	0x0,0x0
	.word	0x3f9f829b,0xe783300
	.word	0x3faf0a30,0xc01162a6
	.word	0x3fb6f0d2,0x8ae56b4c
	.word	0x3fbe2707,0x6e2af2e6
	.word	0x3fc29552,0xf81ff523
	.word	0x3fc5ff30,0x70a793d4
	.word	0x3fc9525a,0x9cf456b4
	.word	0x3fcc8ff7,0xc79a9a22
	.word	0x3fcfb918,0x6d5e3e2b
	.word	0x3fd1675c,0xababa60e
	.word	0x3fd2e8e2,0xbae11d31
	.word	0x3fd4618b,0xc21c5ec2
	.word	0x3fd5d1bd,0xbf5809ca
	.word	0x3fd739d7,0xf6bbd007
	.word	0x3fd89a33,0x86c1425b
	.word	0x3fd9f323,0xecbf984c
	.word	0x3fdb44f7,0x7bcc8f63
	.word	0x3fdc8ff7,0xc79a9a22
	.word	0x3fddd46a,0x4c1c4a1
	.word	0x3fdf128f,0x5faf06ed
	.word	0x3fe02552,0xa5a5d0ff
	.word	0x3fe0be72,0xe4252a83
	.word	0x3fe154c3,0xd2f4d5ea
	.word	0x3fe1e85f,0x5e7040d0
	.word	0x3fe2795e,0x1289b11b
	.word	0x3fe307d7,0x334f10be
	.word	0x3fe393e0,0xd3562a1a
	.word	0x3fe41d8f,0xe84672ae
	.word	0x3fe4a4f8,0x5db03ebb
	.word	0x3fe52a2d,0x265bc5ab
	.word	0x3fe5ad40,0x4c359f2d
