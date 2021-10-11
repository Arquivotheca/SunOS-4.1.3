
	.seg	"data"
	.asciz	"@(#)rint.S 1.1 92/07/30 SMI"

#define LOCORE
#include <machine/asm_linkage.h>

! Copyright (c) 1988 by Sun Microsystems, Inc.
!
!double	aint(x)		truncate x to integral.
!double	anint(x)	truncate sign(x)*(|x|+0.5) to integal. 
!double ceil(x)		round x (round-toward- +inf) to integral
!double floor(x)	round x (round-toward- -inf) to integral
!int	irint(x)	round x to integer according to the prevailing RD.
!int	nint(x)		truncate sign(x)*(|x|+0.5) to integer. 
!double	rint(x)		round x to integral according to the prevailing RD.
!All function except nint(x) preserve sign, including signed zero.
!Code by K.C. Ng, Nov 5, 1986
!Revised on May 10, 1988.

	.seg	"data"
	.align	8
constant:
two52	= 0x00
	.word	0x43300000,0x0		!  2**52
mtwo52	= 0x08
	.word	0xc3300000,0x0		!  -2**52
half	= 0x10
	.word	0x3fe00000,0x0		!  0.5
mhalf	= 0x18
	.word	0xbfe00000,0x0		!  -0.5
zero	= 0x20
	.word	0x0,0x0			!  0.0
mzero	= 0x28
	.word	0x80000000,0x0		!  -0.0
one	= 0x30
	.word	0x3ff00000,0x0		!  1.0
!
! variable using fp 
x	= -0x8
tmp	= -0x10

! input i0=high x , i1 = low x
	.seg	"text"

	ENTRY(aint)
	save	%sp,-128,%sp
	set	constant,%l0
	sethi	%hi(0x80000000),%l1
	andn	%i0,%l1,%i2
	sethi	%hi(0x43300000),%l4
	cmp	%i2,%l4
	bl,a	1f
	ldd	[%l0+two52],%f2
    ! return x if x>=2^52
	std	%i0,[%fp+x]
	ldd	[%fp+x],%f0
	ldd	[%l0+zero],%f2
	faddd	%f0,%f2,%f0
	ba	aint_return
	nop
1:
	st	%i2,[%fp+x]
	st	%i1,[%fp+x+4]
	ldd	[%fp+x],%f4		! f4 = w = |x|
	faddd	%f4,%f2,%f0		! f0 = w+L, L=2^52
	fsubd	%f0,%f2,%f0		! f0 = t = (w+L) - L
	and	%i0,%l1,%l5		! l5 = sign bit of x
	fcmpd	%f0,%f4
	nop
	nop
	fble	2f
	nop
	ldd	[%l0+one],%f4
	fsubd	%f0,%f4,%f0		! subtract one if t > w
2:
	st	%f0,[%fp+x]
	ld	[%fp+x],%l4
	andn	%l4,%l1,%l4
	or	%l5,%l4,%l4
	st	%l4,[%fp+x]
	ld	[%fp+x],%f0
aint_return:
	ret
	restore


	ENTRY(anint)
	save	%sp,-128,%sp
	set	constant,%l0
	sethi	%hi(0x80000000),%l1
	andn	%i0,%l1,%i2
	sethi	%hi(0x43300000),%l4
	cmp	%i2,%l4
	bl,a	1f
	ldd	[%l0+two52],%f2
    ! return x if x>=2^52
	std	%i0,[%fp+x]
	ldd	[%fp+x],%f0
	ldd	[%l0+zero],%f2
	faddd	%f0,%f2,%f0
	ba	aint_return
	nop
1:
	st	%i2,[%fp+x]
	st	%i1,[%fp+x+4]
	ldd	[%fp+x],%f4		! f4 = w = |x|
	faddd	%f4,%f2,%f0		! f0 = w+L, L=2^52
	fsubd	%f0,%f2,%f0		! f0 = t = (w+L) - L
	and	%i0,%l1,%l5		! l5 = sign bit of x
	fcmpd	%f0,%f4
	nop
	nop
	fbe	2f
	nop
	fsubd	%f0,%f4,%f6		! f6 = z = t - w
	ldd	[%l0+half],%f8
	ldd	[%l0+one],%f4
	fcmpd	%f6,%f8			! 
	nop
	nop
	fbg,a	2f
	fsubd	%f0,%f4,%f0		! subtract one if z > 0.5
	ldd	[%l0+mhalf],%f8
	fcmpd	%f6,%f8
	nop
	nop
	fble,a	2f
	faddd	%f0,%f4,%f0
2:
	st	%f0,[%fp+x]
	ld	[%fp+x],%l4
	andn	%l4,%l1,%l4
	or	%l5,%l4,%l4
	st	%l4,[%fp+x]
	ld	[%fp+x],%f0
