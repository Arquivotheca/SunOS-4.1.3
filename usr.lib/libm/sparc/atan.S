
	.seg	"data"
	.asciz	"@(#)atan.S 1.1 92/07/30 SMI"

#define LOCORE
#include <machine/asm_linkage.h>

!
! Copyright (c) 1989 by Sun Microsystems, Inc.
!
! atan(x)
! Table look-up algorithm
! By K.C. Ng, March 21, 1989
!
! Algorithm. See ../C/atan.c
!

! local variable using fp
x 	= -0x8
tmp 	= -0x10

! local variable
! l0 = address of constant
! i0 = x
! i1 = (low) x
! i2 = |x|
	.global	__tbl_atan_hi,__tbl_atan_lo
	.seg	"text"
	ENTRY(atan)
	save	%sp,-128,%sp
	set	0x80000000,%i3
	std	%i0,[%fp+x]
	set	constant,%l0
	andn	%i0,%i3,%i2
	set	0x3fc00000,%o0
	ldd	[%fp+x],%f0
	cmp	%i2,%o0
	bge,a	N1
	sethi	%hi(0x40200000),%o0
    ! |x| < 1/8
	set	0x3f500000,%o0		! 2**(-prec/6-2) = 2**-10
	cmp	%i2,%o0
	bge,a	N0
	fmuld	%f0,%f0,%f2		! f2 = z = x*x
    ! |x| < 2**-10
	set	0x3e300000,%o0		! 2**(-prec/2-2) = 2**-28
	cmp	%i2,%o0
	bge,a	1f
	fmuld	%f0,%f0,%f2
    ! x is tiny
	ldd	[%l0+big],%f2
	ba	exit		
	faddd	%f0,%f2,%f2		! set inexact flag if x!= 0 
1:
	set	0x3f100000,%o0
	cmp	%i2,%o0
	bl	2f
	fmuld	%f0,%f2,%f4		! f4 = x^3
    ! |x| >=  2**-14 : need more term
	ldd	[%l0+p2],%f6
	fmuld	%f2,%f6,%f6
	ldd	[%l0+p1],%f8
	faddd	%f8,%f6,%f6		! f4 = p1+z*p2
	fmuld	%f4,%f6,%f6
	ba	exit
	faddd	%f0,%f6,%f0		! x+(x*z)*(p1+...)
2:
	ldd	[%l0+p1],%f6
	fmuld	%f4,%f6,%f6
	ba	exit
	faddd	%f0,%f6,%f0		! x+(x*z)*(p1+...)
N0:
    ! return x+(x*z)*(p1+z*(p2+z*(p3+z*(p4+z*(p5+z*p6)))));
	ldd	[%l0+p6],%f6
	fmuld	%f2,%f6,%f6
	ldd	[%l0+p5],%f4
	faddd	%f4,%f6,%f6
	fmuld	%f2,%f6,%f6
	ldd	[%l0+p4],%f8
	faddd	%f8,%f6,%f6
	fmuld	%f2,%f6,%f6
	ldd	[%l0+p3],%f4
	faddd	%f4,%f6,%f6
	fmuld	%f2,%f6,%f6
	ldd	[%l0+p2],%f8
	faddd	%f8,%f6,%f6
	fmuld	%f2,%f6,%f6
	ldd	[%l0+p1],%f4
	faddd	%f4,%f6,%f6
	fmuld	%f2,%f0,%f8
	fmuld	%f8,%f6,%f6
	ba	exit
	faddd	%f0,%f6,%f0
N1:	
	cmp	%i2,%o0
	bl	N3
	fabss	%f0,%f0
    ! |x| >= 8.0
	set	0x40504000,%o0		! 65
	cmp	%i2,%o0
	bge	1f
	sethi	%hi(0x41200000),%o0	! 2**19 = 2**(prec/3+2)
    ! 8<=|x|<65 use poly1 on 1/x
	ldd	[%l0+one],%f4
	fdivd	%f4,%f0,%f0		! f0 = r = 1/x
	fmuld	%f0,%f0,%f2
    ! y = r*(one+z*(p1+z*(p2+z*(p3+z*(p4+z*(p5+z*p6))))));
	ldd	[%l0+p6],%f6
	fmuld	%f2,%f6,%f6
	ldd	[%l0+p5],%f4
	faddd	%f4,%f6,%f6
	fmuld	%f2,%f6,%f6
	ldd	[%l0+p4],%f8
	faddd	%f8,%f6,%f6
	fmuld	%f2,%f6,%f6
	ldd	[%l0+p3],%f4
	faddd	%f4,%f6,%f6
	fmuld	%f2,%f6,%f6
	ldd	[%l0+p2],%f8
	faddd	%f8,%f6,%f6
	fmuld	%f2,%f6,%f6
	ldd	[%l0+p1],%f4
	faddd	%f4,%f6,%f6		! p1+z*(...)
	fmuld	%f2,%f6,%f6
	ldd	[%l0+one],%f8
	faddd	%f8,%f6,%f6		! one+z*(p1+...)
	ldd	[%l0+pio2lo],%f4
	fmuld	%f0,%f6,%f0		! r*(1+z*(p1+...))
	ba	N2
	fsubd	%f0,%f4,%f0
