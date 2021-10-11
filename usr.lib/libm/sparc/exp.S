
	.seg	"data"
	.asciz	"@(#)exp.S 1.1 92/07/30 SMI"

#define LOCORE
#include <machine/asm_linkage.h>

! Copyright (c) 1989 by Sun Microsystems, Inc.
!
! double exp(x) 
! Algorithm see libm/C/exp.c.
!					-- K.C. NG (kcng@kcng)
!

! local variable using fp index
x 	= -0x8
tmp	= -0x10
k	= -0x18
	
! local usage of register
! i0 input high x
! i1 input low  x
! i2 high |x|
! i3 k
! i4 j
! i5 m
! f8 hi
! f10 lo
	.seg	"text"
	.global	_SVID_libm_err
	ENTRY(exp)
	save	%sp,-0xb0,%sp
	std	%i0,[%fp+x]
	set	constant,%l0
	ldd	[%fp+x],%f0
	sethi	%hi(0x80000000),%i2
	andn	%i0,%i2,%i2		! i2 = high |x|
	sethi	%hi(0x7ff00000),%o0
	cmp	%i2,%o0
	bl	finite
	ld	[%l0+ln2_64],%l4
    ! x is not finite
	andn	%i2,%o0,%o1
	orcc	%o1,%i1,%g0
	bne	TNaN
	nop
!	faddd	%f0,%f0,%f0		! NaN+NaN

	tst	%i0
	bge	exit			! return x when x = inf
	nop
	ba	exit
	ldd	[%l0+zero],%f0		! return 0.0 when x = -inf
finite:
	
	cmp	%i2,%l4
	ble	T1			!
	sethi	%hi(0x3ed00000),%l4	! 2**-18
    ! if |x| >= ln2/64
	ld	[%l0+ln2_onehalf],%l4
	mov	0,%i3			! set k = 0
	cmp	%i2,%l4
	bl,a	8f
	ld	[%l0+ln2_2],%l4
	bg,a	2f
	tst	%i0
	ld	[%l0+ln2_onehalf+4],%l4
	cmp	%i1,%l4
	bgu,a	2f
	tst	%i0
	ld	[%l0+ln2_2],%l4
8:
	cmp	%i2,%l4
	bg,a	9f
	tst	%i0
	bl	7f
	nop
	ld	[%l0+ln2_2+4],%l4
	cmp	%i2,%l4
	bleu	7f
	nop
9:
    ! now ln2/2 <= |x| < 1.5*ln2
	bl	1f
	ldd	[%l0+ln2hi],%f2
	fsubd	%f0,%f2,%f8
	ldd	[%l0+ln2lo],%f10
	fsubd	%f8,%f10,%f0		! x>0: f0 = x - ln2
	ba	7f
	mov	1,%i3			! k = 1
1:
	ldd	[%l0+ln2hi],%f2
	faddd	%f0,%f2,%f8
	ldd	[%l0+ln2lo],%f10
	faddd	%f8,%f10,%f0		! x<0: f0 = x + ln2
	mov	-1,%i3			! k = -1
	fnegs	%f10,%f10
7:
	fmuld	%f0,%f0,%f2		! f2 = t = z*z
	ldd	[%l0+p5],%f4
	fmuld	%f2,%f4,%f4		! f4 = t*p5
	ldd	[%l0+p4],%f6
	faddd	%f6,%f4,%f4		! f4 = p4+t*p5
	fmuld	%f2,%f4,%f4
	ldd	[%l0+p3],%f6
	faddd	%f6,%f4,%f4
	fmuld	%f2,%f4,%f4
	ldd	[%l0+p2],%f6
	faddd	%f6,%f4,%f4
	ldd	[%l0+two],%f14		! f14 = 2.0
	fmuld	%f2,%f4,%f4
	ldd	[%l0+p1],%f6
	faddd	%f6,%f4,%f4
	fmuld	%f2,%f4,%f4
	fsubd	%f0,%f4,%f4		! f4 = c = z - t*(p1+...)
	cmp	%i3,0
	bne	1f
	nop
    ! k = 0
	fmuld	%f0,%f4,%f2		! f2 = z*c
	fsubd	%f4,%f14,%f14		! f14 = c-2
	fdivd	%f2,%f14,%f14		! f14 = (z*c)/(c-2)
	fsubd	%f14,%f0,%f0
	ldd	[%l0+one],%f2
	ba	exit
	fsubd	%f2,%f0,%f0
