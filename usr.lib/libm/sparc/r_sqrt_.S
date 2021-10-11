
	.seg	"data"
	.asciz	"@(#)r_sqrt_.S 1.1 92/07/30 SMI"

#define LOCORE
#include <machine/asm_linkage.h>

! Copyright (c) 1988 by Sun Microsystems, Inc.
!
! Fortran single _r_sqrt_(x) 
! Code by K.C. Ng, Dec 2, 1986, revised on May 13 (Friday), 1988.
!
! _r_sqrt_(x) returns the correctly rounded (according to the prevailing 
! rounding mode) square root of x. 
!
! Algorithm:
!	0. Disable inexact trap and set rounding mode to RN
!	1. Shift and add to get 7.8 bits approximation of 1/sqrt(x).
!	2. Apply reciproot iteration once in single precision, and then 
!	   twice in double precision (with some adjustment, see the double
!	   "_sqrt") and multiply the reciproot by x to get a near perfect 
!	   approximation of sqrt(x) in double precision.
!	3. Restore rounding mode and inexact enable flag
!	4. Round the double precision answer to single precision.
!
! Note. The algorithm has been tested on a SUN-3 for all single precision
!	arguments between 1.0 and 4.0.
!
	.seg	"data"
	.align	8
constant:
const1	= 0x0
	.word   0x3ff7ffff,0xffc00000   ! double 1.5-2**-30
half	= 0x8
	.word	0x3fe00000,0		! double 0.5
fone	= 0x10
	.word	0x3f800000	! 1.0
two54	= 0x14
	.word	0x5a800000	! 2**54
twom27	= 0x18
	.word	0x32000000	! 2**-27
fhalf	= 0x1c
	.word	0x3f000000	! 0.5
fohalf	= 0x20
	.word	0x3fc00000	! 1.5 
table	= 0x24
	.word	0x1500<<3,0x2ef8<<3,0x4d67<<3,0x6b02<<3
	.word	0x87be<<3,0xa395<<3,0xbe7a<<3,0xd866<<3
	.word	0xf14a<<3,0x1091b<<3,0x11fcd<<3,0x13552<<3
	.word	0x14999<<3,0x15c98<<3,0x16e34<<3
	.word	0x17e5f<<3,0x18d03<<3,0x19a01<<3,0x1a545<<3
	.word	0x1ae8a<<3,0x1b5c4<<3,0x1bb01<<3
	.word	0x1bfde<<3,0x1c28d<<3,0x1c2de<<3,0x1c0db<<3
	.word	0x1ba7e<<3,0x1b11c<<3,0x1a4b5<<3
	.word	0x1953d<<3,0x18266<<3,0x16be0<<3,0x1683e<<3
	.word	0x179d8<<3,0x18a4d<<3,0x19992<<3
	.word	0x1a789<<3,0x1b445<<3,0x1bf61<<3,0x1c989<<3
	.word	0x1d16d<<3,0x1d77b<<3,0x1dddf<<3
	.word	0x1e2ad<<3,0x1e5bf<<3,0x1e6e8<<3,0x1e654<<3
	.word	0x1e3cd<<3,0x1df2a<<3,0x1d635<<3
	.word	0x1cb16<<3,0x1be2c<<3,0x1ae4e<<3,0x19bde<<3
	.word	0x1868e<<3,0x16e2e<<3,0x1527f<<3
	.word	0x1334a<<3,0x11051<<3,0xe951<<3,0xbe01<<3
	.word	0x8e0d<<3,0x5924<<3,0x1edd<<3

! local variable using fp
x	= -0x8
tmp	= -0x10
ofsr	= -0x18
nfsr	= -0x1c

! register usage
! i0 = x
! i1 = |x|
	.seg	"text"
	ENTRY(r_sqrt_)
inrsqrt:
	save	%sp,-128,%sp
	set	constant,%l0
	ld	[%i0],%f0
	ld	[%i0],%i0
	sethi	%hi(0x80000000),%o3	! o3 = 0x80000000
	andn	%i0,%o3,%i1		! i1 = |x|
	sethi	%hi(0x3f800800),%o4	! o4 = 1+2**-12
	cmp	%i1,%o4
	bl	1f
	sethi	%hi(0x3f7ff000),%o4	! o4 = 1-2**-12
	sethi	%hi(0x7f800000),%o4
	cmp	%i1,%o4
	bl	normal
	tst	%i0
	bl	NaN
	nop
    ! x is inf or NaN
	ba	rsqrt_return
	fadds	%f0,%f0,%f0
1:
	cmp	%i1,%o4
	bg	quickcrank
	tst	%i0
	tst	%i1
	bne	1f
	tst	%i0
    ! x = +-0, return x
	ba	rsqrt_return
	nop
