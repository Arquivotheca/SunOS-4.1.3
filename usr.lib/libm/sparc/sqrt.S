
	.seg	"data"
	.asciz	"@(#)sqrt.S 1.1 92/07/30 SMI"

#define LOCORE
#include <machine/asm_linkage.h>

! Copyright (c) 1988 by Sun Microsystems, Inc.
!
! double sqrt(x) 
! Double precision IEEE sqrt on Sunrise
! Code by K.C. Ng, March 9, 1987, revised on May 14,1988.
!
! Sqrt(x) returns the correctly rounded (according to the prevailing 
! rounding mode) square root of x. 
!
! Algorithm:
!	1. Shift and add to get 7.8 bits approximation of 1/sqrt(x).
!	2. Apply reciproot iteration 2 times, then switch to newton iteration
!	   once to get an approximation of sqrt(x) to one ulp.
!	3. Final adjustment using division to get correctly rounded sqrt(x).
!
	.seg	"data"
	.align	8
constant:
twom57	= 0x0
	.word	0x3c600000,0x0		! 2**-57
two54	= 0x8
	.word	0x43500000,0x0		! = 2**54
twom27	= 0x10
	.word	0x3e400000,0x0		! = 2**-27
half	= 0x18
	.word	0x3fe00000,0x0		! = 0.5
one	= 0x20
	.word	0x3ff00000,0x0		! = 1.0
onehalf	= 0x28
	.word	0x3ff80000,0x0		! = 1.5 
onemulp	= 0x30
	.word	0x3fefffff,0xffffffff	! = 1-2**-53
table	= 0x38	
	.word	0x1500,0x2ef8,0x4d67,0x6b02,0x87be,0xa395,0xbe7a,0xd866
	.word	0xf14a,0x1091b,0x11fcd,0x13552,0x14999,0x15c98,0x16e34
	.word	0x17e5f,0x18d03,0x19a01,0x1a545,0x1ae8a,0x1b5c4,0x1bb01
	.word	0x1bfde,0x1c28d,0x1c2de,0x1c0db,0x1ba7e,0x1b11c,0x1a4b5
	.word	0x1953d,0x18266,0x16be0,0x1683e,0x179d8,0x18a4d,0x19992
	.word	0x1a789,0x1b445,0x1bf61,0x1c989,0x1d16d,0x1d77b,0x1dddf
	.word	0x1e2ad,0x1e5bf,0x1e6e8,0x1e654,0x1e3cd,0x1df2a,0x1d635
	.word	0x1cb16,0x1be2c,0x1ae4e,0x19bde,0x1868e,0x16e2e,0x1527f
	.word	0x1334a,0x11051,0xe951,0xbe01,0x8e0d,0x5924,0x1edd

! local variable using fp
x	= -0x8
y	= -0x10
tmp	= -0x18
ofsr	= -0x20
nfsr	= -0x24
tfsr	= -0x28


	.seg	"text"
	.global	_SVID_libm_err
	ENTRY(sqrt)
insqrt:		
	save	%sp,-144,%sp
	set	constant,%l0		! l0 = address of constant
	std	%i0,[%fp+x]		! save x 
	sethi	%hi(0x3ff00000),%o4	! o4 = 0x3ff00000
	subcc	%i0,%o4,%o4
	bne	1f
	addcc	%o4,1,%o4
    ! check whether x < 1+2**-27
	sethi	%hi(0x02000000),%o3	
	cmp	%i1,%o3
	bgu	2f
	tst	%i1
	bne	quickcrank
    ! x = 1, return x
	nop
	ba	sqrt_return
	ldd	[%fp+x],%f0		! load x
    ! x = 1 +- tiny with tiny < 2**-27
quickcrank:
	ldd	[%l0+twom57],%f2	! f2 = 2**-57
	sra	%i1,1,%o2
	st	%o2,[%fp+x+4]
	andcc	%i1,1,%g0		! x even?
	bne	odd
	ldd	[%fp+x],%f0
	fnegs	%f2,%f2			! even
odd:
	faddd	%f0,%f2,%f0
	ba	sqrt_return
	nop
1:
	bne	2f
	sethi	%hi(0x7ff00000),%o4
	sethi	%hi(0xfc000000),%o3
	cmp	%i1,%o3
	bgu	quickcrank
	nop
