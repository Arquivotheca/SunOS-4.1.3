 
        .seg    "data"
        .asciz  "@(#)r_fmod_.S 1.1 92/07/30 SMI"

#define LOCORE
#include <machine/asm_linkage.h>

! Copyright (c) 1988 by Sun Microsystems, Inc.
!
! single r_fmod_(x,y) 
! Algorithm see libm/R/r_fmod_.c
! Note: inner loop is optimized, may look different from r_fmod_.c.
!
!
! local variable using fp
x	= -0x8
y	= -0x10

! local register usage
! i0 = x
! i2 = y
! o0 = x0
! o2 = y0
! o4 = z0
! l0	= 
! l1    = 0x80000000
! l2	= 0x7f800000
! l3 	= 0x00800000
! l4    = ia
! l5	= ib
! l6	= n
! l7	= k = n&3
! i4,i5	...scratch register
!
	.global	_fmod
	.seg	"text"
	ENTRY(r_fmod_)
	save	%sp,-128,%sp
	set	0x80000000,%l1
	ld	[%i0],%i0
	ld	[%i1],%i2
	andn	%i0,%l1,%o0		! o0 = |x|
	andn	%i2,%l1,%o2		! o2 = |y|
	set	0x7f800000,%l2
    ! purge off exception values
	cmp	%o0,%l2
	bge	rfmod_exception		! x is inf or NaN, goto exception
	orcc	%o2,%g0,%g0
	be	rfmod_exception		! y is zero, goto exception
	cmp	%o2,%l2
	bl	T1
	nop
	bg	rfmod_exception		! y is NaN, goto exception
	nop
    ! y is inf, x is finite , simply return x
T0:
	st	%i0,[%fp+x]
	ba	rfmod_return
	ld	[%fp+x],%f0
T1:
	cmp	%o0,%o2
	bg	T2
	nop
	bl	T0
	nop
	be	rfmod_zero
	nop
T2:
	set	0x0c800000,%i4
	cmp	%o2,%i4
	bl	rfmod_tiny
	and	%i0,%l2,%l4
	and	%i2,%l2,%l5
	sub	%l4,%l5,%l6
	srl	%l6,23,%l6
	set	0x00800000,%l3
	andn	%o0,%l2,%o0
	andn	%o2,%l2,%o2
	or	%l3,%o0,%o0
	or	%l3,%o2,%o2
	and	%l6,3,%l7		! l7 = k = n&3
	srl	%l6,2,%l6		! n = n>>2, unroll loop 4 times
    ! now do fix point fmod
	tst	%l6
	ble	T3_end
	subcc	%o0,%o2,%o4		! z0 = x0-y0
T3_loop:
	bl,a	1f
	add	%o0,%o0,%o0		! x0 = x0+x0
	bne,a	1f
	addcc	%o4,%o4,%o0		! x0 = z0+z0
	ba	rfmod_zero
	nop
1: 	subcc	%o0,%o2,%o4		! z0 = x0-y0
	bl,a	1f
	add	%o0,%o0,%o0		! x0 = x0+x0
	bne,a	1f
	addcc	%o4,%o4,%o0		! x0 = z0+z0
	ba	rfmod_zero
	nop
1: 	subcc	%o0,%o2,%o4		! z0 = x0-y0
	bl,a	1f
	add	%o0,%o0,%o0		! x0 = x0+x0
	bne,a	1f
	addcc	%o4,%o4,%o0		! x0 = z0+z0
	ba	rfmod_zero
	nop
1: 	subcc	%o0,%o2,%o4		! z0 = x0-y0
	bl,a	1f
	add	%o0,%o0,%o0		! x0 = x0+x0
	bne,a	1f
	addcc	%o4,%o4,%o0		! x0 = z0+z0
	ba	rfmod_zero
	nop
1: 	subcc	%l6,1,%l6
	bg,a	T3_loop
	subcc	%o0,%o2,%o4		! z0 = x0-y0
T3_end:
    ! now sub 1 from k
	subcc	%l7,1,%l7
	bl	T4
	subcc	%o0,%o2,%o4		! z0 = x0-y0
	bl,a	1f
	add	%o0,%o0,%o0		! x0 = x0+x0
	bne,a	1f
	addcc	%o4,%o4,%o0		! x0 = z0+z0
	ba	rfmod_zero
	nop
1: 	subcc	%l7,1,%l7
	bl	T4
	subcc	%o0,%o2,%o4		! z0 = x0-y0
	bl,a	1f
	add	%o0,%o0,%o0		! x0 = x0+x0
	bne,a	1f
	addcc	%o4,%o4,%o0		! x0 = z0+z0
	ba	rfmod_zero
	nop
1: 	subcc	%l7,1,%l7
	bl	T4
	subcc	%o0,%o2,%o4		! z0 = x0-y0
	bl,a	1f
	add	%o0,%o0,%o0		! x0 = x0+x0
	bne,a	1f
	addcc	%o4,%o4,%o0		! x0 = z0+z0
	ba	rfmod_zero
	nop
1:
T4:
	subcc	%o0,%o2,%o4
	be	rfmod_zero
	nop
	bl	2f
	nop
	mov	%o4,%o0
2:

    ! convert back to floating point
	andcc	%o0,%l3,%g0
	bne	T6
	nop
T5:
	add	%o0,%o0,%o0
	andcc	%o0,%l3,%g0
	be	T5
	sub	%l5,%l3,%l5
T6:
	and	%i0,%l1,%i4		! i4  = sign(x)
	or	%l5,%i4,%l5		! ib |= sign(x)
	andn	%o0,%l2,%o0		! clear exponent field of x
	or	%l5,%o0,%o0
	st	%o0,[%fp+x]
	ld	[%fp+x],%f0
rfmod_return:
	ret
	restore

rfmod_zero:
	and	%i0,%l1,%o4
	st	%o4,[%fp+x]
	ba	rfmod_return
	ld	[%fp+x],%f0

rfmod_tiny:
    ! |y| < 2^-126, using double fmod to avoid rescaling
	st      %i0,[%fp+x]
        ld      [%fp+x],%f0
        fstod   %f0,%f0
        st      %i2,[%fp+y]
        ld      [%fp+y],%f2
        fstod   %f2,%f2
        std     %f0,[%fp+x]
        std     %f2,[%fp+y]
        ldd     [%fp+x],%o0
        call    _fmod
        ldd     [%fp+y],%o2
        ba      rfmod_return
        fdtos   %f0,%f0

rfmod_exception:
	st	%i0,[%fp+x]
	st	%i2,[%fp+y]
	ld	[%fp+x],%f0
	ld	[%fp+y],%f2
	fmuls	%f0,%f2,%f0
	fdivs	%f0,%f0,%f0
	ba	rfmod_return
	nop
