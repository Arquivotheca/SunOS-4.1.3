
 	.seg	"data"
	.asciz	"@(#)r_trig_.S 1.1 92/07/30 SMI"

#define LOCORE
#include <machine/asm_linkage.h>

! Copyright (c) 1988 by Sun Microsystems, Inc.
!
! single r_sin_(x), r_cos_(x), r_tan_(x), r_sincos_(x,s,c)
! IEEE Single precision trig functions on Sunrise
!
! Algorithm: see libm/R/r_trig_.c
!
	.seg	"text"
	ENTRY(r_sin_)
	ba	r_trig
	mov	0,%o4
	ENTRY(r_cos_)
	ba	r_trig
	mov	1,%o4
	ENTRY(r_tan_)
	ba	r_trig
	mov	2,%o4
	ENTRY(r_sincos_)
	ba	r_trig
	mov	3,%o4

! r_trig()

	.seg	"data"
	.align	8
constant:
fone 	= 0x0
	.single	0r1.0
f1_2	= 0x4
	.single	0r0.5
f1_6	= 0x8
	.single	0r0.16666666666666666
f1_3	= 0xc
	.single	0r0.33333333333333333
fbig	= 0x10
	.single	0r1.0e20
finvpio2= 0x14
	.single	0r0.636619772367581343
half	= 0x18
	.double	0r0.5
pio2	= 0x20
	.word   0x3ff921fb,0x54442d18
pio2_1	= 0x28
	.word   0x3ff921fb,0x54400000
pio2_t	= 0x30
	.word   0x3dd0b461,0x1a626331
pio2_t66= 0x38
	.word   0x3dd0b461,0x1a600000
pio2_t53= 0x40
	.word   0x3dd0b460,0x0
one	= 0x48
	.double	0r1.0
S0	= 0x50
	.double	0r1.66666552424430847168e-01
S1	= 0x58
	.double	0r-8.33219196647405624390e-03
S2	= 0x60
	.double	0r1.95187909412197768688e-04
C0	= 0x68
	.double	0r4.16666455566883087158e-02
C1	= 0x70
	.double	0r-1.38873036485165357590e-03
C2	= 0x78
	.double	0r2.44309903791872784495e-05

! temporary variable using fp pointer

x	= -0x8
y	= -0x10
t	= -0x18
z	= -0x20
n	= -0x28

! local register variable
! i0 = |x| 			input *x, update to |x|
! i1 = *s			input from sincos
! i2 = *c			input from sincos
! i3
! i4 = k			input 0-sin,1-cos,2-tan,3-sincos
! i5 = x			local
! l0 = address of constant	local
! l1
! l2 = n
!
! f0 = x			local
! f2 = y
! f12= y*y			local
	.seg	"text"
r_trig:	
	save	%sp,-0xc0,%sp
	ld	[%i0],%f0		! make f0 = x
	sethi	%hi(0x80000000),%l5
	ld	[%i0],%i5		! make i5 = x
	set	constant,%l0		! l0 = address of constant
	andn	%i5,%l5,%i0		! i0 = |x|

    ! if(t<0.0078125) then ...
	set	0x3c000000,%l5
	cmp	%i0,%l5
	bge	T2
	mov	0,%l2			! set n = 0

    !     if(t<tiny) then ...
	set	0x3727c5ac,%l5
	cmp	%i0,%l5
	bge	1f
	cmp	%i4,1
	ld	[%l0+fbig],%f2
	fadds	%f0,%f2,%f2		! create inexact flag
	cmp	%i4,3
	bne	2f
	nop
	ld	[%l0+fone],%f4		! k=3
	st	%f0,[%i1]
	ba	Treturn
	st	%f4,[%i2]
2:
	cmp	%i4,1			
	bne	Treturn			! k=0,2 sin(x)=tan(x)=x
	nop
	ba	Treturn
	ld	[%l0+fone],%f0		! k=1	cos(x)=1
1:
	fmuls	%f0,%f0,%f12		! f12 = z = x*x;

    !     Now tiny <= |x| <  2**-7
	bg	2f
	nop
	be	3f
	nop
	ld	[%l0+f1_6],%f2		! k = 0
	fmuls	%f0,%f2,%f2
	fmuls	%f12,%f2,%f2
	ba	Treturn
	fsubs	%f0,%f2,%f0
3:
	ld	[%l0+f1_2],%f2		! k = 1
	fmuls	%f2,%f12,%f10
	ld	[%l0+fone],%f0
	ba	Treturn
	fsubs	%f0,%f10,%f0
