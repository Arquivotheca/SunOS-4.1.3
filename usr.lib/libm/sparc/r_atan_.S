
	.seg	"data"
	.asciz	"@(#)r_atan_.S 1.1 92/07/30 SMI"

#define LOCORE
#include <machine/asm_linkage.h>

!
! Copyright (c) 1989 by Sun Microsystems, Inc.
!
! r_atan_(x)
! Table look-up algorithm
! By K.C. Ng, March 21, 1989
!
! Algorithm. see ../R/r_atan_.c
!

! local variables
x 	= -0x8
tmp 	= -0x10

! local variable
! l0 = address of constant
! i0 = x
! i2 = |x|
	.global	__tbl_r_atan_hi,__tbl_r_atan_lo
	.seg	"text"
	ENTRY(r_atan_)
	save	%sp,-128,%sp
	ld	[%i0],%f0
	ld	[%i0],%i0
	set	constant,%l0
	set	0x80000000,%i3
	andn	%i0,%i3,%i2
	sethi	%hi(0x3f000000),%o0
	cmp	%i2,%o0			! compare |x| with 1/2
	bge	N1
	sethi	%hi(0x38800000),%o0		! 2**(-prec/2-2) = 2**-14
    ! |x| < 1/2
	cmp	%i2,%o0
	bge,a	1f
	fmuls	%f0,%f0,%f2
    ! x is tiny
	ld	[%l0+big],%f2
	ba	exit		
	fadds	%f0,%f2,%f2		! set inexact flag if x!= 0 
1:
	set	0x3c000000,%o0		
	cmp	%i2,%o0			! compare |x| with 2**-7
	fmuls	%f0,%f2,%f4		! f4 = x^3
	bl,a	2f
	ld	[%l0+p1],%f6
	set	0x3e000000,%o0
	cmp	%i2,%o0
	bl,a	1f
	ld	[%l0+p2],%f6
    ! for 1/8 <= x < 1/2, user poly3
	ld	[%l0+a5],%f10
	fmuls	%f2,%f10,%f10
	ld	[%l0+a4],%f6
	fadds	%f6,%f10,%f10
	fmuls	%f2,%f10,%f10
	ld	[%l0+a3],%f8
	fadds	%f8,%f10,%f10
	fmuls	%f2,%f10,%f10
	ld	[%l0+a2],%f6
	fadds	%f6,%f10,%f10
	fmuls	%f2,%f10,%f10
	ld	[%l0+a1],%f8
	fadds	%f8,%f10,%f10
	fmuls	%f4,%f10,%f10
	ba	exit
	fadds	%f0,%f10,%f0
1:
    ! 0.125 > |x| >=  2**-7 : x+(x*z)*(p1+z*p2)
	fmuls	%f2,%f6,%f6
	ld	[%l0+p1],%f8
	fadds	%f8,%f6,%f6		! f4 = p1+z*p2
2:
	fmuls	%f4,%f6,%f6
	ba	exit
	fadds	%f0,%f6,%f0		! x+(x*z)*(p1+...)
N1:	
	sethi	%hi(0x41000000),%o0
	cmp	%i2,%o0			! compare |x| with 8.0
	bl	N3
	fabss	%f0,%f0
    ! |x| >= 8.0
	set	0x42820000,%o0		! 65
	cmp	%i2,%o0
	bge	1f
	sethi	%hi(0x44800000),%o0	! 2**10 = 2**(prec/3+2)
    ! 8<=|x|<65 use poly1 on 1/x
	ld	[%l0+one],%f4
	fdivs	%f4,%f0,%f0		! f0 = r = 1/x
	fmuls	%f0,%f0,%f2
    ! y = r*(one+z*(p1+z*p2));
	ld	[%l0+p2],%f6
	fmuls	%f2,%f6,%f6
	ld	[%l0+p1],%f4
	fadds	%f4,%f6,%f6		! p1+z*(...)
	fmuls	%f2,%f6,%f6
	ld	[%l0+one],%f8
	fadds	%f8,%f6,%f6		! one+z*(p1+...)
	ld	[%l0+pio2lo],%f4
	fmuls	%f0,%f6,%f0		! r*(1+z*(p1+...))
	ba	N2
	fsubs	%f0,%f4,%f0