2:
	sethi	%hi(0x00100000),%o3
	and	%i0,%o4,%o2
	cmp	%o2,%o3
	bg	1f
	sethi	%hi(0x80000000),%o3	! o3 = 0x80000000
    ! x is 0 or very tiny, but may not be subnormal
	andn	%i0,%o3,%o2		! purging signed zero
	orcc	%o2,%i1,%g0
	be	quickreturn		! x is +-zero, return x+x
	tst	%i0
	bl	NaN			! x < 0, return NaN
	nop
    ! tiny positive x 
	ldd	[%l0+two54],%f2		! set f2 = 2**54
	ldd	[%fp+x],%f0		! load x
	fmuld	%f0,%f2,%f0		! scale up x by 2**54
	std	%f0,[%fp+tmp]
	ldd	[%fp+tmp],%o0		! get High part of new x
	call	insqrt			! call sqrt again, avoid profiling
	nop
	ldd	[%l0+twom27],%f2	! f2 = 2**-(27)
	fmuld	%f0,%f2,%f0		! scale down sqrt(x) by 2**27
	ba	sqrt_return
	nop

1:
    ! finite x, 0.5*x will not underflow
	tst	%i0
	bl	NaN			! x < 0 return NaN
	cmp	%o2,%o4			! here o4 = 0x7ff00000
	be	quickreturn		! x must be NaN or +INF; return x+x
	nop

    ! now for reciproot algorithm ... 
    ! save RD, NX flags; reset RD to RN; disable NX;  save f4-f11
	st	%fsr,[%fp+ofsr]		! save fsr
	ld	[%fp+ofsr],%o2		! o2 = fsr
	sethi	%hi(0xff800000),%o3	! set mask to reset TEM,RP and RD
	andn	%o2,%o3,%o2		! fsr with everything clear
	sethi	%hi(0x40000000),%o3	! RD = RZ
	or	%o2,%o3,%o2		! new fsr with RD = RZ
	st	%o2,[%fp+nfsr]
	st	%g0,[%fp+tfsr]
	ld	[%fp+tfsr],%fsr		! disable TEM, and set RD to RN
	ldd	[%fp+x],%f0
	ldd	[%l0+half],%f2
	fmuld	%f0,%f2,%f2		! f2 = 0.5*x
    ! initial approximation for 1/sqrt(x)
	srl	%i0,1,%o0		! high x >> 1
	sethi	%hi(0x5fe80000),%o1	! offset constant
	sub	%o1,%o0,%o0		! o0 = k = 0x5fe80000 - (hx >> 1)
	add	%l0,table,%o2		! o2 = address of data table
	srl	%o0,12,%o1		! o1 := 4*(k >> 14)
	and	%o1,0xfc,%o1		! o1 := 4*[63&(o0>>14)]
	add	%o2,%o1,%o2		! o2 = address of the adjusting data
	ld	[%o2],%o3		! o3 = adjustment for the approximation
	sub	%o0,%o3,%o0		! high y ~ 1/sqrt(x) to 7.8 bits
	st	%o0,[%fp+y]		! tmp = y
	ldd	[%fp+y],%f10		! f10 = y
    ! reciproot iteration once
	fmuld	%f2,%f10,%f4		! f4 = (0.5*x)*y
	fmuld	%f10,%f4,%f6		! f6 = ((0.5*x)*y)*y)
	ldd	[%l0+onehalf],%f8	! f8 = 1.5
	fsubd	%f8,%f6,%f6		! f6 = 1.5-0.5*x*y*y
	fmuld	%f10,%f6,%f4		! f4 = y' = y*(1.5-0.5*x*y*y)
    ! reciproot iteration twice
	sethi	%hi(0x00100000),%o4
	fmuld	%f0,%f4,%f6		! f6 = x*y
	std	%f4,[%fp+tmp]
	ld	[%fp+tmp],%o3		! o3 = Hy
	sub	%o3,%o4,%o3		! o3 = H(0.5*y)
	st	%o3,[%fp+tmp]
	ldd	[%fp+tmp],%f8		! f8 = 0.5*y
	fmuld	%f8,%f6,%f8		! f8 = 0.5*x*y*y
	ldd	[%l0+onehalf],%f10	! f10 = 1.5
	fsubd	%f10,%f8,%f8		! f8 = 1.5-0.5*x*y*y
	fmuld	%f6,%f8,%f4		! f4 = (x*y)*(1.5-0.5*x*y*y)
					! now f4 is 29 bit to sqrt(x)
    ! Now use Newton iteration, at the same time determine RD
	fdivd	%f2,%f4,%f6		! f6 = (0.5*x)/y rounded
	ld	[%fp+ofsr],%o4		! o4 = old fsr
	or	%o4,0x21,%o0
	st	%o0,[%fp+tfsr]		! old fsr with inexact accured
	srl	%o4,30,%o3		! o3 = RD
	sethi	%hi(0xf0000000),%o2	! o2 = mask for RD and RP
	andn	%o4,%o2,%o4
	sethi	%hi(0x40000000),%o2
	or	%o4,%o2,%o4		! old fsr with RD=RZ and RP=0
	or	%o4,0x20,%o4
	xor	%o4,0x20,%o4		! with aexc (inexact)=0
	st	%o4,[%fp+nfsr]
	std	%f4,[%fp+tmp]
	ld	[%fp+tmp],%o0		! o0 = Hy
	sethi	%hi(0x00100000),%o1
	sub	%o0,%o1,%o0		! o0 = H(0.5*y)
	st	%o0,[%fp+tmp]
	ldd	[%fp+tmp],%f10		! f10 = 0.5*y
	faddd	%f6,%f10,%f4		! f4 = sqrt(x) to 1 ulp

    ! Twisted last bit to make sqrt(x) correctly rounded
	ld	[%fp+nfsr],%fsr		! restore INEXACT flags, RD=RZ
	nop				! three nop after ldfsr
	nop
	nop
	fdivd	%f2,%f4,%f0		! f6 = z = (0.5*x)/y chopped quotient
	std	%f4,[%fp+tmp]		! y is f4
	ldd	[%fp+tmp],%o0
	sethi	%hi(0x00100000),%o4
	sub	%o0,%o4,%o0		! y = y/2
	st	%o0,[%fp+tmp]
	ldd	[%fp+tmp],%f2		! f2 = y/2
	addcc	%o1,1,%o1
	addx	%o0,%g0,%o0		!  y/2+ulp
	std	%o0,[%fp+tmp]		! now tmp+4 = y/2+ulp
	ldd	[%l0+onemulp],%f4	! f4 = 0.9999999.... chopped
	set	0x20,%o4		! o4 = inexact accrued mask
					! o3 = RD from above
	std	%f0,[%fp+x]		! make sure div is finished
	st	%fsr,[%fp+nfsr]
	fmuld	%f0,%f4,%f4		! f4 = Z-1
	ld	[%fp+nfsr],%o0		! o0 = current fsr
	andcc	%o0,%o4,%o0		! see if inexact flags was up
	bne	1f
	nop
    ! exact division, if z=y then exit
	fcmpd	%f0,%f2
	nop
	fbne	2f
	nop
	ld	[%fp+ofsr],%fsr		! restore old fsr
	nop				! three nop after ldfsr
	nop
	nop
	faddd	%f0,%f0,%f0
	ba	sqrt_return
	nop

