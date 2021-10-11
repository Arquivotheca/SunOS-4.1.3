
	.seg	"data"
	.asciz	"@(#)ieee_func.S 1.1 92/07/30 SMI"

!	Copyright (c) 1988 by Sun Microsystems, Inc.

#define LOCORE
#include <machine/asm_linkage.h>

!
! IEEE supported functions
!	copysign(x,y)	transfer the sign bit of y to x's
!	finite(x)	return TRUE(=1) if x is finite, else return FALSE(=0)
!	isnan(x)	return TRUE(=1) if x is NaN, else return FALSE(=0)
!	isinf(x)	return TRUE(=1) if x is INF, else return FALSE(=0)
!	isnormal(x)	return TRUE(=1) if x is normal, else return FALSE(=0)
!	issubnormal(x)	return TRUE(=1) if x is subnormal, else return FALSE(=0)
!	iszero(x)	return TRUE(=1) if x is zero, else return FALSE(=0)
!	ilogb(x)	return unbias exponent of x in integer
!	nextafter(x,y)	return x+ sign(y-x)*ulp
!	scalbn(x,N)	return x*2**N
!	signbit(x)	return TRUN(=1) if x's sign bit is set
! Code by K.C. Ng, Oct 30, 1986, revised on May 11, 1988.
!
	.seg	"data"
	.align	8
constant:
two54 	= 0x00
	.word	0x43500000,0x0		!  2**54
twom54 	= 0x08
	.word	0x3c900000,0x0		!  2**-54
huge	= 0x10
	.word	0x7fe00000,0x0		!  huge 
tiny	= 0x18
	.word	0x00100000,0x0		!  tiny

! variable using fp
x 	= -0x8
y 	= -0x10

	.seg	"text"

	ENTRY(copysign)
	sethi	%hi(0x80000000),%o3
	andn	%o0,%o3,%o0
	and	%o2,%o3,%o2
	or	%o2,%o0,%o0
	std	%o0,[%sp+0x48]
	retl
	ldd	[%sp+0x48],%f0

	ENTRY(finite)
	sethi	%hi(0x7ff00000),%o1	! o1 = 0x7ff00000
	and	%o1,%o0,%o0		! o0 = bias exponent of x
	cmp	%o0,%o1
	bne,a	1f			! goto 1f if finite
	mov	1,%o0
	mov	0,%o0
1:	retl
	nop

	ENTRY(isnan)
	sethi	%hi(0x7ff00000),%o3	! o3 = 0x7ff00000
	and	%o3,%o0,%o2		! o2 = bias exponent of x
	cmp	%o2,%o3
	bne,a	1f
	mov	0,%o0
    ! x is either a NaN or an INF
	sethi	%hi(0xfff00000),%o3	! o3 = 0xfff00000
	andn	%o0,%o3,%o0		! mask off x's sign and exponent bits
	orcc	%o0,%o1,%g0		! see if the mantissa of x is zero
	be,a	1f			! x is INF, return o0 = 0 (FALSE)
	mov	0,%o0
	mov	1,%o0
1:	retl
	nop

	ENTRY(isinf)
	sethi	%hi(0x7ff00000),%o3	! o3 = 0x7ff00000
	and	%o3,%o0,%o2		! o2 = bias exponent of x
	subcc	%o3,%o2,%o2
	bne,a	1f			! branch to 1f if o2 is finite
	mov	0,%o0
    ! x is either a NaN or an INF
	sethi	%hi(0xfff00000),%o3	! o3 = 0xfff00000
	andn	%o0,%o3,%o0		! mask off x's sign and exponent bits
	orcc	%o0,%o1,%g0		! see if the mantissa of x is zero
	bne,a	1f			! x is NaN, return o0 = 0 (FALSE)
	mov	0,%o0
	mov	1,%o0
1:	retl
	nop

	ENTRY(isnormal)
	sethi	%hi(0x7ff00000),%o3	! o3 = 0x7ff00000
	andcc	%o3,%o0,%o2		! o2 = bias exponent of x
	be,a	1f			! branch to 1f if o2 is zero
	mov	0,%o0
	cmp	%o2,%o3
	be,a	1f			! branch to 1f if o2 is not finite
	mov	0,%o0
	mov	1,%o0
