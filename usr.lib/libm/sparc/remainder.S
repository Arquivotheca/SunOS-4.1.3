
	.seg	"data"
	.asciz	"@(#)remainder.S 1.1 92/07/30 SMI"

!	Copyright (c) 1988 by Sun Microsystems, Inc.

#define LOCORE
#include <machine/asm_linkage.h>
!
! double remainder(x,y) 
! Double precision IEEE remainder on Sunrise
! See libm/C/remainder.c
!
	.seg	"data"
	.align	8
constant:
zero	= 0x00
	.word	0x0, 0x0
half	= 0x08
	.word	0x3fe00000,0x0

! local variable using fp
x	= -0x8
p	= -0x10
tmp	= -0x18

! local register variable
! i0,i1,i2,i3	input x,y
! i4 = i0, i0 will become |i0|
! l0 	= constant
! l1 	= 0x80000000
! l2 	= 0x7ff00000
! l3 	= 0x7fdfffff
! l4 	= 0x00200000

	.global	_fmod
	.seg	"text"
	ENTRY(remainder)
	save	%sp,-128,%sp
	set	constant,%l0		! l0 = address of constant
	sethi	%hi(0x80000000),%l1	! l1 = 0x80000000
	andn	%i2,%l1,%i2		! make p positive
	mov	%i0,%i4
	andn	%i0,%l1,%i0		! make x positive
	std	%i0,[%fp+x]
	std	%i2,[%fp+p]
	ldd	[%fp+x],%f0		! f0 = x
	ldd	[%fp+p],%f2		! f2 = p
	fcmpd	%f0,%f2			! |x| : |p|
	sethi	%hi(0x7ff00000),%l2
	nop
	fbu	exception		! x or p is NaN
	cmp	%i0,%l2
	be	exception		! x is inf
	orcc	%i2,%i3,%g0
	be	exception		! y is zero
	nop
	set	0x7fdfffff,%l3
	fbl	T1			! |x| < |p|, skip fmod
	cmp	%i2,%l3
	bg	T1
	nop
	faddd	%f2,%f2,%f2		! p <= hfmax, x = fmod(x,p+p)
	std	%f2,[%fp+tmp]
	ldd	[%fp+tmp],%o2		! p+p
	mov	%i0,%o0
	call	_fmod			! x = fmod(x,p+p)
	mov	%i1,%o1
	ldd	[%fp+p],%f2		! reset f2 to be p 
	std	%f0,[%fp+x]
	ldd	[%fp+x],%i0
T1:
    ! now |x| < |2p|
	sethi	%hi(0x00200000),%l4
	cmp	%i2,%l4			! is p < 2**-1021?
	bge	2f
	nop
    ! p < 2**-1021
	faddd	%f0,%f0,%f4		! f4 = x+x
	cmp	%i0,%i2	
	bne	1f
	cmp	%i1,%i3			! if x==p return signx*zero
	be	signzero
	nop
1:
	fcmpd	%f4,%f2			! x+x > p ?
	nop
	nop
	fble	final
	nop
	fsubd	%f0,%f2,%f0		! x -= p
	faddd	%f0,%f0,%f4
	fcmpd	%f4,%f2
	nop
	fbl	final
	nop
	ba	final
	fsubd	%f0,%f2,%f0
2:
    ! p >= 2**-1021
	ldd	[%l0+half],%f4
	fmuld	%f4,%f2,%f4
	cmp	%i0,%i2
	bne	1f
	cmp	%i1,%i3
	be	signzero
	nop
1:
	fcmpd	%f0,%f4
	nop
	nop
	fble	final
	nop
	fsubd	%f0,%f2,%f0
	fcmpd	%f0,%f4
	nop
	nop
	fbl	final
	nop
	fsubd	%f0,%f2,%f0
final:
	tst	%i4
	bl,a	1f
	fnegs	%f0,%f0
1:
	ret
	restore

exception:
	mov	%i4,%o0
	mov	%i1,%o1
	mov	%i2,%o2
	call	_fmod
	mov	%i3,%o3
	ba	1b
	nop

signzero:
	tst	%i4
	ldd	[%l0+zero],%f0
	bl,a	1b
	fnegs	%f0,%f0
	ba	1b
	nop

!
! old name for remainder
!
	.global	_drem 			! For compatibility with 3.2 and 4.3BSD
_drem:				
	ba	_remainder
	nop

