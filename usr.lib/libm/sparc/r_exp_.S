
	.seg	"data"
	.asciz	"@(#)r_exp_.S 1.1 92/07/30 SMI"

#define LOCORE
#include <machine/asm_linkage.h>

! Copyright (c) 1989 by Sun Microsystems, Inc.
!
! single exp(x) float *x;
! Code by K.C. Ng (based on libm/R/r_exp_.c), April 7, 1987
! revised on Feb 22, 1989 (using table look-up scheme).
!

! variables in fp
ofsr = -8
tmp  = -16 
	.global	_scalbn
	.seg	"text"
	ENTRY(r_exp_)
	save	%sp,-120,%sp
	set	constant,%l0
	ld	[%i0],%f0		! f0 = x
	ld	[%i0],%o0		! o0 = x
	sethi	%hi(0x80000000),%o1	! o1 = 0x80000000
	andn	%o0,%o1,%o1		! o1 = |x|
	set	0x3EB17218,%o2		! o2 = 0.5*ln2
	cmp	%o1,%o2			! |x| < 0.5*ln2 ?
	bl	small
	sethi	%hi(0x3b000000),%o3	! o3 = 2**-9
    ! |x| >= 0.5*ln2
	set	%hi(0x42b00000),%o3	! o3 = 88.0
	cmp	%o1,%o3
	bl,a	finite
	ld	[%l0+invln2_32],%f2
	set	0x42cff1b4,%o3
	cmp	%o1,%o3
	bg	exception
	nop
	set	0x42b17218,%o3
	cmp	%o0,%o3
	bl,a	finite
	ld	[%l0+invln2_32],%f2
exception:
	sethi	%hi(0x7f800000),%o3
	cmp	%o0,%o3
	be	exit			! exp(inf)=inf, no exception
	cmp	%o1,%o3
	be,a	exit			! exp(-inf) = 0, no exception
	ld	[%l0+fzero],%f0
	st	%fsr,[%fp+ofsr]
	ld	[%fp+ofsr],%i1
	andn	%i1,0x1f,%i1		! clear cexc
	sethi	%hi(0xce800000),%i2
	andcc	%i1,%i2,%g0		! see if RD=0 and no trap on 
					! on invalid, over, underflow, inexact
	be	fastexception
	cmp	%o1,%o3
	bg,a	exit			! return NaN+NaN
	fadds	%f0,%f0,%f0
	ldd	[%l0+huge],%f2
	ldd	[%l0+tiny],%f4
	tst	%o0
	bg,a	exit
	fdtos	%f2,%f0			! overflow
	ba	exit		
	fdtos	%f4,%f0			! underflow
fastexception:
	sethi	%hi(0x7fc00000),%o4	! qNaN
	cmp	%o1,%o4
	bge	exit			! return qNaN with no exception
	cmp	%o1,%o3
	bl	xflow
	tst	%o0
    ! sNaN, set aexc and cexc and return quiet(sNaN)
	or	%i1,0x210,%i1
	st	%i1,[%fp+ofsr]
	ld	[%fp+ofsr],%fsr
	set	0x00400000,%o5
	or	%o5,%o0,%o0
	st	%o0,[%fp+tmp]
	ba	exit
	ld	[%fp+tmp],%f0
xflow:
	bl	uflow
	nop
    ! overflow
	or	%i1,0x129,%i1
	st	%i1,[%fp+ofsr]
	ld	[%fp+ofsr],%fsr
	st	%o3,[%fp+tmp]
	ba	exit
	ld	[%fp+tmp],%f0
uflow:
	or	%i1,0xa5,%i1
	st	%i1,[%fp+ofsr]
	ld	[%fp+ofsr],%fsr
	st	%o3,[%fp+tmp]
	ba	exit
	ld	[%l0+fzero],%f0
finite: ! result is finite
	! f0 = x, f2 = invln2_32
	fmuls	%f0,%f2,%f2		!
	tst	%o0
	bg,a	1f
	ld	[%l0+fhalf],%f4
	ld	[%l0+mfhalf],%f4
1:	fadds	%f2,%f4,%f2
	fstoi	%f2,%f6
	fitod	%f6,%f8
	st	%f6,[%fp+tmp]
	ldd	[%l0+ln2_32],%f10
	fmuld	%f10,%f8,%f10		! f10 = k*ln2_32
	ld	[%fp+tmp],%i3		! i3 = k
	fstod	%f0,%f0
	and	%i3,0x1f,%i4		! i4 = j = k&0x1f
	sra	%i3,5,%i5		! i5 = m = k>>5
	fsubd	%f0,%f10,%f0		! f0 = r 
	ldd	[%l0+a2],%f8
	fmuld	%f8,%f0,%f10
	ldd	[%l0+a1],%f6
	faddd	%f6,%f10,%f10
	sll	%i4,3,%i4		! j  <<3
	set	Sexp,%o3
	fmuld	%f0,%f10,%f10
	ldd	[%l0+one],%f4
	faddd	%f4,%f10,%f10
	ldd	[%o3+%i4],%f12		! f12 = Sexp[j]
	fmuld	%f0,%f10,%f10
	ld	[%o3+%i4],%o3
	sll	%i5,20,%i5		! i5 = m<<20
	faddd	%f4,%f10,%f10
	add	%i5,%o3,%o3		! o3 = 2^m * Sexp[j]
	st	%o3,[%fp+tmp]
	ld	[%fp+tmp],%f12
	fmuld	%f12,%f10,%f10
	fdtos	%f10,%f0