anint_return:
	ret
	restore


	ENTRY(ceil)
	save	%sp,-128,%sp
	set	constant,%l0
	sethi	%hi(0x80000000),%l1
	andn	%i0,%l1,%i2
	sethi	%hi(0x43300000),%l4
	cmp	%i2,%l4
	bl,a	1f
	tst	%i0
    ! return x if x>=2^52
	std	%i0,[%fp+x]
	ldd	[%fp+x],%f0
	ldd	[%l0+zero],%f2
	faddd	%f0,%f2,%f0
	ba	aint_return
	nop
1:
	bge,a	1f
	ldd	[%l0+two52],%f2
	ldd	[%l0+mtwo52],%f2
1:
	std	%i0,[%fp+x]
	ldd	[%fp+x],%f4		! f4 = w = x
	faddd	%f4,%f2,%f0		! f0 = w+L, L=sign(x)*2^52
	fsubd	%f0,%f2,%f0		! f0 = t = (w+L) - L
	and	%i0,%l1,%l5		! l5 = sign bit of x
	fcmpd	%f0,%f4
	nop
	nop
	fbge	2f
	nop
	ldd	[%l0+one],%f4
	faddd	%f0,%f4,%f0		! add one if t < x
2:
	st	%f0,[%fp+x]
	ld	[%fp+x],%l4
	andn	%l4,%l1,%l4
	or	%l5,%l4,%l4
	st	%l4,[%fp+x]
	ld	[%fp+x],%f0
ceil_return:
	ret
	restore

	ENTRY(floor)
	save	%sp,-128,%sp
	set	constant,%l0
	sethi	%hi(0x80000000),%l1
	andn	%i0,%l1,%i2
	sethi	%hi(0x43300000),%l4
	cmp	%i2,%l4
	bl,a	1f
	tst	%i0
    ! return x if x>=2^52
	std	%i0,[%fp+x]
	ldd	[%fp+x],%f0
	ldd	[%l0+zero],%f2
	faddd	%f0,%f2,%f0
	ba	aint_return
	nop
1:
	bge,a	1f
	ldd	[%l0+two52],%f2
	ldd	[%l0+mtwo52],%f2
1:
	std	%i0,[%fp+x]
	ldd	[%fp+x],%f4		! f4 = w = x
	faddd	%f4,%f2,%f0		! f0 = w+L, L=sign(x)*2^52
	fsubd	%f0,%f2,%f0		! f0 = t = (w+L) - L
	and	%i0,%l1,%l5		! l5 = sign bit of x
	fcmpd	%f0,%f4
	nop
	nop
	fble	2f
	nop
	ldd	[%l0+one],%f4
	fsubd	%f0,%f4,%f0		! subtract one if x < t
2:
	st	%f0,[%fp+x]
	ld	[%fp+x],%l4
	andn	%l4,%l1,%l4
	or	%l5,%l4,%l4
	st	%l4,[%fp+x]
	ld	[%fp+x],%f0
floor_return:
	ret
	restore


	ENTRY(irint)
	save	%sp,-128,%sp
	mov	%i0,%o0
	call	_rint
	mov	%i1,%o1
	fdtoi	%f0,%f0
	st	%f0,[%fp+tmp]
	ld	[%fp+tmp],%i0
	ret
	restore


	ENTRY(nint)
	save	%sp,-128,%sp
	mov	%i0,%o0
	call	_anint
	mov	%i1,%o1
	fdtoi	%f0,%f0
	st	%f0,[%fp+tmp]
	ld	[%fp+tmp],%i0
	ret
	restore


	ENTRY(rint)
	save	%sp,-128,%sp
	set	constant,%l0
	std	%i0,[%fp+x]
	sethi	%hi(0x80000000),%l4
	andn	%i0,%l4,%i2		! i2 = high|x|
	ldd	[%fp+x],%f0
	sethi	%hi(0x43300000),%l5
	cmp	%i2,%l5
	bl	1f
	tst	%i0
	ldd	[%l0+zero],%f2
	faddd	%f0,%f2,%f0		! return x+0.0 (raise flag if x is 
	ba	rint_return		! sNaN)
	nop
1:
	bge,a	2f
	ldd	[%l0+two52],%f2
	ldd	[%l0+mtwo52],%f2
2:
	faddd	%f0,%f2,%f0		! x + L rounded
	fcmpd	%f0,%f2
	nop
	nop
	fbne	3f
	nop
    ! return copysign(0.0,x)
	tst	%i0
	bge,a	rint_return
	ldd	[%l0+zero],%f0
	ba	rint_return
	ldd	[%l0+mzero],%f0
3:
    ! simply return (x+L)rounded - L
	fsubd	%f0,%f2,%f0
rint_return:
	ret
	restore