1:
	bl	NaN
	sethi	%hi(0x01000000),%o4
	cmp	%i1,%o4
	bg	normal
	tst	%i0
    ! x is too tiny, may be subnormal
	ld	[%l0+two54],%f1
	fmuls	%f0,%f1,%f0		! scale up x
	st	%f0,[%fp+x]
	call	inrsqrt			! call rsqrt again
	add	%fp,x,%o0
	ld	[%l0+twom27],%f2
	fmuls	%f0,%f2,%f0		! scale back sqrt(x)
	ba	rsqrt_return
	nop

quickcrank:
    ! now |x| is between 0x3f7ff0000 and 0x3f800800
	bl	NaN
	sethi	%hi(0x3f800000),%o3
	cmp	%i0,%o3
	bne,a	1f
	ld	[%l0+twom27],%f2	! f2 = 2**-27
	ba	rsqrt_return
	nop
1:
	sra	%i0,1,%o1
	sethi	%hi(0x00400000),%o2
	sub	%o1,%o2,%o1
	sethi	%hi(0x20000000),%o2
	or	%o1,%o2,%o1		! o1 = 1+(x-1)/2
	st	%o1,[%fp+x]
	andcc	%i0,1,%g0
	bne	odd
	ld	[%fp+x],%f0
	fnegs	%f2,%f2			! even 
odd:
	ba	rsqrt_return
	fadds	%f0,%f2,%f0
normal:
	bl	NaN
    ! save RD, NX flags; reset RD to RN; disable NX;  save f4-f11
	st	%fsr,[%fp+ofsr]		! save fsr 
	ld	[%fp+ofsr],%o4		! o4 = fsr
	sethi	%hi(0xc8000000),%o3	! set mask to reset NX and RD
	andn	%o4,%o3,%o4		! reset o4 (fsr's copy)
	st	%o4,[%fp+nfsr]
	ld	[%fp+nfsr],%fsr		! disable NX and set RD to RN
    ! initial approximation for 1/sqrt(x)
	srl	%i0,1,%o0		! high x >> 1
	ld	[%l0+fhalf],%f2		! f2 = 0.5
	fmuls	%f2,%f0,%f1		! f1 = 0.5*x, f0=x 
	sethi	%hi(0x5f400000),%o1	! offset constant
	sub	%o1,%o0,%o0		! o0 = k = 0x5f400000 - (hx >> 1)
	add	%l0,table,%o2		! o2 = address of data table
	srl	%o0,15,%o1		! o1 := 4*(k >> 17)
	and	%o1,0xfc,%o1		! o1 := 4*[63&(o0>>17)]
	add	%o2,%o1,%o2		! o2 = address of the constant
	ld	[%o2],%o3		! o3 = adjustment for the approximation
	sub	%o0,%o3,%o0		! y approximates 1/sqrt(x) to 7.8 bits
	st	%o0,[%fp+tmp]		! tmp = y
	ld	[%fp+tmp],%f2		! f2 = initial approx. of 1/sqrt(x)
    ! reciproot iteration once
	fmuls	%f1,%f2,%f3		! f3 = (0.5*x)*y
	fstod	%f1,%f6			! set f6 = double x*0.5
	fmuls	%f2,%f3,%f3		! f3 = ((0.5*x)*y)*y
	ld	[%l0+fohalf],%f4	! f4 = 1.5
	fsubs	%f4,%f3,%f4		! 1.5-0.5*x*y*y
	fmuls	%f2,%f4,%f2		! new y = old y*(1.5-0.5*x*y*y)
    ! reciproot iteration twice in double
	fstod	%f2,%f8			! f8 = double y
	fmuld	%f8,%f8,%f4		! f4 = y*y
	fmuld	%f6,%f4,%f4		! f4 = 0.5*x*y*y
	ldd	[%l0+const1],%f10
	fsubd	%f10,%f4,%f4		! f4 = (1.5-2**-30)-0.5*x*y*y
	fmuld	%f8,%f4,%f8		! f8 = new y 
    ! reciproot iteration thrice in double
	fstod	%f0,%f10		! f10= (double x)
	fmuld	%f10,%f8,%f4		! f4 = z = x*y
	std	%f8,[%fp+tmp]
	ld	[%fp+tmp],%o4
	sethi	%hi(0x00100000),%o3
	sub	%o4,%o3,%o4
	st	%o4,[%fp+tmp]
	ldd	[%fp+tmp],%f12		! f12= 0.5*y
	fmuld	%f12,%f4,%f2		! f2 = 0.5*z*y
	ldd	[%l0+half],%f6		! f4 = 0.5
	fsubd	%f6,%f2,%f2		! f2 = 0.5 - 0.5*z*y
	fmuld	%f4,%f2,%f2
	faddd	%f4,%f2,%f2		! f2 = z + z*(0.5-0.5*z*y)
    ! restore fsr
	ld	[%fp+ofsr],%fsr
    ! convert the double precision result to single with adjustment
	fdtos	%f2,%f0
rsqrt_return:
	ret
	restore

NaN:
	fsubs	%f0,%f0,%f0		! f0 is either 0 or NaN 
	ba	rsqrt_return
	fdivs	%f0,%f0,%f0		! f0 is NaN with signal unless x is NaN
