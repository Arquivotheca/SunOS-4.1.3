
 
        .seg    "data"
        .asciz  "@(#)fmod.S 1.1 92/07/30 SMI"

#define LOCORE
#include <machine/asm_linkage.h>

! Copyright (c) 1988 by Sun Microsystems, Inc.
!
! double fmod(x,y) 
! Algorithm see libm/C/fmod.c
! Note: inner loop is optimized, may look different from fmod.c.
!
	.seg	"data"
	.align	8
constant:
two110	= 0x0
	.word	0x46d00000,0x0
twom110	= 0x8
	.word	0x39100000,0x0

! local variable using fp
x	= -0x8
y	= -0x10

! local register usage
! i0 = high x  i1 = low  x
! i2 = high y  i3 = low  y
! o0,o1 = x0,x1
! o2,o3 = y0,y1
! o4,o5 = z0,z1
! l0	= constant
! l1    = 0x80000000
! l2	= 0x7ff00000
! l3 	= 0x00100000
! l4    = ia
! l5	= ib
! l6	= n
! l7	= k = n&3
! i4,i5	...scratch register
!
	.seg	"text"
	ENTRY(fmod)
fmod_in:	! label to avoid multi-profiling
	save	%sp,-128,%sp
	set	0x80000000,%l1
	andn	%i0,%l1,%o0		! o0 = |x|
	andn	%i2,%l1,%o2		! o2 = |y|
	set	0x7ff00000,%l2
    ! purge off exception values
	cmp	%o0,%l2
	bge	fmod_exception		! x is inf or NaN, goto exception
	orcc	%o2,%i3,%g0
	be	fmod_exception		! y is zero, goto exception
	cmp	%o2,%l2
	bl	T1
	nop
	bg	fmod_exception		! y is NaN, goto exception
	tst	%i3
	bne	fmod_exception		! y is NaN, goto exception
	nop
    ! y is inf, x is finite , simply return x
T0:
	std	%i0,[%fp+x]
	ba	fmod_return
	ldd	[%fp+x],%f0
T1:
	cmp	%o0,%o2
	bg	T2
	nop
	bl	T0
	cmp	%i1,%i3
	blu	T0
	nop
	be	fmod_zero
	nop
T2:
	set	0x03600000,%i4
	cmp	%o2,%i4
	bl	fmod_tiny
	and	%i0,%l2,%l4
	and	%i2,%l2,%l5
	sub	%l4,%l5,%l6
	srl	%l6,20,%l6
	set	0x00100000,%l3
	andn	%o0,%l2,%o0
	andn	%o2,%l2,%o2
	or	%l3,%o0,%o0
	or	%l3,%o2,%o2
	and	%l6,3,%l7		! l7 = k = n&3
	srl	%l6,2,%l6		! n = n>>2, unroll loop 4 times
    ! now do fix point fmod
	mov	%i1,%o1
	mov	%i3,%o3
	tst	%l6
	ble	T3_end
	subcc	%o1,%o3,%o5		! z1 = x1-y1	cc
T3_loop:
	subxcc	%o0,%o2,%o4		! z0 = x0-y0-b
	bl,a	1f
	addcc	%o1,%o1,%o1		! x1 = x1+x1	cc
	addcc	%o5,%o5,%o1		! x1 = z1+z1	cc
	bne	2f
	addxcc	%o4,%o4,%o0		! x0 = z0+z0+c	cc
	bne	3f
	subcc	%o1,%o3,%o5		! z1 = x1-y1
	ba	fmod_zero
	nop
1: 	addx	%o0,%o0,%o0		! x0 = x0+x0+c
2:	subcc	%o1,%o3,%o5		! z1 = x1-y1
3:
	subxcc	%o0,%o2,%o4		! z0 = x0-y0-b
	bl,a	1f
	addcc	%o1,%o1,%o1		! x1 = x1+x1	cc
	addcc	%o5,%o5,%o1		! x1 = z1+z1	cc
	bne	2f
	addxcc	%o4,%o4,%o0		! x0 = z0+z0+c	cc
	bne	3f
	subcc	%o1,%o3,%o5		! z1 = x1-y1
	ba	fmod_zero
	nop
1: 	addx	%o0,%o0,%o0		! x0 = x0+x0+c
2:	subcc	%o1,%o3,%o5		! z1 = x1-y1
3:
	subxcc	%o0,%o2,%o4		! z0 = x0-y0-b
	bl,a	1f
	addcc	%o1,%o1,%o1		! x1 = x1+x1	cc
	addcc	%o5,%o5,%o1		! x1 = z1+z1	cc
	bne	2f
	addxcc	%o4,%o4,%o0		! x0 = z0+z0+c	cc
	bne	3f
	subcc	%o1,%o3,%o5		! z1 = x1-y1
	ba	fmod_zero
	nop
1: 	addx	%o0,%o0,%o0		! x0 = x0+x0+c
2:	subcc	%o1,%o3,%o5		! z1 = x1-y1
3:
    ! final section in loop
	subxcc	%o0,%o2,%o4		! z0 = x0-y0-b
	bl,a	1f
	addcc	%o1,%o1,%o1		! x1 = x1+x1	cc
	addcc	%o5,%o5,%o1		! x1 = z1+z1	cc
	bne	2f
	addxcc	%o4,%o4,%o0		! x0 = z0+z0+c	cc
	bne	3f
	subcc	%l6,1,%l6
	ba	fmod_zero
	nop