1:	retl
	nop

	ENTRY(issubnormal)
	sethi	%hi(0x7ff00000),%o3	! o3 = 0x7ff00000
	andcc	%o3,%o0,%o2		! o2 = bias exponent of x
	bne,a	1f			! branch to 1f if o2 is not zero
	mov	0,%o0
	sethi	%hi(0x80000000),%o3
	andn	%o0,%o3,%o2
	orcc	%o2,%o1,%g0
	be,a	1f			! branch to 1f if o2 is not zero
	mov	0,%o0
	mov	1,%o0
1:	retl
	nop

	ENTRY(iszero)
	sethi	%hi(0x80000000),%o3	! o3 = 0x80000000
	andn	%o0,%o3,%o2		! o2 = Hx&0x7fffffff
	orcc	%o1,%o2,%g0		! o0 = Lx|(Hx&0x7fffffff)
	bne,a	1f			! branch to 1f if o2 is not zero
	mov	0,%o0
	mov	1,%o0
1:	retl
	nop

	ENTRY(ilogb)
	std	%o0,[%sp+0x48]
	sethi	%hi(0x7ff00000),%o4	! o4 = 0x7ff00000
	andcc	%o4,%o0,%o2		! o2 = bias exponent of x
	bne	1f			! x is normal 
	ldd	[%sp+0x48],%f0
    ! x is subnormal or zero
	set	constant,%o3
	ldd	[%o3+two54],%f2		! f2 = 2**54
	fmuld	%f0,%f2,%f0		! scale up x by 2**54 
	set	0x80000001,%o0		! o0 =  -(2**31-1) 
	st	%f0,[%sp+0x48]
	ld	[%sp+0x48],%o2
	andcc	%o2,%o4,%o2
	be	2f
	sra	%o2,20,%o2		! o2 = shifted bias exponent
	ba	2f
	sub	%o2,0x435,%o0		! o0 = unbias exponent
    ! x is not subnormal
1:	cmp	%o4,%o2			! check whether x is NaN/INF
	sra	%o2,20,%o3		! o3 = shifted bias exponent
	bne,a	2f			! goto 1f if x is not NaN or INF
	sub	%o3,0x3ff,%o0		! o0 = unbias exponent
	set	0x7fffffff,%o0		! return 2**31-1 if x is not finite
2:	retl
	nop

	ENTRY(nextafter)
	save	%sp,-128,%sp
	std	%i0,[%fp+x]
	std	%i2,[%fp+y]
	ldd	[%fp+x],%f0
	ldd	[%fp+y],%f2
	fcmpd	%f0,%f2			! x:y
	set	constant,%l0
	sethi	%hi(0x80000000),%l1
	andn	%i0,%l1,%l4
	fbe	next_return
	nop
	fbu,a	next_return
	faddd	%f0,%f2,%f0
	orcc	%i1,%l4,%g0		! see if x is zero
	bne	1f
	tst	%i0
    ! x is zero, return sign(y)*min 
	and	%i2,%l1,%i0
	ba	next_final
	mov	1,%i1
1:	bge	2f
	nop
    ! x is negative
	fbl	next_subulp
	nop
	fbg	next_addulp
	nop
2:
	fbl	next_addulp
	nop
next_subulp:
	subcc	%i1,1,%i1
	ba	next_final
	subx	%i0,0,%i0
next_addulp:
	addcc	%i1,1,%i1
	addx	%i0,0,%i0
next_final:
	sethi	%hi(0x7ff00000),%l3
	std	%i0,[%fp+x]
	andcc	%i0,%l3,%i2
	be,a	xflow
	ldd	[%l0+tiny],%f2
	cmp	%i2,%l3
	bne,a	next_return
	ldd	[%fp+x],%f0
	ldd	[%l0+huge],%f2
xflow:
	fmuld	%f2,%f2,%f2
	ldd	[%fp+x],%f0
next_return:
	ret
	restore
	
	
	ENTRY(scalbn)
	save	%sp,-128,%sp
	set	constant,%l0
	sethi	%hi(0x80000000),%l1	! l1 = 0x80000000
	sethi	%hi(0x7ff00000),%l2	! l2 = 0x7ff00000
	andcc	%i0,%l2,%i3		! i3 = exponent of x
	bne,a	1f
	cmp	%i3,%l2			! see if x is NaN/inf
    ! x is zero or subnormal
	std	%i0,[%fp+x]
	andn	%i0,%l1,%l4
	andn	%l4,%l2,%l4
	orcc	%l4,%i1,%g0
	be	scalbn_return		! x is zero, return x
	ldd	[%fp+x],%f0
    ! for subnormal x, scalb up x by 2**54, subtract n by 54, then 
    ! goto scalbn_finite
	ldd	[%l0+two54],%f2
	fmuld	%f0,%f2,%f0
	cmp	%i2,-54			! if n < -54, underflow
	bl	scalbn_underflow
	nop
	sub	%i2,54,%i2
	std	%f0,[%fp+x]
	ldd	[%fp+x],%i0
	and	%i0,%l2,%i3		! i3 = exponent of x
	ba	scalbn_finite
	srl	%i3,20,%i3		! i3 = bias exponent of x