2:
	cmp	%i4,3
	be	4f
	nop
	ld	[%l0+f1_3],%f2		! k = 2
	fmuls	%f0,%f2,%f2
	fmuls	%f12,%f2,%f2
	ba	Treturn
	fadds	%f0,%f2,%f0
4:
	ld	[%l0+f1_2],%f4		! k = 3
	fmuls	%f12,%f4,%f4		! z*0.5
	ld	[%l0+fone],%f6
	fsubs	%f6,%f4,%f4		! 1-z*0.5
	ld	[%l0+f1_6],%f8
	fmuls	%f0,%f8,%f8		! x*1/6
	fmuls	%f12,%f8,%f8
	fsubs	%f0,%f8,%f8		! x - x*z*1/6
	st	%f4,[%i2]
	ba	Treturn
	st	%f8,[%i1]
T2:
	fstod	%f0,%f2			! f2 = y = (double) x

    ! if (t>pio4) then ...
	set	0x3f490fdb,%l5
	cmp	%i0,%l5
	ble	T3
	sethi	%hi(0x3fc90fdb),%l5
    ! 	if (t<pio2) then ...
	or	%l5,%lo(0x3fc90fdb),%l5
	cmp	%i0,%l5
	bg	2f
	tst	%i5
	bl	1f
	ldd	[%l0+pio2],%f4
	fsubd	%f2,%f4,%f2
	ba	T3
	mov	1,%l2			! n = 1
1:
	faddd	%f2,%f4,%f2
	ba	T3
	mov	-1,%l2			! n = -1
2:
    !   if (t<medium) then ...
	set	0x49c90fd8,%l5
	cmp	%i0,%l5
	bg	4f
	ld	[%l0+finvpio2],%f4
	fmuls	%f0,%f4,%f4
	tst	%i5
	bge	1f
	ld	[%l0+f1_2],%f6
	fsubs	%f4,%f6,%f4	
	ba	2f
	nop
1:
	fadds	%f6,%f4,%f4
2:
	fstoi	%f4,%f8
	ldd	[%l0+pio2],%f6
	set	0x01600000,%o0
	fitod	%f8,%f4			! f4 = w = (double) n
	st	%f8,[%fp+t]
	set	0x7ff00000,%l4
	fmuld	%f4,%f6,%f6		! f6 = w*pio2
	ld	[%fp+t],%l2		! set n
	st	%f4,[%fp+n]
	fsubd	%f2,%f6,%f6		! f6 = z = y - w*pio2
	ld	[%fp+n],%l3
	and	%l3,%l4,%l3
	std	%f6,[%fp+z]
	ld	[%fp+z],%l5
	and	%l5,%l4,%l5
	sub	%l3,%l5,%l5
	cmp	%l5,%o0
	ble,a	T3
	ldd	[%fp+z],%f2
	ldd	[%l0+pio2_1],%f6
	fmuld	%f4,%f6,%f6
	set	_fp_pi,%l4
	ld	[%l4],%l4
	fsubd	%f2,%f6,%f6		! z - w*pio2_1
	cmp	%l4,1
	be,a	2f
	ldd	[%l0+pio2_t66],%f8
	bg,a	2f
	ldd	[%l0+pio2_t53],%f8
	ldd	[%l0+pio2_t],%f8
2:
	fmuld	%f4,%f8,%f8		! w*pio2_t..
	ba	T3
	fsubd	%f6,%f8,%f2
4:
    ! if x is not finite return NaN
	sethi	%hi(0x7f800000),%l5
	cmp	%i0,%l5
	bl	5f
	nop
	fsubs	%f0,%f0,%f0
	cmp	%i4,3
	bne	Treturn
	nop
	st	%f0,[%i1]
	ba	Treturn
	st	%f0,[%i2]
5:
    ! huge argument reducion
	std	%f2,[%fp+y]
	ld	[%fp+y],%o0
	ld	[%fp+y+4],%o1
	call	r_argred
	add	%fp,y,%o2
	mov	%o0,%l2			! n = r_argred
	ldd	[%fp+y],%f2
T3:
	fmuld	%f2,%f2,%f4		! f4 = z = y*y

    !  if (k < 2)  sin, cos
	cmp	%i4,2
	bge	3f
	nop
	add	%l2,%i4,%l2		! n += k
	andcc	%l2,1,%g0
	bne,a	1f
	ldd	[%l0+C2],%f6		
	ldd	[%l0+S2],%f6		! sin
	fmuld	%f4,%f6,%f6
	ldd	[%l0+S1],%f8
	faddd	%f8,%f6,%f6
	fmuld	%f4,%f6,%f6
	ldd	[%l0+S0],%f8
	faddd	%f8,%f6,%f6
	fmuld	%f4,%f6,%f6
	ldd	[%l0+one],%f8
	fsubd	%f8,%f6,%f6
	ba	2f
	fmuld	%f2,%f6,%f6