1:
    ! f8=ho, f10=lo
	fmuld	%f0,%f4,%f2		! f2 = z*c
	fsubd	%f14,%f4,%f14		! f14 = 2-c
	fdivd	%f2,%f14,%f14		! f14 = (z*c)/(2-c)
	fsubd	%f10,%f14,%f14
	ldd	[%l0+one],%f2
	fsubd	%f14,%f8,%f14
	fsubd	%f2,%f14,%f0
	cmp	%i3,1
	be,a	exit
	faddd	%f0,%f0,%f0
	ld	[%l0+half],%f2
	ba	exit
	fmuld	%f2,%f0,%f0
2:
	bg,a	1f
	ld	[%l0+threshold1],%l4
	ld	[%l0+threshold2],%l4
1:
	cmp	%i2,%l4
	bl,a	2f
	ldd	[%l0+invln2_32],%f2
	bg	3f
	tst	%i0
	bg,a	1f
	ld	[%l0+threshold1+4],%l4
	ld	[%l0+threshold2+4],%l4	! check lower half of x
1:
	cmp	%i1,%l4
	bleu,a	2f
	ldd	[%l0+invln2_32],%f2
	tst	%i0
3:
    ! |x| > threshold
	bg	xflow			! overflow
	mov	6,%o4
	bl	xflow			! underflow
	mov	7,%o4
2:
    ! ln2_onehalf <= |x| <= threshold
	fmuld	%f0,%f2,%f4		! invln2_32*x
	ldd	[%l0+half],%f6
	tst	%i0
	bg,a	1f
	faddd	%f4,%f6,%f4
	fsubd	%f4,%f6,%f4
1:
	fdtoi	%f4,%f4
	fitod	%f4,%f6			! f6 = t = (double) k
	st	%f4,[%fp+k]
	ldd	[%l0+ln2_32hi],%f8
	fmuld	%f6,%f8,%f8
	ld	[%fp+k],%i3
	and	%i3,0x1f,%i4		! i4  = j  = k&0x1f
	fsubd	%f0,%f8,%f8		! f8  = hi = x-t*ln2_32hi
	ldd	[%l0+ln2_32lo],%f10
	fmuld	%f6,%f10,%f10		! f10 = lo = t*ln2_32lo
	sra	%i3,5,%i5		! i5  = m  = k>>5
	ba	primary
	fsubd	%f8,%f10,%f0		! f0  = hi - lo
T1:
	cmp	%i2,%l4			! l4 = 2**-18
	bge	2f
	sethi	%hi(0x3e300000),%l4	! 2**-28
	ldd	[%l0+huge],%f2
	faddd	%f0,%f2,%f2		! raise inexact flag if x != 0
	cmp	%i2,%l4
	bge	1f
	ldd	[%l0+one],%f4
    ! |x| < 2**-28 return 1+x
	ba	exit
	faddd	%f0,%f4,%f0
1:  ! 2**-28 <= |x| < 2**-18, return 1+x*(1+0.5*x)
	ldd	[%l0+half],%f6
	fmuld	%f0,%f6,%f6
	faddd	%f4,%f6,%f6
	fmuld	%f0,%f6,%f6
	ba	exit
	faddd	%f4,%f6,%f0
2:
	mov	0,%i4
	mov	0,%i5
primary:
	fmuld	%f0,%f0,%f2		! f2 = t = z*z
	sll	%i4,3,%i4		! j  = j<<3
	ldd	[%l0+t2],%f4
	fmuld	%f2,%f4,%f4		! f4 = t*t2
	ldd	[%l0+t1],%f6
	faddd	%f6,%f4,%f4		! f4 = t1+t*t2
	set	S,%l1
	fmuld	%f2,%f4,%f4		! f4 = t*(t1+t*t2)
	set	S2,%l2
	fsubd	%f0,%f4,%f4		! f4 = z - t*(t1+t*t2)
	ldd	[%l0+two],%f6
	fsubd	%f4,%f6,%f4		! f4 = (z - t*(t1+t*t2)) - 2.0
	ldd	[%l2+%i4],%f8
	fmuld	%f0,%f8,%f8		! f8 = S2[j]*z
	set	S_trail,%l3
	fdivd	%f8,%f4,%f4		! f4 = f8/f4
	ldd	[%l3+%i4],%f6		! f6 = S_trail[j]
	fsubd	%f4,%f6,%f4		! f4 = f4 - f6
	ldd	[%l1+%i4],%f8
	fsubd	%f8,%f4,%f0		! f0 = S[j]-f4
	tst	%i5
	be	exit
	cmp	%i5,-1021
	bl,a	subnormal
	add	%i5,54,%i5
    ! normal output
	sll	%i5,20,%i5
	st	%f0,[%fp+tmp]
	ld	[%fp+tmp],%l4
	add	%i5,%l4,%l4
	st	%l4,[%fp+tmp]
	ba	exit
	ld	[%fp+tmp],%f0