1:	
	bne,a	scalbn_finite
	srl	%i3,20,%i3		! i3 = bias exponent of x
    ! x is NaN or Inf
	std	%i0,[%fp+x]
	andn	%i0,%l1,%l4
	andn	%l4,%l2,%l4
	orcc	%l4,%i1,%g0
	be	scalbn_return		! x is inf, return x
	ldd	[%fp+x],%f0
	set	0x00080000,%l5
	andcc	%i0,%l5,%g0
	bne	scalbn_return		! x is qNaN, return x
	nop
	ba	scalbn_return
	faddd	%f0,%f0,%f0		! x is sNaN, return x+x
scalbn_finite:
	addcc	%i2,%i3,%i4		! add n to x's bias exponent
	bvc	2f			! goto 2f if not overflow
	sll	%i4,20,%i5		! i5 = exponent of 2^n*x
    ! n too large: overflow
	ba	scalbn_overflow
	nop
2:
	bg,a	3f			! non-subnormal output goto 3f
	cmp	%i4,0x7ff
    ! output could be underflow or subnormal
	cmp	%i4,-53
	ble	scalbn_underflow
	add	%i4,54,%i4
	sll	%i4,20,%i5
	andn	%i0,%l2,%i0
	or	%i0,%i5,%i0
	std	%i0,[%fp+x]
	ldd	[%l0+twom54],%f2
	ldd	[%fp+x],%f0
	ba	scalbn_return
	fmuld	%f0,%f2,%f0
3:
	bl,a	4f			! normal output goto 4f
	andn	%i0,%l2,%i0
	ba	scalbn_overflow
	nop
4:
	or	%i0,%i5,%i0
	std	%i0,[%fp+x]
	ldd	[%fp+x],%f0
scalbn_return:
	ret
	restore

scalbn_overflow:
	st	%fsr,[%fp+x]
	ld	[%fp+x],%l4
	set	0xc4000000,%l5
	andcc	%l4,%l5,%g0
	be	1f
	nop
    ! fsr not in default mode, simply return (sign(x)*huge)*huge
	andn	%i0,%l2,%i0
	set	0x7fe00000,%l5
	or	%l5,%i0,%i0
	std	%i0,[%fp+x]
	ldd	[%fp+x],%f0
	ldd	[%l0+huge],%f2
	ba	scalbn_return
	fmuld	%f0,%f2,%f0
1:
    ! fsr in default mode, simply return sign(x)*inf with overflow & inexact
	or	%l4,0x129,%l4
	st	%l4,[%fp+x]
	ld	[%fp+x],%fsr
	mov	0,%i1
	and	%i0,%l1,%i0
	or	%i0,%l2,%i0
	std	%i0,[%fp+x]
	ba	scalbn_return
	ldd	[%fp+x],%f0

scalbn_underflow:
	st	%fsr,[%fp+x]
	ld	[%fp+x],%l4
	set	0xc4000000,%l5
	andcc	%l4,%l5,%g0
	be	1f
	nop
    ! fsr not in default mode, simply return (sign(x)*tiny)*tiny
	andn	%i0,%l2,%i0
	set	0x00100000,%l5
	or	%l5,%i0,%i0
	std	%i0,[%fp+x]
	ldd	[%fp+x],%f0
	ldd	[%l0+tiny],%f2
	ba	scalbn_return
	fmuld	%f0,%f2,%f0
1:
    ! fsr in default mode, simply return sign(x)*0 with underflow & inexact
	or	%l4,0xa5,%l4
	st	%l4,[%fp+x]
	ld	[%fp+x],%fsr
	mov	0,%i1
	and	%i0,%l1,%i0
	std	%i0,[%fp+x]
	ba	scalbn_return
	ldd	[%fp+x],%f0


	ENTRY(signbit)
	retl
	srl	%o0,31,%o0