1:
					! cos
	fmuld	%f4,%f6,%f6
	ldd	[%l0+C1],%f8
	faddd	%f8,%f6,%f6
	fmuld	%f4,%f6,%f6
	ldd	[%l0+C0],%f8
	faddd	%f8,%f6,%f6
	fmuld	%f4,%f6,%f6
	ldd	[%l0+half],%f8
	fsubd	%f8,%f6,%f6
	fmuld	%f4,%f6,%f6
	ldd	[%l0+one],%f8
	fsubd	%f8,%f6,%f6
2:
	andcc	%l2,2,%g0
	be	Treturn
	fdtos	%f6,%f0
	ba	Treturn
	fnegs	%f0,%f0
3:
    ! tan, sincos
	ldd	[%l0+C2],%f6		! cos
	fmuld	%f4,%f6,%f6
	ldd	[%l0+C1],%f8
	faddd	%f8,%f6,%f6
	fmuld	%f4,%f6,%f6
	ldd	[%l0+C0],%f8
	faddd	%f8,%f6,%f6
	fmuld	%f4,%f6,%f6
	ldd	[%l0+half],%f8
	fsubd	%f8,%f6,%f6
	fmuld	%f4,%f6,%f6
	ldd	[%l0+one],%f8
	fsubd	%f8,%f6,%f12		! f12 = cc
	ldd	[%l0+S2],%f6		! sin
	fmuld	%f4,%f6,%f6
	ldd	[%l0+S1],%f8
	faddd	%f8,%f6,%f6
	fmuld	%f4,%f6,%f6
	ldd	[%l0+S0],%f8
	faddd	%f8,%f6,%f6
	fmuld	%f4,%f6,%f6
	ldd	[%l0+one],%f8
	fsubd	%f8,%f6,%f6
	fmuld	%f2,%f6,%f10		! f10 = ss
	cmp	%i4,3
	bne	4f
	nop
	fdtos	%f12,%f1		! f1 = (float) cc
	andcc	%l2,3,%l2
	bne	1f
	fdtos	%f10,%f0		! f0 = (float) ss
	st	%f1,[%i2]
	ba	Treturn
	st	%f0,[%i1]
1:
	cmp	%l2,2
	be	1f
	nop
	bg	2f
	nop
	st	%f1,[%i1]
	fnegs	%f0,%f0
	ba	Treturn
	st	%f0,[%i2]
1:
	fnegs	%f1,%f1
	fnegs	%f0,%f0
	st	%f1,[%i2]
	ba	Treturn
	st	%f0,[%i1]
2:
	fnegs	%f1,%f1
	st	%f0,[%i2]
	ba	Treturn
	st	%f1,[%i1]
4:
    ! tan
	andcc	%l2,1,%g0
	bne	1f
	nop
	fdivd	%f10,%f12,%f0		!  ss/cc
	ba	Treturn
	fdtos	%f0,%f0
1:
	fnegs	%f12,%f12
	fdivd	%f12,%f10,%f0		! -cc/ss
	fdtos	%f0,%f0
Treturn:
	ret
	restore

!
! r_argred()
!
	.seg	"text"			! [internal]
	.proc	4
r_argred:
	save	%sp,-176,%sp
	st	%i0,[%fp+68]
	st	%i1,[%fp+72]
	sethi	%hi(_fp_pi),%o1
	ld	[%o1+%lo(_fp_pi)],%o1
	sethi	%hi(_fv_66),%o0
	tst	%o1
	bne	L77003
	add	%o0,%lo(_fv_66),%i3
	sethi	%hi(_fv_inf),%o2
	add	%o2,%lo(_fv_inf),%i3
L77003:
	sethi	%hi(_fp_pi),%o3
	ld	[%o3+%lo(_fp_pi)],%o3
	cmp	%o3,2
	bne,a	LY8
	ld	[%fp+68],%f30
	sethi	%hi(_fv_53),%o4
	add	%o4,%lo(_fv_53),%i3
	ld	[%fp+68],%f30