subnormal:
	sll	%i5,20,%i5
	st	%f0,[%fp+tmp]
	ld	[%fp+tmp],%l4
	add	%i5,%l4,%l4
	st	%l4,[%fp+tmp]
	ld	[%fp+tmp],%f0
	ldd	[%l0+twom54],%f2
	fmuld	%f2,%f0,%f0
exit:
	ret
	restore
xflow:
	mov	%i0,%o0
	mov	%i0,%o2
	mov	%i1,%o1
	call	_SVID_libm_err
	mov	%i1,%o3
	ba	exit
	nop

TNaN:
    !	ba	exit
    !	faddd	%f0,%f0,%f0		! trigger invalid if x is sNaN
    ! rewrite to avoid kernel trap when TEM=RD=0
	set     0x00080000,%l5
        andcc   %l5,%i0,%g0
        bne     exit                 	! quiet NaN
        nop
        or      %l5,%i0,%i0             ! change to quiet NaN
	st      %fsr,[%fp+tmp]
	ld      [%fp+tmp],%l4
	set     0xcf800000,%l5
	andcc   %l4,%l5,%g0
	bne	1f
	nop
    ! signaling NaN
        set     0x210,%l5
        or      %l5,%l4,%l4
        st      %l4,[%fp+tmp]
        ld      [%fp+tmp],%fsr
        st      %i0,[%fp+x]
        ba      exit
        ldd     [%fp+x],%f0
1:
   ! standard way to treat NaN
        ba      exit
        faddd   %f0,%f0,%f0

	.seg	"data"
	.align	8
constant:
threshold1	= 0x00
	.word	0x40862E42,0xFEFA39EF		! exp(x>threshold1)overflow
threshold2	= 0x08
	.word	0x40874910,0xD52D3051		! exp(x<-threshold1)underflow
ln2_onehalf	= 0x10
	.word	0x3FF0A2B2,0x3F3BAB73		! 1.5*ln2   chopped
ln2	= 0x18
	.word	0x3fe62e42,0xfefa39ef		! ln2   chopped
ln2_2	= 0x20
	.word	0x3fd62e42,0xfefa39ef		! ln2/2 chopped
ln2hi	= 0x28
	.word   0x3fe62e42,0xfee00000   	! ln2hi
ln2lo	= 0x30
	.word   0x3dea39ef,0x35793c76   	! ln2lo
two	= 0x38
	.double	0r2.0
one	= 0x40
	.double	0r1.0
half	= 0x48
	.double	0r0.5
p1	= 0x50
        .word   0x3fc55555,0x5555553e
p2	= 0x58
        .word   0xbf66c16c,0x16bebd93
p3	= 0x60
        .word   0x3f11566a,0xaf25de2c
p4	= 0x68
        .word   0xbebbbd41,0xc5d26bf1
p5	= 0x70
        .word   0x3e663769,0x72bea4d0
invln2	= 0x78
	.word   0x3ff71547,0x652b82fe
twom54	= 0x80
	.word   0x3c900000,0x00000000
zero	= 0x88
	.word   0x0,0x0
huge	= 0x90
	.double	0r1.0e30
ln2_64	= 0x98
	.word	0x3f862e42,0xfefa39ef
ln2_32hi= 0xa0
	.word	0x3f962e42,0xfee00000
ln2_32lo= 0xa8
	.word	0x3d9a39ef,0x35793c76
invln2_32 = 0xb0
	.word	0x40471547,0x652b82fe
twom18	= 0xb8
	.word	0x3ed00000,0x0
twom28	= 0xc0
	.word	0x3e300000,0x0
t1	= 0xc8
	.word	0x3fc55555,0x55551e29