1:
	cmp	%i2,%o0
	bge	2f
	sethi	%hi(0x43600000),%o0
    !  |x| < 2**(prec/3+2), use poly2 on 1/x */
	ldd	[%l0+one],%f4
	fdivd	%f4,%f0,%f0		! f0 = r = 1/x
	fmuld	%f0,%f0,%f2
    ! y = r*(one+z*(q1+z*(q2+z*q3)));
	ldd	[%l0+q3],%f6
	fmuld	%f2,%f6,%f6
	ldd	[%l0+q2],%f4
	faddd	%f4,%f6,%f6
	fmuld	%f2,%f6,%f6
	ldd	[%l0+q1],%f8
	faddd	%f8,%f6,%f6
	fmuld	%f2,%f6,%f6
	ldd	[%l0+one],%f4
	faddd	%f4,%f6,%f6
	ldd	[%l0+pio2lo],%f4
	fmuld	%f0,%f6,%f0
	ba	N2
	fsubd	%f0,%f4,%f0
2:
	cmp	%i2,%o0
	bge	3f
	sethi	%hi(0x7ff00000),%o0
    ! x <  2**(prec+2), set y = 1/x-pio2lo
	ldd	[%l0+one],%f4
	fdivd	%f4,%f0,%f0		! f0 = r = 1/x
	ldd	[%l0+pio2lo],%f2
	ba	N2
	fsubd	%f0,%f2,%f0
3:
	ldd	[%l0+pio2lo],%f0
	cmp	%i2,%o0
	bl	N2			! 2**(prec+2)<=x<inf, y = -pio2lo;
	fnegs	%f0,%f0
	sub	%i2,%o0,%o1
	orcc	%i1,%o1,%g0
	be	N2
	nop
	ldd	[%fp+x],%f0
	ba	exit			! x is NaN
	fsubd	%f0,%f0,%f0
N2:
	ldd	[%l0+pio2hi],%f2
	tst	%i0
	bl,a	exit
	fsubd	%f0,%f2,%f0
	ba	exit
	fsubd	%f2,%f0,%f0
N3:
    ! now x is between 1/8 and 8
	set	0x8000,%o0
	add	%i2,%o0,%i4
	set	0x7fff0000,%o0
	and	%i4,%o0,%i4
	mov	0,%i5
	std	%i4,[%fp+tmp]
	ldd	[%fp+tmp],%f2		! y
	fmuld	%f0,%f2,%f6		! x*y
	tst	%i0
	bl,a	1f
	fsubd	%f2,%f0,%f4
	fsubd	%f0,%f2,%f4
1:
	set	0x3fc00000,%o2
	ldd	[%l0+one],%f8
	faddd	%f8,%f6,%f6
	set	__tbl_atan_hi,%o0
	fdivd	%f4,%f6,%f0
	sub	%i4,%o2,%i3
	srl	%i3,13,%i3		! i3 = j
	set	__tbl_atan_lo,%o1
	cmp	%i2,%i4
	fmuld	%f0,%f0,%f2		! f2 = s^2
	be,a	2f
	ldd	[%l0+q1],%f8
	ldd	[%l0+q3],%f8
	fmuld	%f2,%f8,%f8
	ldd	[%l0+q2],%f6
	faddd	%f6,%f8,%f8
	fmuld	%f2,%f8,%f8
	ldd	[%l0+q1],%f4
	faddd	%f4,%f8,%f8		! f8 = q1+...
2:
	fmuld	%f2,%f8,%f8		! z*(q1+...)
	ldd	[%l0+one],%f6
	faddd	%f6,%f8,%f8
	ldd	[%o0+%i3],%f10		! f10 = hi
	fmuld	%f0,%f8,%f8		! f8  = p = s*(one+z*(q1+...))
	ldd	[%o1+%i3],%f12		! f12 = lo
	tst	%i0
	bl,a	3f
	fsubd	%f8,%f12,%f0
    ! x > 0
	faddd	%f8,%f12,%f0
	ba	exit
	faddd	%f0,%f10,%f0
3:
	fsubd	%f0,%f10,%f0
exit:
	ret
	restore
	

	.seg	"data"
	.align	8
constant:
one	= 0x0
	.word	0x3ff00000,0x0
p1	= 0x8
	.word	0xbfd55555,0x555554cc
p2	= 0x10
	.word	0x3fc99999,0x99945f2a
p3	= 0x18
	.word	0xbfc24924,0x89cdb42c
p4	= 0x20
	.word	0x3fbc71ba,0x726d04fe
p5	= 0x28
	.word	0xbfb74107,0x98893f6e
p6	= 0x30
	.word	0x3fb2cc1f,0xf0dd3da2
q1	= 0x38
	.word	0xbfd55555,0x55553b5d
q2	= 0x40
	.word	0x3fc99999,0x882347ff
q3	= 0x48
	.word	0xbfc24760,0x59d3fc13
pio2hi	= 0x50
	.word	0x3ff921fb,0x54442d18
pio2lo	= 0x58
	.word	0x3c91a626,0x33145c07
big	= 0x60
	.word	0x479E17B8,0x4357691B
	.seg	"text"