1: 	addx	%o0,%o0,%o0		! x0 = x0+x0+c
2:	subcc	%l6,1,%l6
3:	bg,a	T3_loop
	subcc	%o1,%o3,%o5
T3_end:
    ! new sub 1 from k
	subcc	%l7,1,%l7
	bl	T4
	subcc	%o1,%o3,%o5
	subxcc	%o0,%o2,%o4		! z0 = x0-y0-b
	bl,a	1f
	addcc	%o1,%o1,%o1		! x1 = x1+x1	cc
	addcc	%o5,%o5,%o1		! x1 = z1+z1	cc
	bne	2f
	addxcc	%o4,%o4,%o0		! x0 = z0+z0+c	cc
	bne	3f
	subcc	%l7,1,%l7
	ba	fmod_zero
	nop
1: 	addx	%o0,%o0,%o0		! x0 = x0+x0+c
2:	subcc	%l7,1,%l7
3:	bl	T4
	subcc	%o1,%o3,%o5		! z1 = x1-y1
	subxcc	%o0,%o2,%o4		! z0 = x0-y0-b
	bl,a	1f
	addcc	%o1,%o1,%o1		! x1 = x1+x1	cc
	addcc	%o5,%o5,%o1		! x1 = z1+z1	cc
	bne	2f
	addxcc	%o4,%o4,%o0		! x0 = z0+z0+c	cc
	bne	3f
	subcc	%l7,1,%l7
	ba	fmod_zero
	nop
1: 	addx	%o0,%o0,%o0		! x0 = x0+x0+c
2:	subcc	%l7,1,%l7
3:	bl	T4
	subcc	%o1,%o3,%o5		! z1 = x1-y1
	subxcc	%o0,%o2,%o4		! z0 = x0-y0-b
	bl,a	1f
	addcc	%o1,%o1,%o1		! x1 = x1+x1	cc
	addcc	%o5,%o5,%o1		! x1 = z1+z1	cc
	bne	2f
	addxcc	%o4,%o4,%o0		! x0 = z0+z0+c	cc
	bne	3f
	subcc	%o1,%o3,%o5		! z1 = x1-y1
	ba	fmod_zero
	nop
1: 	addx	%o0,%o0,%o0		! x0 = x0+x0+c
2:	subcc	%o1,%o3,%o5		! z1 = x1-y1
3:	
T4:
	subxcc	%o0,%o2,%o4
	bl	3f
	nop
	mov	%o4,%o0
	mov	%o5,%o1
3:
	orcc	%o0,%o1,%g0
	bne,a	4f
	nop
	and	%i0,%l1,%o0
	std	%o0,[%fp+x]
	ba	fmod_return
	ldd	[%fp+x],%f0
4:
    ! convert back to floating point
	andcc	%o0,%l3,%g0
	bne	T6
	nop
T5:
	addcc	%o1,%o1,%o1
	addx	%o0,%o0,%o0
	andcc	%o0,%l3,%g0
	be	T5
	sub	%l5,%l3,%l5
T6:
	and	%i0,%l1,%i4		! i4  = sign(x)
	or	%l5,%i4,%l5		! ib |= sign(x)
	andn	%o0,%l2,%o0		! clear exponent field of x
	or	%l5,%o0,%o0
	std	%o0,[%fp+x]
	ldd	[%fp+x],%f0
fmod_return:
	ret
	restore

fmod_zero:
	and	%i0,%l1,%o4
	st	%o4,[%fp+x]
	st	%g0,[%fp+x+4]
	ba	fmod_return
	ldd	[%fp+x],%f0

fmod_tiny:
    ! |y| < 2^969, return 2^-110*fmod(2^110*fmod(x,2^110*y),2^110*y)
	mov	%i0,%o0
	mov	%i1,%o1
	mov	%i3,%o3
	set	constant,%l0
	ldd	[%l0+two110],%f2
	std	%o2,[%fp+y]
	ldd	[%fp+y],%f0
	fmuld	%f0,%f2,%f0
	std	%f0,[%fp+y]
	ldd	[%fp+y],%o2		! 2^110*y
	call	fmod_in
	nop
	ldd	[%l0+two110],%f2
	fmuld	%f2,%f0,%f0		! 2^110*fmod(x,2^110*y)
	ldd	[%fp+y],%o2		! 2^110*y
	std	%f0,[%fp+x]
	ldd	[%fp+x],%o0
	call	fmod_in
	nop
	ldd	[%l0+twom110],%f2
	fmuld	%f0,%f2,%f0
	ba	fmod_return
	nop

fmod_exception:
	std	%i0,[%fp+x]
	std	%i2,[%fp+y]
	ldd	[%fp+x],%f0
	ldd	[%fp+y],%f2
	fmuld	%f0,%f2,%f0
	fdivd	%f0,%f0,%f0
	ba	fmod_return
	nop