t2	= 0xd0
	.word	0xbf66c166,0x4a3720a8
S:
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
	.word	0x3ff6247e,0xb03a5585
	.word	0x3ff6a09e,0x667f3bcd
	.word	0x3ff71f75,0xe8ec5f74
	.word	0x3ff7a114,0x73eb0187
	.word	0x3ff82589,0x994cce13
	.word	0x3ff8ace5,0x422aa0db
	.word	0x3ff93737,0xb0cdc5e5
	.word	0x3ff9c491,0x82a3f090
	.word	0x3ffa5503,0xb23e255d
	.word	0x3ffae89f,0x995ad3ad
	.word	0x3ffb7f76,0xf2fb5e47
	.word	0x3ffc199b,0xdd85529c
	.word	0x3ffcb720,0xdcef9069
	.word	0x3ffd5818,0xdcfba487
	.word	0x3ffdfc97,0x337b9b5f
	.word	0x3ffea4af,0xa2a490da
	.word	0x3fff5076,0x5b6e4540
S_trail:
	.word	0x0,0x0
	.word	0x3c8d73e2,0xa475b465
	.word	0x3c98a62e,0x4adc610a
	.word	0xbc96c510,0x39449b3a
	.word	0xbc819041,0xb9d78a76
	.word	0x3c9e016e,0xa2643c
	.word	0x3c99b07e,0xb6c70573
	.word	0x3c8612e8,0xafad1255
	.word	0x3c86f46a,0xd23182e4
	.word	0xbc963aea,0xbf42eae2
	.word	0x3c8ada09,0x11f09ebc
	.word	0x3c489b7a,0x4ef80d0
	.word	0x3c7d4397,0xafec42e2
	.word	0xbc807abe,0x1db13cac
	.word	0x3c96324c,0x54647ad
	.word	0xbc9383c1,0x7e40b497
	.word	0xbc9bdd34,0x13b26456
	.word	0xbc816e47,0x86887a99
	.word	0xbc841577,0xee04992f
	.word	0xbc9d4c1d,0xd41532d8
	.word	0x3c96e9f1,0x56864b27
	.word	0xbc675fc7,0x81b57ebc
	.word	0x3c7c7c46,0xb071f2be
	.word	0xbc9d2f6e,0xdb8d41e1
	.word	0x3c97a1cd,0x345dcc81
	.word	0xbc75584f,0x7e54ac3b
	.word	0x3c811065,0x895048dd
	.word	0x3c7503cb,0xd1e949db
	.word	0x3c82ed02,0xd75b3706
	.word	0xbc91a5cd,0x4f184b5c
	.word	0xbc9e9c23,0x179c2893
	.word	0x3c99d3e1,0x2dd8a18b
	.align	8
S2:
	.word	0x40000000,0x0
	.word	0x400059b0,0xd3158574
	.word	0x4000b558,0x6cf9890f
	.word	0x40011301,0xd0125b51
	.word	0x400172b8,0x3c7d517b
	.word	0x4001d487,0x3168b9aa
	.word	0x4002387a,0x6e756238
	.word	0x40029e9d,0xf51fdee1
	.word	0x400306fe,0xa31b715
	.word	0x400371a7,0x373aa9cb
	.word	0x4003dea6,0x4c123422
	.word	0x40044e08,0x6061892d
	.word	0x4004bfda,0xd5362a27
	.word	0x4005342b,0x569d4f82
	.word	0x4005ab07,0xdd485429
	.word	0x4006247e,0xb03a5585
	.word	0x4006a09e,0x667f3bcd
	.word	0x40071f75,0xe8ec5f74
	.word	0x4007a114,0x73eb0187
	.word	0x40082589,0x994cce13
	.word	0x4008ace5,0x422aa0db
	.word	0x40093737,0xb0cdc5e5
	.word	0x4009c491,0x82a3f090
	.word	0x400a5503,0xb23e255d
	.word	0x400ae89f,0x995ad3ad
	.word	0x400b7f76,0xf2fb5e47
	.word	0x400c199b,0xdd85529c
	.word	0x400cb720,0xdcef9069
	.word	0x400d5818,0xdcfba487
	.word	0x400dfc97,0x337b9b5f
	.word	0x400ea4af,0xa2a490da
	.word	0x400f5076,0x5b6e4540
	.seg	"text"