2:
    ! z!=y, set z=z-1
	fmovs	%f4,%f0
	fmovs	%f5,%f1
	std	%f4,[%fp+x]
1:
	andcc	%o3,1,%g0		! test RD
	bne	1f			! goto 1f if RD is RZ or RM
	ld	[%fp+x],%o4
	ld	[%fp+x+4],%o2
	addcc	%o2,1,%o2
	addx	%o4,%g0,%o4
	st	%o4,[%fp+x]
	st	%o2,[%fp+x+4]		! fp+x store z+ulp
	andcc	%o3,2,%g0
	be	1f			! if RN goto 1f 
	ldd	[%fp+x],%f0
	ldd	[%fp+tmp],%f2		! RP: f2 <- y+ulp
1:	
	faddd	%f0,%f2,%f0
	nop
	nop
	ld	[%fp+tfsr],%fsr
sqrt_return:
	ret
	restore


quickreturn:
	ldd	[%fp+x],%f0
	ba	sqrt_return
	faddd	%f0,%f0,%f0

NaN:
    ! reload x in %f0
	ldd	[%fp+x],%f0
    ! purge off NaN argument
	ldd	[%fp+x],%o0
	set	0xfff00000,%o2
	and	%o0,%o2,%o3
	cmp	%o3,%o2
	bne	invalid
	andn	%o0,%o2,%o0
	orcc	%o0,%o1,%g0
	be	invalid
	nop
    ! input is -NaN, just return x+x
	ba	sqrt_return
	faddd	%f0,%f0,%f0
    ! result is invalid
invalid:
!	fsubd	%f0,%f0,%f0
!	fdivd	%f0,%f0,%f0		! 0/0 create invalid signal
	!
	! Call SVID_libm_err to create invalid signal
	!
	    ! set input parameter for system V error handling routine
		ldd	[%fp+x],%o0
		mov	%o0,%o2
		mov	%o1,%o3
		set	26,%o4		! private error type 1: sqrt(-ve)
	    ! call system V error handling routine
		call	_SVID_libm_err
		nop
		ba	sqrt_return
		nop
	!
	! end of codes for System V
	!