1:
	cmp	%i2,%o0			! compare |x| with 2**10
	bge	2f
	sethi	%hi(0x4c800000),%o0	! 2**26
    !  |x| < 2**(prec/3+2), use poly2 on 1/x */
	ld	[%l0+one],%f4
	fdivs	%f4,%f0,%f0		! f0 = r = 1/x
	fmuls	%f0,%f0,%f2
    ! y = r*(one+z*q1);
	ld	[%l0+q1],%f6
	fmuls	%f2,%f6,%f6
	ld	[%l0+one],%f4
	fadds	%f4,%f6,%f6
	ld	[%l0+pio2lo],%f4
	fmuls	%f0,%f6,%f0
	ba	N2
	fsubs	%f0,%f4,%f0
2:
	cmp	%i2,%o0			! compare |x| with 2**26
	bge	3f
	sethi	%hi(0x7f800000),%o0
    ! x <  2**(prec+2), set y = 1/x-pio2lo
	ld	[%l0+one],%f4
	fdivs	%f4,%f0,%f0		! f0 = r = 1/x
	ld	[%l0+pio2lo],%f2
	ba	N2
	fsubs	%f0,%f2,%f0
3:
	ld	[%l0+pio2lo],%f1
	cmp	%i2,%o0			! comapre |x| with inf
	ble,a	N2			! 2**(prec+2)<=x<=inf, y = -pio2lo;
	fnegs	%f1,%f0
	bg	exit			! x is NaN
	fsubs	%f0,%f0,%f0
N2:
	ld	[%l0+pio2hi],%f2
	tst	%i0
	bl,a	exit
	fsubs	%f0,%f2,%f0
	ba	exit
	fsubs	%f2,%f0,%f0
N3:
    ! now x is between 1/2 and 8
	set	0x40000,%o0
	add	%i2,%o0,%i4
	set	0x7ff80000,%o0
	and	%i4,%o0,%i4
	st	%i4,[%fp+tmp]
	ld	[%fp+tmp],%f2		! y
	tst	%i0
	bl,a	1f
	fsubs	%f2,%f0,%f4
	fsubs	%f0,%f2,%f4
1:
	cmp	%i2,%i4
	bne,a	2f
	fmuls	%f0,%f2,%f6		! x*y
    ! x==y, hence s=0
	set	0x3f000000,%o2
	sub	%i4,%o2,%i3
	srl	%i3,17,%i3		! i3 = j
	set	__tbl_r_atan_hi,%o0
	ld	[%o0+%i3],%f0
	set	__tbl_r_atan_lo,%o2
	ld	[%o2+%i3],%f2
	tst	%i0
	bg	exit
	fadds	%f0,%f2,%f0		! hi+lo
	fnegs	%f0,%f0
	ba	exit
	fsubs	%f0,%f2,%f0		! (-hi) - lo
2:
	ld	[%l0+one],%f8
	fadds	%f8,%f6,%f6		! 1+x*y
	set	0x3f000000,%o2
	fdivs	%f4,%f6,%f0		! s=+-(x-y)/(1+x*y)
	set	__tbl_r_atan_hi,%o0
	sub	%i4,%o2,%i3
	srl	%i3,17,%i3		! i3 = j
	set	__tbl_r_atan_lo,%o1
	fmuls	%f0,%f0,%f2		! f2 = s*s
	ld	[%l0+q1],%f8
	fmuls	%f2,%f8,%f8		! z*q1
	ld	[%l0+one],%f6
	fadds	%f6,%f8,%f8		! one+z*q1
	ld	[%o0+%i3],%f10		! f10 = hi
	fmuls	%f0,%f8,%f8		! f8  = p = s*(one+z*q1)
	ld	[%o1+%i3],%f12		! f12 = lo
	tst	%i0
	bl,a	3f
	fsubs	%f8,%f12,%f0
    ! x > 0
	fadds	%f8,%f12,%f0
	ba	exit
	fadds	%f0,%f10,%f0
3:
	fsubs	%f0,%f10,%f0
exit:
	ret
	restore
	
	.seg	"data"
	.align	4
constant:
big	= 0x0
	.word	0x7cf0bdc2
one	= 0x4
	.word	0x3f800000
p1	= 0x8
	.word	0xbeaaa8bc
p2	= 0xc
	.word	0x3e49a968
q1	= 0x10
	.word	0xbeaaa546
pio2hi	= 0x14
	.word	0x3fc90fdb
pio2lo	= 0x18
	.word	0xb33bbd2e
a1	= 0x1c
	.word	0xbeaaaa8a
a2	= 0x20
	.word	0x3e4cbdbe
a3	= 0x24
	.word	0xbe112825
a4	= 0x28
	.word	0x3dd020d0
a5	= 0x2c
	.word	0xbd526999
	.seg	"text"