LY8:					! [internal]
	ld	[%fp+72],%f31
	std	%f30,[%sp+LP185+8]
	ld	[%sp+LP185+8],%o0
	ld	[%sp+LP185+12],%o1
	st	%f30,[%sp+LP185]
	st	%f31,[%sp+LP185+4]
	call	_signbit,2
	nop
	ld	[%sp+LP185],%f30
	ld	[%sp+LP185+4],%f31
	std	%f30,[%sp+LP185+8]
	mov	%o0,%l6
	ld	[%sp+LP185+8],%o0
	ld	[%sp+LP185+12],%o1
	st	%f30,[%sp+LP185]
	call	_fabs,2
	nop
	fmovs	%f1,%f31
	fmovs	%f0,%f30
	std	%f30,[%sp+LP185+8]
	ld	[%sp+LP185+8],%o0
	ld	[%sp+LP185+12],%o1
	st	%f30,[%sp+LP185]
	st	%f31,[%sp+LP185+4]
	call	_ilogb,2
	nop
	ld	[%sp+LP185],%f30
	ld	[%sp+LP185+4],%f31
	mov	%o0,%i4
	sub	%i4,26,%o0
	call	.div,2
	mov	24,%o1
	mov	%o0,%i5
	tst	%i5
	bge,a	LY7
	add	%i4,60,%o0
	mov	0,%i5
	add	%i4,60,%o0
LY7:					! [internal]
	call	.div,2
	mov	24,%o1
	sub	%o0,%i5,%i1
	sll	%i5,3,%i5
	mov	0,%i4
	cmp	%i4,%i1
	add	%i5,%i3,%i5
	add	%fp,-64,%i0
	bg	L77010
	add	%g0,%i0,%i3
LY1:					! [internal]
	ld	[%i5],%f0
	ld	[%i5+4],%f1
	fmuld	%f0,%f30,%f2
	inc	%i4
	cmp	%i4,%i1
	inc	8,%i5
	st	%f2,[%i3]
	st	%f3,[%i3+4]
	ble	LY1
	inc	8,%i3
L77010:
	ldd	[%fp-64],%f6
	sethi	%hi(L2000000),%l0	! [internal]
	ldd	[%l0+%lo(L2000000)],%f4
	fmuld	%f6,%f4,%f8
	sethi	%hi(L2000001),%l1	! [internal]
	st	%f8,[%sp+LP185+8]
	ld	[%sp+LP185+8],%o0
	st	%f9,[%sp+LP185+8]
	ld	[%sp+LP185+8],%o1
	st	%f30,[%sp+LP185]
	st	%f31,[%sp+LP185+4]
	call	_aint,2
	nop
	ldd	[%l1+%lo(L2000001)],%f10
	fmuld	%f10,%f0,%f12
	ldd	[%fp-64],%f14
	ldd	[%fp-56],%f18
	fsubd	%f14,%f12,%f14
	std	%f14,[%fp-64]
	ldd	[%l0+%lo(L2000000)],%f16
	fmuld	%f18,%f16,%f20
	st	%f20,[%sp+LP185+8]
	ld	[%sp+LP185+8],%o0
	st	%f21,[%sp+LP185+8]
	call	_aint,2
	ld	[%sp+LP185+8],%o1
	ldd	[%l1+%lo(L2000001)],%f22
	fmuld	%f22,%f0,%f24
	ldd	[%fp-56],%f26
	fsubd	%f26,%f24,%f26
	ldd	[%fp-48],%f0
	std	%f26,[%fp-56]
	ldd	[%l0+%lo(L2000000)],%f28
	fmuld	%f0,%f28,%f2
	fdtoi	%f2,%f3
	st	%f3,[%sp+LP185+8]
	ld	[%sp+LP185+8],%i4
	tst	%i4
	be,a	LY6
	sll	%i1,3,%o4
	sll	%i4,3,%i4
	st	%i4,[%sp+LP185+8]
	ld	[%sp+LP185+8],%f4
	fitod	%f4,%f6
	ldd	[%fp-48],%f8
	fsubd	%f8,%f6,%f8
	std	%f8,[%fp-48]
	sll	%i1,3,%o4
LY6:					! [internal]
	add	%o4,%i0,%o4
	ld	[%o4],%o7
	ld	[%o4+4],%o4
	st	%o7,[%sp+LP185+8]
	st	%o4,[%sp+LP185+12]
	ldd	[%sp+LP185+8],%f30
	sub	%i1,1,%i4
	sll	%i4,3,%i5
	tst	%i4
	bl	L77015
	add	%i5,%i0,%i5
LY2:					! [internal]
	ld	[%i5],%f10
	ld	[%i5+4],%f11
	faddd	%f30,%f10,%f30
	deccc	%i4
	bpos	LY2
	dec	8,%i5