exit:
	ret
	restore

small:
	cmp	%o1,%o3
	ble	two_m9
	sethi	%hi(0x38800000),%o3	! 2**-14
    	sethi	%hi(0x3c800000),%o3	! 2**-6
	cmp	%o1,%o3
	bl,a	two_m6
	ld	[%l0+p2],%f2
    ! 2**-6 <= |x| < 0.5*ln2, use long poly
	fmuls	%f0,%f0,%f8
	ld	[%l0+p5],%f2
	fmuls	%f0,%f2,%f4
	ld	[%l0+p4],%f6
	fadds	%f6,%f4,%f4
	fmuls	%f0,%f4,%f4
	ld	[%l0+p3],%f2
	fadds	%f2,%f4,%f4
	fmuls	%f0,%f4,%f4
	ld	[%l0+p2],%f6
	fadds	%f6,%f4,%f4
	fmuls	%f0,%f4,%f4
	ld	[%l0+p1],%f2
	fadds	%f2,%f4,%f4
	fmuls	%f8,%f4,%f4
	fadds	%f0,%f4,%f4
	ld	[%l0+fone],%f6
	ba	exit
	fadds	%f6,%f4,%f0
two_m6:
	fmuls	%f0,%f2,%f4
	ld	[%l0+p1],%f6
	fadds	%f6,%f4,%f4
	ld	[%l0+fone],%f2
	fmuls	%f0,%f4,%f4
	fadds	%f2,%f4,%f4
	fmuls	%f0,%f4,%f4
	ba	exit
	fadds	%f2,%f4,%f0
two_m9:
	cmp	%o1,%o3
	ble	two_m14
	nop
	ld	[%l0+fhalf],%f2
	fmuls	%f0,%f2,%f2
	ld	[%l0+fone],%f4
	fadds	%f4,%f2,%f2
	fmuls	%f0,%f2,%f2
	ba	exit
	fadds	%f4,%f2,%f0
two_m14:
	ld	[%l0+fone],%f4
	ba	exit
	fadds	%f4,%f0,%f0

	.seg	"data"
        .align 8
constant:
invln2	= 0x00
	.double	0r1.44269504088896338700
ln2	= 0x08
	.double 0r6.93147180559945286227e-01
half	= 0x10
	.double	0r0.5
a1	= 0x18
	.word	0x3fe00008,0x80000000
a2	= 0x20
	.word	0x3fc5555d,0x80000000
ln2_32	= 0x28
	.word	0x3f962e42,0xfefa39ef
tiny	= 0x30
	.word	0x2b2bff2e,0xe48e0530
huge	= 0x38
	.word	0x54b249ad,0x2594c37d
fzero	= 0x40
	.single	0r0.0
fone	= 0x44
	.single	0r1.0
fhalf	= 0x48
	.word	0x3f000000
mfhalf	= 0x4c
	.word	0xbf000000
invln2_32 = 0x50
	.word	0x4238aa3b
p1	= 0x54
	.single	0r5.00000009512921380000e-01
p2	= 0x58
	.single	0r1.66665188973472840000e-01
p3	= 0x5c
	.single	0r4.16662059758234840000e-02
p4	= 0x60
	.single	0r8.36888310539362950000e-03
p5	= 0x64
	.single	0r1.39504796300460640000e-03
one	= 0x68
	.word	0x3ff00000,0x0


	.seg	"data"
	.align	8
Sexp:
	.word	0x3ff00000,0x0
	.word	0x3ff059b0,0xd3158574
	.word	0x3ff0b558,0x6cf9890f
	.word	0x3ff11301,0xd0125b51
	.word	0x3ff172b8,0x3c7d517b
	.word	0x3ff1d487,0x3168b9aa
	.word	0x3ff2387a,0x6e756238
	.word	0x3ff29e9d,0xf51fdee1
	.word	0x3ff306fe,0xa31b715
	.word	0x3ff371a7,0x373aa9cb
	.word	0x3ff3dea6,0x4c123422
	.word	0x3ff44e08,0x6061892d
	.word	0x3ff4bfda,0xd5362a27
	.word	0x3ff5342b,0x569d4f82
	.word	0x3ff5ab07,0xdd485429
	.word	0x3ff6247e,0xb03a5584
	.word	0x3ff6a09e,0x667f3bcc
	.word	0x3ff71f75,0xe8ec5f74
	.word	0x3ff7a114,0x73eb0187
	.word	0x3ff82589,0x994cce13
	.word	0x3ff8ace5,0x422aa0dc
	.word	0x3ff93737,0xb0cdc5e5
	.word	0x3ff9c491,0x82a3f090
	.word	0x3ffa5503,0xb23e255c
	.word	0x3ffae89f,0x995ad3ae
	.word	0x3ffb7f76,0xf2fb5e47
	.word	0x3ffc199b,0xdd85529c
	.word	0x3ffcb720,0xdcef9069
	.word	0x3ffd5818,0xdcfba487
	.word	0x3ffdfc97,0x337b9b5f
	.word	0x3ffea4af,0xa2a490da
	.word	0x3fff5076,0x5b6e4540
	.seg	"text"