L77015:
	fdtoi	%f30,%f11
	sethi	%hi(L2000002),%l3
	st	%f11,[%sp+LP185+8]
	ld	[%sp+LP185+8],%i3
	st	%i3,[%sp+LP185+8]
	ld	[%sp+LP185+8],%f12
	fitod	%f12,%f14
	ldd	[%l3+%lo(L2000002)],%f18
	fsubd	%f30,%f14,%f16
	fmovs	%f17,%f31
	fmovs	%f16,%f30
	fcmped	%f30,%f18
	nop				! [internal]
	fbule,a	LY5
	std	%f30,[%sp+LP185+8]
	sethi	%hi(L2000003),%l4
	ldd	[%l4+%lo(L2000003)],%f20
	fsubd	%f30,%f20,%f30
	inc	%i3
	std	%f30,[%sp+LP185+8]
LY5:					! [internal]
	ld	[%sp+LP185+8],%o0
	ld	[%sp+LP185+12],%o1
	st	%f30,[%sp+LP185]
	st	%f31,[%sp+LP185+4]
	call	_fabs,2
	sethi	%hi(L2000004),%l5
	ld	[%sp+LP185],%f30
	ld	[%sp+LP185+4],%f31
	ldd	[%l5+%lo(L2000004)],%f22
	fcmped	%f0,%f22
	nop				! [internal]
	fbuge,a	LY4
	sethi	%hi(_pio2),%o1
	st	%i3,[%sp+LP185+8]
	ld	[%sp+LP185+8],%f23
	fitod	%f23,%f24
	mov	1,%i4
	cmp	%i4,%i1
	mov	8,%i5
	add	%i5,%i0,%i5
	ldd	[%fp-64],%f26
	fsubd	%f26,%f24,%f28
	fmovs	%f29,%f31
	bg	L77022
	fmovs	%f28,%f30
LY3:					! [internal]
	ld	[%i5],%f0
	ld	[%i5+4],%f1
	faddd	%f30,%f0,%f30
	inc	%i4
	cmp	%i4,%i1
	ble	LY3
	inc	8,%i5
L77022:
	sethi	%hi(_pio2),%o1
LY4:					! [internal]
	ldd	[%o1+%lo(_pio2)],%f2
	fmuld	%f30,%f2,%f4
	tst	%l6
	st	%f4,[%i2]
	be	L77024
	st	%f5,[%i2+4]
	ld	[%i2],%f6
	fnegs	%f6,%f6
	ld	[%i2+4],%f7
	st	%f6,[%i2]
	st	%f7,[%i2+4]
	sub	%g0,%i3,%i3
L77024:
	and	%i3,7,%i0
	ret
	restore
LP185	=	96
	.seg	"data"			! [internal]
_sccsid:
	.half	0x255a
	.half	0x2525
	.half	0x4d25
	.half	0x2025
	.half	0x4925
	.half	0x2025
	.half	0x4525
	.half	0x2053
	.half	0x4d49
	.half	0
	.align	8
_pio2:
	.word	0x3ff921fb
	.word	0x54442d18
	.align	8
_fv_53:
	.word	0x3fe45f30
	.word	0x60000000
	.word	0x3e5b9391
	.word	0
	.word	0x3cb6f3c4
	.word	0
	.word	0x3b6d4d36
	.word	-0x60000000
	.word	0x39ea7c26
	.word	0x20000000
	.word	0x38583874
	.word	-0x80000000
	.word	0x36eaf305
	.word	0x40000000
	.word	0x3568063e
	.word	-0x60000000
	.align	8
_fv_66:
	.word	0x3fe45f30
	.word	0x60000000
	.word	0x3e5b9391
	.word	0
	.word	0x3cb52a00
	.word	0
	.word	0x3b18af10
	.word	0
	.word	0x39ab44ec
	.word	0
	.word	0x3867da98
	.word	0x20000000
	.word	0x36efbf20
	.word	-0x60000000
	.word	0x3557356e
	.word	-0x80000000
	.align	8
_fv_inf:
	.word	0x3fe45f30
	.word	0x60000000
	.word	0x3e5b9391
	.word	0
	.word	0x3cb529fc
	.word	0
	.word	0x3b43abe8
	.word	-0x80000000
	.word	0x39eea69b
	.word	-0x60000000
	.word	0x38681b6c
	.word	0x40000000
	.word	0x36e2b327
	.word	-0x80000000
	.word	0x3550e410
	.word	0x40000000
	.align	8
L2000000:
	.word	0x3fc00000
	.word	0
	.align	8
L2000001:
	.word	0x40200000
	.word	0
	.align	8
L2000002:
	.word	0x3fe00000
	.word	0
	.align	8
L2000003:
	.word	0x3ff00000
	.word	0
	.align	8
L2000004:
	.word	0x3ec00000
	.word	0
