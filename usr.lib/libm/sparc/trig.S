
 	.seg	"data"
	.asciz	"@(#)trig.S 1.1 92/07/30 SMI"

#define LOCORE
#include <machine/asm_linkage.h>

! Copyright (c) 1988 by Sun Microsystems, Inc.
!
! double sin(x), cos(x), tan(x), sincos(x)
! IEEE Double precision trig functions on Sunrise
!
! Algorithm: see libm/C/trig.c
!
	.seg	"text"
	ENTRY(sin)
	ba	trig
	mov	0,%o4
	ENTRY(cos)
	ba	trig
	mov	1,%o4
	ENTRY(tan)
	ba	trig
	mov	2,%o4
	ENTRY(sincos)
	ba	trig
	mov	3,%o4

! trig()

	.seg	"data"
	.global	_fp_pi
	.align	4
_fp_pi:	.word	1		! initialize fp_pi = 1 = fp_pi_66
        .align 8
constant:
thresh1	= 0x0
	.word   0x3fe0b70c,0x6d604dd5
thresh2	= 0x8
	.word   0x3fd0584c,0x22231902
invpio2	= 0x10
	.word   0x3fe45f30,0x6dc9c883
pio4	= 0x18
	.word   0x3fe921fb,0x54442d18
pio2	= 0x20
	.word   0x3ff921fb,0x54442d18
pio2_1	= 0x28
	.word   0x3ff921fb,0x54400000
pio2_1t	= 0x30
	.word   0x3dd0b461,0x1a626331
pio2_1t5= 0x38
	.word   0x3dd0b460,0x0
pio2_2	= 0x40
	.word   0x3dd0b461,0x1a600000
pio2_2t	= 0x48
	.word   0x3ba3198a,0x2e037073
pio2_3	= 0x50
	.word   0x3ba3198a,0x2e000000
pio2_3t	= 0x58
	.word   0x397b839a,0x252049c1
medium	= 0x60
	.double	0r1647099.0
tiny	= 0x68
	.double	0r1.0e-10
big	= 0x70
	.double	0r1.0e10
zero	= 0x78
	.double	0r0
one	= 0x80
	.double	0r1.0
half	= 0x88
	.double	0r0.5
c5_8	= 0x90
	.double	0r0.625
c3_8	= 0x98
	.double	0r0.375
c13_16	= 0xa0
	.double	0r0.8125
c3_16	= 0xa8
	.double	0r0.1875
c002246	= 0xb0
	.double	0r0.002246
c000085	= 0xb8
	.double	0r0.000085
c1_3	= 0xc0
	.double	0r0.33333333333333333333
c1_6	= 0xc8
	.double	0r0.16666666666666666666
c2_15	= 0xd0
	.double	0r0.13333333333333333333
c1_24	= 0xd8
	.double	0r0.041666666666666666666
c1_120	= 0xe0
	.double	0r0.0083333333333333333333
ctiny1	= 0xe8
	.double	0r0.00390625
ctiny2	= 0xf0
	.double	0r9.094947e-13
C:
	! C_inf
	.double 0r4.16666666666666019E-2, 0r-1.38888888888744744E-3
	.double 0r2.48015872896717822E-5, 0r-2.75573144009911252E-7
	.double 0r2.08757292566166689E-9, 0r-1.13599319556004135E-11
	.double 0r0,0r0
	! C_66 = C_inf
	.double 0r4.16666666666666019E-2, 0r-1.38888888888744744E-3
	.double 0r2.48015872896717822E-5, 0r-2.75573144009911252E-7
	.double 0r2.08757292566166689E-9, 0r-1.13599319556004135E-11
	.double 0r0,0r0
	! C_53
	.double 0r4.1666666666666504759E-2, 0r-1.3888888888865301516E-3
	.double 0r2.4801587269650015769E-5, 0r-2.7557304623183959811E-7
	.double 0r2.0873958177697780076E-9, 0r-1.1250289076471311557E-11
	.double 0r0,0r0
S:
	! S_inf
	.double 0r1.66666666666666796E-1, 0r-8.33333333333178931E-3
	.double 0r1.98412698361250482E-4, 0r-2.75573156035895542E-6
	.double 0r2.50510254394993115E-8, 0r-1.59108690260756780E-10
	.double 0r0,0r0
	! S_66 = S_inf
	.double 0r1.66666666666666796E-1, 0r-8.33333333333178931E-3
	.double 0r1.98412698361250482E-4, 0r-2.75573156035895542E-6
	.double 0r2.50510254394993115E-8, 0r-1.59108690260756780E-10
	.double 0r0,0r0
	! S_53
	.double 0r1.6666666666666463126E-1, 0r-8.3333333332992771264E-3
	.double 0r1.9841269816180999116E-4, 0r-2.7557309793219876880E-6
	.double 0r2.5050225177523807003E-8, 0r-1.5868926979889205164E-10
	.double 0r0,0r0

! temporary variable using fp pointer

x	= -0x8
y1	= -0x10
y2	= -0x18
t	= -0x20
z	= -0x28
n	= -0x30

! local register variable
! i0 = high x, 			input, will be changed to high |x|
! i1 = low x			input
! i2 = *s			input from sincos
! i3 = *c			input from sincos
! i4 = k			input 0-sin,1-cos,2-tan,3-sincos
! i5 = signx			local
! l0 = address of constant	local
! l1 = j			indicator of whether y2=0
! l2 = n
!
! f0 = x			local
! f12= x*x			local
! f22= y1
! f24= y2
	.seg	"text"
trig:	
	save	%sp,-0xc0,%sp
	set	constant,%l0		! l0 = address of constant
	st	%i0,[%fp+x]
	st	%i1,[%fp+4+x]
	sethi	%hi(0x80000000),%l5
	and	%i0,%l5,%i5		! i5 = signx
	andn	%i0,%l5,%i0		! i0 = high |x|
    ! if(!finite(x)) return NaN
	sethi	%hi(0x7ff00000),%l5
	and	%l5,%i0,%l4
	cmp	%l5,%l4
	bne	T1
	ldd	[%fp+x],%f0		! set f0 = x
	fsubd	%f0,%f0,%f0
	cmp	%i4,3
	bne	Treturn
	nop
	st	%f0,[%i2]
	st	%f1,[%i2+4]
	st	%f0,[%i3]
	ba	Treturn
	st	%f1,[%i3+4]
T1:
    ! if(t<0.002246) then ...
	ld	[%l0+c002246],%l5
	cmp	%i0,%l5
	bge	T2
	mov	0,%l1			! set j=0

    !     if(t<tiny) then ...
	ld	[%l0+tiny],%l5
	cmp	%i0,%l5
	bge	1f
	nop
	ldd	[%l0+big],%f2
	faddd	%f0,%f2,%f2		! create inexact flag
	cmp	%i4,3
	bne	2f
	nop
	ldd	[%l0+one],%f4		! k=3
	st	%f0,[%i2]
	st	%f1,[%i2+4]
	st	%f4,[%i3]
	ba	Treturn
	st	%f5,[%i3+4]
2:
	cmp	%i4,1			
	bne	Treturn			! k=0,2 sin(x)=tan(x)=x
	nop
	ba	Treturn
	ldd	[%l0+one],%f0		! k=1	cos(x)=1
1:
	fmuld	%f0,%f0,%f12		! f12 = z = x*x;

    !     if(t<0.000085) then ...
	ld	[%l0+c000085],%l5
	cmp	%i0,%l5
	bge	1f
	cmp	%i4,1
	bg	2f
	nop
	be	3f
	nop
	fmuld	%f0,%f12,%f10		! k = 0
	ldd	[%l0+c1_6],%f2
	fmuld	%f10,%f2,%f2
	ba	Treturn
	fsubd	%f0,%f2,%f0
3:
	ldd	[%l0+half],%f2		! k = 1
	fmuld	%f2,%f12,%f10
	ldd	[%l0+one],%f0
	ba	Treturn
	fsubd	%f0,%f10,%f0
2:
	cmp	%i4,3
	be	4f
	fmuld	%f0,%f12,%f10		! k = 2
	ldd	[%l0+c1_3],%f2
	fmuld	%f2,%f10,%f10
	ba	Treturn
	faddd	%f0,%f10,%f0
4:
	ldd	[%l0+half],%f4		! k = 3
	fmuld	%f12,%f4,%f4		! z*0.5
	ldd	[%l0+one],%f6
	fsubd	%f6,%f4,%f4		! 1-z*0.5
	ldd	[%l0+c1_6],%f8
	fmuld	%f10,%f8,%f8		! x*z*1/6
	fsubd	%f0,%f8,%f8		! x - x*z*1/6
	st	%f4,[%i3]
	st	%f5,[%i3+4]
	st	%f8,[%i2]
	ba	Treturn
	st	%f9,[%i2+4]
1:   ! now 0.002246 <= |x| < 0.00085
	bg	2f
	nop
	be	3f
	nop
	fmuld	%f0,%f12,%f10		! k = 0
	ldd	[%l0+c1_120],%f2
	fmuld	%f12,%f2,%f2
	ldd	[%l0+c1_6],%f4
	fsubd	%f4,%f2,%f2
	fmuld	%f10,%f2,%f2
	ba	Treturn
	fsubd	%f0,%f2,%f0
3:
	ldd	[%l0+c1_24],%f2		! k = 1
	fmuld	%f12,%f2,%f2
	ldd	[%l0+half],%f4
	fsubd	%f4,%f2,%f2
	fmuld	%f12,%f2,%f2
	ldd	[%l0+one],%f0
	ba	Treturn
	fsubd	%f0,%f2,%f0
2:
	cmp	%i4,3
	be	4f
	fmuld	%f12,%f0,%f10		! k = 2, f10 = x*z
	ldd	[%l0+c2_15],%f2
	fmuld	%f12,%f2,%f2
	ldd	[%l0+c1_3],%f4
	faddd	%f4,%f2,%f2
	fmuld	%f10,%f2,%f2
	ba	Treturn
	faddd	%f0,%f2,%f0
4:
	ldd	[%l0+c1_120],%f2	! k = 3
	ldd	[%l0+c1_24],%f4
	fmuld	%f12,%f2,%f2		! f2 = z/120
	ldd	[%l0+c1_6],%f6
	fsubd	%f6,%f2,%f2
	fmuld	%f10,%f2,%f2
	fsubd	%f0,%f2,%f2		! f2 = sin(x)
	fmuld	%f12,%f4,%f4
	ldd	[%l0+half],%f6
	fsubd	%f6,%f4,%f4
	fmuld	%f12,%f4,%f4
	ldd	[%l0+one],%f0
	st	%f2,[%i2]
	st	%f3,[%i2+4]
	fsubd	%f0,%f4,%f4		! f4 = cos(x)
	st	%f4,[%i3]
	ba	Treturn
	st	%f5,[%i3+4]
T2:
	mov	0,%l2			! set n = 0
	ldd	[%fp+x],%f22		! y1 = x

    ! if (t>pio4) then ...
	ld	[%l0+pio4],%l5
	cmp	%i0,%l5
	bl	T3
	nop
	bg	1f
	nop
	ld	[%l0+pio4+4],%l5
	cmp	%i1,%l5
	bleu	T3
	nop
1:
	set	base,%l4
	set	_fp_pi,%l3
	ld	[%l3],%l3
	sll	%l3,3,%l3		! l3 = (_fp_pi)<<3
	jmp	%l4+%l3
	nop
base:
	ba	pi_inf
	nop
	ba	pi_66
	nop
	ba	pi_53
	nop
pi_inf:
	mov	1,%l1			! j = 1
	ld	[%l0+pio2],%l5
	cmp	%i0,%l5
	bg	pi_inf_1
	fabss	%f0,%f0			! now f0 = |x|
	bl	1f
	nop
	ld	[%l0+pio2+4],%l5
	cmp	%i1,%l5
	bgu	pi_inf_1
	nop
1:
	ldd	[%l0+pio2_1],%f2
	fsubd	%f0,%f2,%f4		! z  = |x| - pio2_1
	ldd	[%l0+pio2_1t],%f6	! f6 = pio2_1t
	mov	1,%l2			! n = 1
	ld	[%l0+ctiny1],%l4
	fsubd	%f4,%f6,%f22		! y1 = z - pio2_1t
	st	%f4,[%fp+t]
	ld	[%fp+t],%l5
	sethi	%hi(0x80000000),%l3
	andn	%l5,%l3,%l5
	cmp	%l5,%l4
	ble	2f
	nop
	fsubd	%f4,%f22,%f8
	ba	4f
	fsubd	%f8,%f6,%f24		! y2 = (z-y1)-pio2_1t
2:
	ldd	[%l0+pio2_2],%f8
	fsubd	%f4,%f8,%f4		! z -= pio2_2
	ldd	[%l0+pio2_2t],%f6	! f6 = pio2_2t
	fsubd	%f4,%f6,%f22
	st	%f4,[%fp+t]
	ld	[%fp+t],%l5
	ld	[%l0+ctiny2],%l4
	andn	%l5,%l3,%l5
	cmp	%l5,%l4
	ble	3f
	nop
	fsubd	%f4,%f22,%f8
	ba	4f
	fsubd	%f8,%f6,%f24		! y2 = (z-y1)-pio2_2t
3:
	ldd	[%l0+pio2_3],%f8
	fsubd	%f4,%f8,%f4		! z -= pio2_3
	ldd	[%l0+pio2_3t],%f6
	fsubd	%f4,%f6,%f22		! y1 = z - pio2_3t
	fsubd	%f4,%f22,%f8
	fsubd	%f8,%f6,%f24		! y2 = (z-y1) -pio2_3t
4:
	tst	%i5
	be	T3
	nop
	mov	-1,%l2			! n = -1
	fnegs	%f22,%f22
	ba	T3
	fnegs	%f24,%f24

pi_inf_1:	
	ld	[%l0+medium],%l5
	cmp	%i0,%l5
	ble	1f
	nop
	or	%i0,%i5,%o0		! call argred for huge argument
	mov	%i1,%o1
	add	%fp,y1,%o2
	call	argred
	add	%fp,y2,%o3
	mov	%o0,%l2
	ldd	[%fp+y1],%f22
	ba	T3
	ldd	[%fp+y2],%f24
1:
    ! 	else if(t<=medium) then ...
	ldd	[%l0+invpio2],%f2
	fmuld	%f0,%f2,%f2
	ldd	[%l0+half],%f4
	faddd	%f4,%f2,%f2
	fdtoi	%f2,%f2
	fitod	%f2,%f4			! f4 = double n
	st	%f2,[%fp+n]
	ld	[%fp+n],%l2
	ldd	[%l0+pio2_1],%f6
	fmuld	%f4,%f6,%f6
	fsubd	%f0,%f6,%f6		! f6 = z = |x|-n*pio2_1
	ldd	[%l0+pio2_1t],%f8
	fmuld	%f4,%f8,%f8		! f8 = w = n*pio2_1t
	sethi	%hi(0x80000000),%l3
	ld	[%l0+ctiny1],%l5
	fsubd	%f6,%f8,%f22		! y1 = z - w
	st	%f6,[%fp+t]
	ld	[%fp+t],%l4
	andn	%l4,%l3,%l4
	cmp	%l4,%l5
	ble	2f
	nop
	fsubd	%f6,%f22,%f24
	ba	4f
	fsubd	%f24,%f8,%f24
2:
	ldd	[%l0+pio2_2],%f10
	fmuld	%f4,%f10,%f10
	fsubd	%f6,%f10,%f6		! f6 = z -= n*pio2_2
	ldd	[%l0+pio2_2t],%f8
	fmuld	%f4,%f8,%f8		! f8 = w  = n*pio2_2t
	ld	[%l0+ctiny2],%l5
	fsubd	%f6,%f8,%f22		! y1 = z - w
	st	%f6,[%fp+t]
	ld	[%fp+t],%l4
	andn	%l4,%l3,%l4
	cmp	%l4,%l5
	ble	3f
	nop
	fsubd	%f6,%f22,%f24
	ba	4f
	fsubd	%f24,%f8,%f24
3:
	ldd	[%l0+pio2_3],%f10
	fmuld	%f4,%f10,%f10
	fsubd	%f6,%f10,%f6		! f6 = z -= n*pio2_3
	ldd	[%l0+pio2_3t],%f8
	fmuld	%f4,%f8,%f8		! f8 = w  = n*pio2_3t
	fsubd	%f6,%f8,%f22		! y1 = z - w
	fsubd	%f6,%f22,%f24
	fsubd	%f24,%f8,%f24
4:
	tst	%i5
	be	T3
	nop
	sub	%g0,%l2,%l2		! n = -n
	fnegs	%f22,%f22
	ba	T3
	fnegs	%f24,%f24
pi_66:
	mov	1,%l1			! j = 1
	ld	[%l0+pio2],%l5
	cmp	%i0,%l5
	bg	pi_66_1
	fabss	%f0,%f0			! now f0 = |x|
	bl	1f
	nop
	ld	[%l0+pio2+4],%l5
	cmp	%i1,%l5
	bgu	pi_66_1
	nop
1:
	ldd	[%l0+pio2_1],%f2
	fsubd	%f0,%f2,%f4		! z  = |x| - pio2_1
	ldd	[%l0+pio2_2],%f6	! f6 = pio2_2
	mov	1,%l2			! n = 1
	fsubd	%f4,%f6,%f22		! y1 = z - pio2_2
	fsubd	%f4,%f22,%f24
	fsubd	%f24,%f6,%f24		! y2 = (z-y1)-pio2_2;
	tst	%i5
	be	T3
	nop
	fnegs	%f22,%f22
	mov	-1,%l2			! n = -1
	ba	T3
	fnegs	%f24,%f24
pi_66_1:
	ld	[%l0+medium],%l5
	cmp	%i0,%l5
	ble	1f
	nop
	or	%i0,%i5,%o0		! call argred for huge argument
	mov	%i1,%o1
	add	%fp,y1,%o2
	call	argred
	add	%fp,y2,%o3
	mov	%o0,%l2
	ldd	[%fp+y1],%f22
	ba	T3
	ldd	[%fp+y2],%f24
1:
    ! 	else if(t<=medium) then ...
	ldd	[%l0+invpio2],%f2
	fmuld	%f0,%f2,%f2
	ldd	[%l0+half],%f4
	faddd	%f4,%f2,%f2
	fdtoi	%f2,%f2
	fitod	%f2,%f4			! f4 = double n
	st	%f2,[%fp+n]
	ld	[%fp+n],%l2
	ldd	[%l0+pio2_1],%f6
	fmuld	%f4,%f6,%f6
	fsubd	%f0,%f6,%f6		! f6 = z = |x|-n*pio2_1
	ldd	[%l0+pio2_2],%f8
	fmuld	%f4,%f8,%f8		! f8 = w = n*pio2_2
	fsubd	%f6,%f8,%f22		! y1 = z - w
	fsubd	%f6,%f22,%f24
	fsubd	%f24,%f8,%f24
	tst	%i5
	be	T3
	nop
	fnegs	%f22,%f22
	sub	%g0,%l2,%l2		! n = -n
	ba	T3
	fnegs	%f24,%f24
pi_53:
	ld	[%l0+pio2],%l5
	cmp	%i0,%l5
	bg	pi_53_1
	fabss	%f0,%f0			! now f0 = |x|
	bl	1f
	nop
	ld	[%l0+pio2+4],%l5
	cmp	%i1,%l5
	bgu	pi_53_1
	nop
1:
	ldd	[%l0+pio2],%f2
	fsubd	%f0,%f2,%f22		! y1  = |x| - pio2
	tst	%i5
	be	T3
	mov	1,%l2
	mov	-1,%l2
	ba	T3
	fnegs	%f22,%f22
pi_53_1:
	ld	[%l0+medium],%l5
	cmp	%i0,%l5
	ble	1f
	nop
	or	%i0,%i5,%o0		! call argred for huge argument
	mov	%i1,%o1
	mov	1,%l1			! set j=1
	add	%fp,y1,%o2
	call	argred
	add	%fp,y2,%o3
	mov	%o0,%l2
	ldd	[%fp+y1],%f22
	ba	T3
	ldd	[%fp+y2],%f24
1:
    ! 	else if(t<=medium) then ...
	ldd	[%l0+invpio2],%f2
	fmuld	%f0,%f2,%f2
	ldd	[%l0+half],%f4
	faddd	%f4,%f2,%f2
	fdtoi	%f2,%f2
	fitod	%f2,%f4			! f4 = double n
	st	%f2,[%fp+n]
	ld	[%fp+n],%l2
	ldd	[%l0+pio2_1],%f6
	fmuld	%f4,%f6,%f6
	fsubd	%f0,%f6,%f6		! f6 = z = |x|-n*pio2_1
	ldd	[%l0+pio2_1t5],%f8
	fmuld	%f4,%f8,%f8		! f8 = w = n*pio2_1t5
	fsubd	%f6,%f8,%f22		! y1 = z - w
	tst	%i5
	be	T3
	nop
	fnegs	%f22,%f22
	sub	%g0,%l2,%l2		! n = -n
	fnegs	%f24,%f24
T3:
    ! redefine local variable:
    ! f12 = z = y1*y1
    ! l5  = _fp_pi<<6
	fmuld	%f22,%f22,%f12
	set	_fp_pi,%l5
	ld	[%l5],%l5
	sll	%l5,6,%l5		! l5 = (_fp_pi)<<6

    ! if(k<2) then ...	(sin,cos)
	cmp	%i4,2
	bge	T4
	nop
	add	%l2,%i4,%l2		! n += k;

    !     if ((n&1)==0) then ... (sin)
	andcc	%l2,1,%g0
	bne	T3_cos
	nop
	fmuld	%f22,%f12,%f8		! f8 = t = y1*z;
	set	S,%l4			! l4 = S
	add	%l4,%l5,%l4		! l4 = &S[m]
	ldd	[%l4+40],%f4		! f4 = S[m+5]
	fmuld	%f12,%f4,%f4		! f4 = z*S[m+5]
	ldd	[%l4+32],%f6		! f6 = S[m+4]
	faddd	%f6,%f4,%f4		! f4 = S[m+4]+z*S[m+5]
	fmuld	%f12,%f4,%f4
	ldd	[%l4+24],%f2		! f2 = S[m+3]
	faddd	%f2,%f4,%f4
	fmuld	%f12,%f4,%f4
	ldd	[%l4+16],%f6
	faddd	%f6,%f4,%f4
	fmuld	%f12,%f4,%f4
	ldd	[%l4+8],%f2
	faddd	%f2,%f4,%f4
	fmuld	%f12,%f4,%f4
	ldd	[%l4],%f6
	faddd	%f6,%f4,%f4		! f4 = ss = S[m]+ ...
	tst	%l1
	bne	1f
	nop
    !	    j==0
	fmuld	%f8,%f4,%f4
	ba	2f
	fsubd	%f22,%f4,%f0
1:
    !	    j!=0
	fmuld	%f24,%f12,%f0
	ldd	[%l0+half],%f2
	fmuld	%f2,%f0,%f0
	fsubd	%f24,%f0,%f0
	fmuld	%f8,%f4,%f4
	fsubd	%f4,%f0,%f0
	fsubd	%f22,%f0,%f0
2:
	andcc	%l2,2,%g0
	be	Treturn
	nop
	ba	Treturn
	fnegs	%f0,%f0
T3_cos:
	fmuld	%f12,%f12,%f8		! f8 = z*z
	set	C,%l4
	add	%l5,%l4,%l4		! l4 = &C[m]
	ldd	[%l4+40],%f4		! f4 = C[m+5]
	fmuld	%f12,%f4,%f4		! f4 = z*C[m+5]
	ldd	[%l4+32],%f6		! f6 = C[m+4]
	faddd	%f6,%f4,%f4		! f4 = C[m+4]+z*C[m+5]
	fmuld	%f12,%f4,%f4
	ldd	[%l4+24],%f2		! f2 = C[m+3]
	faddd	%f2,%f4,%f4
	fmuld	%f12,%f4,%f4
	ldd	[%l4+16],%f6
	faddd	%f6,%f4,%f4
	fmuld	%f12,%f4,%f4
	ldd	[%l4+8],%f2
	faddd	%f2,%f4,%f4
	fmuld	%f12,%f4,%f4
	ldd	[%l4],%f6
	faddd	%f6,%f4,%f4		! f4 = C[m] + ...
	fmuld	%f8,%f4,%f4		! f4 = cc = (z*z)*(C[m]+...)
	ldd	[%l0+half],%f2
	fmuld	%f12,%f2,%f2		! f2 = half*z
	tst	%l1
	be	1f
	nop
	fmuld	%f22,%f24,%f26
	fsubd	%f4,%f26,%f4		! f4 = cc -= y1*y2
1:
	std	%f12,[%fp+z]
	ld	[%l0+thresh1],%l3
	ld	[%fp+z],%l4

    !		if(z>=thresh1) then ...
	cmp	%l4,%l3
	bl	3f
	nop
	bg	2f
	ld	[%l0+thresh1+4],%l3
	ld	[%fp+z+4],%l4
	cmp	%l4,%l3
	blu	3f
	nop
2:
	ldd	[%l0+c3_8],%f6
	fsubd	%f2,%f6,%f6
	fsubd	%f6,%f4,%f4
	ldd	[%l0+c5_8],%f0
	fsubd	%f0,%f4,%f0
	andcc	%l2,2,%g0
	be	Treturn
	nop
	ba	Treturn
	fnegs	%f0,%f0
3:
	ld	[%l0+thresh2],%l3
	ld	[%fp+z],%l4

    !		if(z>=thresh2) then ...
	cmp	%l4,%l3
	bl	3f
	nop
	bg	2f
	ld	[%l0+thresh2+4],%l3
	ld	[%fp+z+4],%l4
	cmp	%l4,%l3
	blu	3f
	nop
2:
	ldd	[%l0+c3_16],%f6
	fsubd	%f2,%f6,%f6
	fsubd	%f6,%f4,%f4
	ldd	[%l0+c13_16],%f0
	fsubd	%f0,%f4,%f0
	andcc	%l2,2,%g0
	be	Treturn
	nop
	ba	Treturn
	fnegs	%f0,%f0
3:
	fsubd	%f2,%f4,%f4
	ldd	[%l0+one],%f0
	fsubd	%f0,%f4,%f0
	andcc	%l2,2,%g0
	be	Treturn
	nop
	ba	Treturn
	fnegs	%f0,%f0
T4:
    !   tan,sincos
	fmuld	%f12,%f12,%f10		! f10 = z*z
	set	S,%l4			! l4 = S
	add	%l4,%l5,%l4		! l4 = &S[m]
	ldd	[%l4+40],%f4		! f4 = S[m+5]
	fmuld	%f12,%f4,%f4		! f4 = z*S[m+5]
	ldd	[%l4+32],%f6		! f6 = S[m+4]
	faddd	%f6,%f4,%f4		! f4 = S[m+4]+z*S[m+5]
	fmuld	%f12,%f4,%f4
	ldd	[%l4+24],%f2		! f2 = S[m+3]
	faddd	%f2,%f4,%f4
	fmuld	%f12,%f4,%f4
	ldd	[%l4+16],%f6
	faddd	%f6,%f4,%f4
	fmuld	%f12,%f4,%f4
	ldd	[%l4+8],%f2
	faddd	%f2,%f4,%f4
	fmuld	%f12,%f4,%f4
	ldd	[%l4],%f6
	faddd	%f6,%f4,%f26		! f26 = ss = S[m]+...
	set	C,%l4			! l4 = C
	add	%l4,%l5,%l4		! l4 = &C[m]
	ldd	[%l4+40],%f4		! f4 = C[m+5]
	fmuld	%f12,%f4,%f4		! f4 = z*C[m+5]
	ldd	[%l4+32],%f6		! f6 = C[m+4]
	faddd	%f6,%f4,%f4		! f4 = C[m+4]+z*C[m+5]
	fmuld	%f12,%f4,%f4
	ldd	[%l4+24],%f2		! f2 = C[m+3]
	faddd	%f2,%f4,%f4
	fmuld	%f12,%f4,%f4
	ldd	[%l4+16],%f6
	faddd	%f6,%f4,%f4
	fmuld	%f12,%f4,%f4
	ldd	[%l4+8],%f2
	faddd	%f2,%f4,%f4
	fmuld	%f12,%f4,%f4
	ldd	[%l4],%f6
	faddd	%f6,%f4,%f4
	fmuld	%f10,%f4,%f28		! f28 = cc = (z*z)*(C[m]+...)
	ldd	[%l0+half],%f2
	fmuld	%f12,%f2,%f2		! f2  = hz = z/2

	cmp	%i4,2
	bne	1f
	std	%f12,[%fp+z]
	ba	2f
	fmuld	%f12,%f26,%f26		! if (k==2) ss *= z;
1:
	tst	%l1
	be	2f
	nop
	fmuld	%f22,%f24,%f20
	fsubd	%f28,%f20,%f28		! if (k==3) cc -= y1*y2
2:
	ld	[%l0+thresh1],%l3
	ld	[%fp+z],%l4

    !		if(z>=thresh1) then ...
	cmp	%l4,%l3
	bl	3f
	nop
	bg	2f
	ld	[%l0+thresh1+4],%l3
	ld	[%fp+z+4],%l4
	cmp	%l4,%l3
	blu	3f
	nop
2:
	ldd	[%l0+c3_8],%f6
	fsubd	%f2,%f6,%f6
	fsubd	%f6,%f28,%f4
	ldd	[%l0+c5_8],%f0
	ba	4f
	fsubd	%f0,%f4,%f0		! f0 = w = 5/8-((hz-3/8)-cc)
3:
	ld	[%l0+thresh2],%l3
	ld	[%fp+z],%l4

    !		if(z>=thresh2) then ...
	cmp	%l4,%l3
	bl	3f
	nop
	bg	2f
	ld	[%l0+thresh2+4],%l3
	ld	[%fp+z+4],%l4
	cmp	%l4,%l3
	blu	3f
	nop
2:
	ldd	[%l0+c3_16],%f6
	fsubd	%f2,%f6,%f6
	fsubd	%f6,%f28,%f4
	ldd	[%l0+c13_16],%f0
	ba	4f
	fsubd	%f0,%f4,%f0		! f0 = w = 13/16 -((hz-3/16)-cc)
3:
	fsubd	%f2,%f28,%f4
	ldd	[%l0+one],%f0
	fsubd	%f0,%f4,%f0		! f0 = w = 1 - (hz - cc)
4:
    !   if(k==2) then ... (tan)
	cmp	%i4,2
	bne	T4_sincos
	nop
    !		if((n&1)==0) then ... tan = sin/cos
	andcc	%l2,1,%g0
	bne	5f
	tst	%l1
	bne	1f
	nop
    ! 		j==0
	faddd	%f26,%f28,%f20
	fsubd	%f2,%f20,%f20
	fdivd	%f20,%f0,%f0
	fmuld	%f22,%f0,%f0
	ba	Treturn
	faddd	%f22,%f0,%f0
1:
    !		j==1
	faddd	%f26,%f28,%f20
	fsubd	%f20,%f2,%f20
	fdivd	%f20,%f0,%f0
	fmuld	%f22,%f24,%f18
	fsubd	%f0,%f18,%f0
	fmuld	%f22,%f0,%f0
	fsubd	%f0,%f24,%f0
	ba	Treturn
	fsubd	%f22,%f0,%f0
5:
    !		tan = -cos/sin
	bne	1f
	nop
    ! 		j==0
	fmuld	%f22,%f26,%f20
	fnegs	%f0,%f0
	fsubd	%f22,%f20,%f20
	ba	Treturn
	fdivd	%f0,%f20,%f0
1:
	fmuld	%f24,%f12,%f20
	faddd	%f24,%f20,%f20
	fmuld	%f0,%f20,%f20
	fmuld	%f22,%f26,%f18
	fnegs	%f0,%f0
	fsubd	%f18,%f20,%f20
	fsubd	%f22,%f20,%f20
	ba	Treturn
	fdivd	%f0,%f20,%f0
T4_sincos:
	fmuld	%f22,%f12,%f20
	tst	%l1
	bne	1f
	nop
	fmuld	%f20,%f26,%f20
	ba	2f
	fsubd	%f22,%f20,%f20
1:
	fmuld	%f2,%f24,%f18
	fsubd	%f24,%f18,%f18
	fmuld	%f20,%f26,%f20
	fsubd	%f20,%f18,%f20
	fsubd	%f22,%f20,%f20
2:
	andcc	%l2,3,%l2
	bne	3f
	cmp	%l2,1
	st	%f0,[%i3]		! n == 0
	st	%f1,[%i3+4]
	st	%f20,[%i2]
	ba	Treturn
	st	%f21,[%i2+4]
3:
	bne	4f
	cmp	%l2,2
	fnegs	%f20,%f20
	st	%f0,[%i2]		! n == 1
	st	%f1,[%i2+4]
	st	%f20,[%i3]
	ba	Treturn
	st	%f21,[%i3+4]
4:
	bg	5f
	nop
	fnegs	%f0,%f0
	st	%f0,[%i3]		! n == 2
	st	%f1,[%i3+4]
	fnegs	%f20,%f20
	st	%f20,[%i2]
	ba	Treturn
	st	%f21,[%i2+4]
5:
	fnegs	%f0,%f0
	st	%f0,[%i2]		! n == 1
	st	%f1,[%i2+4]
	st	%f20,[%i3]
	st	%f21,[%i3+4]
Treturn:
	ret
	restore

!!
!! argred()
!!
	.seg	"text"			! [internal]
argred:
	save	%sp,-376,%sp
	st	%i0,[%fp+68]
	st	%i1,[%fp+72]
	sethi	%hi(_one+4),%o1
	ld	[%o1+%lo(_one+4)],%o1
	sethi	%hi(0x3ff00000),%o2
	cmp	%o1,%o2
	mov	1,%i4
	bne	L77003
	add	%fp,-8,%i0
	mov	0,%i4
L77003:
	ld	[%fp+68],%o0
	st	%f30,[%sp+LP192]
	st	%f31,[%sp+LP192+4]
	call	_signbit,2
	mov	%i1,%o1
	ld	[%fp+72],%o1
	mov	%o0,%l7
	call	_fabs,2
	ld	[%fp+68],%o0
	fmovs	%f1,%f31
	fmovs	%f0,%f30
	std	%f30,[%sp+LP192+8]
	ld	[%sp+LP192+8],%o0
	ld	[%sp+LP192+12],%o1
	st	%f30,[%sp+LP192]
	st	%f31,[%sp+LP192+4]
	call	_ilogb,2
	nop
	ld	[%sp+LP192],%f30
	ld	[%sp+LP192+4],%f31
	dec	26,%o0
	call	.div,2
	mov	24,%o1
	mov	%o0,%i5
	tst	%i5
	bg,a	LY25
	std	%f30,[%sp+LP192+8]
	b	L77006
	mov	0,%i5
LY25:					! [internal]
	ld	[%sp+LP192+8],%o0
	ld	[%sp+LP192+12],%o1
	st	%f30,[%sp+LP192]
	st	%f31,[%sp+LP192+4]
	sub	%g0,%i5,%o2
	sll	%o2,3,%o2
	sll	%o2,1,%o3
	call	_scalbn,3
	add	%o2,%o3,%o2
	fmovs	%f1,%f31
	fmovs	%f0,%f30
L77006:
	std	%f30,[%fp-8]
	sll	%i4,2,%i4
	add	%i0,%i4,%i0
	ld	[%i0],%o7
	sethi	%hi(-0x20000000),%l2
	and	%o7,-32,%o7
	st	%o7,[%i0]
	ldd	[%fp-8],%f0
	std	%f0,[%fp-16]
	ld	[%i0],%l1
	sethi	%hi(_p3_inf),%l5
	and	%l1,%l2,%l1
	st	%l1,[%i0]
	ldd	[%fp-16],%f4
	ldd	[%fp-16],%f10
	fsubd	%f30,%f10,%f12
	ldd	[%fp-8],%f6
	ldd	[%fp-8],%f2
	std	%f2,[%fp-24]
	sethi	%hi(_fp_pi),%l6
	sethi	%hi(_p_inf),%l4
	add	%l4,%lo(_p_inf),%i4
	fsubd	%f4,%f6,%f8
	std	%f12,[%fp-16]
	std	%f8,[%fp-32]
	ldd	[%l5+%lo(_p3_inf)],%f14
	std	%f14,[%fp-40]
	ld	[%l6+%lo(_fp_pi)],%l6
	cmp	%l6,1
	bne,a	LY24
	sethi	%hi(_fp_pi),%o2
	sethi	%hi(_p3_66),%o1
	ldd	[%o1+%lo(_p3_66)],%f16
	std	%f16,[%fp-40]
	sethi	%hi(_p_66),%o0
	add	%o0,%lo(_p_66),%i4
	sethi	%hi(_fp_pi),%o2
LY24:					! [internal]
	ld	[%o2+%lo(_fp_pi)],%o2
	cmp	%o2,2
	bne,a	LY23
	cmp	%i5,1
	sethi	%hi(_p3_53),%o4
	ldd	[%o4+%lo(_p3_53)],%f18
	std	%f18,[%fp-40]
	sethi	%hi(_p_53),%o3
	add	%o3,%lo(_p_53),%i4
	cmp	%i5,1
LY23:					! [internal]
	bl,a	LY22
	sethi	%hi(_twon24),%l0	! [internal]
	sub	%i5,1,%o5
	sll	%o5,2,%o5
	ld	[%i4+%o5],%f19
	fitod	%f19,%f20
	cmp	%i5,1
	ble	L77014
	std	%f20,[%fp-48]
	sub	%i5,2,%l0
	sll	%l0,2,%l0
	ld	[%i4+%l0],%f21
	fitod	%f21,%f22
	sethi	%hi(_two24),%l2
	ldd	[%l2+%lo(_two24)],%f24
	fmuld	%f22,%f24,%f26
	std	%f26,[%fp-56]
L77014:
	sethi	%hi(_twon24),%l0	! [internal]
LY22:					! [internal]
	ldd	[%l0+%lo(_twon24)],%f28
	std	%f28,[%fp-64]
	ldd	[%fp-64],%f2
	ldd	[%fp-64],%f8
	sll	%i5,2,%l4
	ld	[%i4+%l4],%f29
	fitod	%f29,%f0
	ldd	[%l0+%lo(_twon24)],%f6
	fmuld	%f8,%f6,%f8
	fmuld	%f0,%f2,%f4
	std	%f8,[%fp-64]
	ldd	[%fp-64],%f12
	ldd	[%fp-64],%f18
	add	%i5,1,%l6
	sll	%l6,2,%l6
	ld	[%i4+%l6],%f9
	fitod	%f9,%f10
	fmovs	%f5,%f31
	fmovs	%f4,%f30
	fmuld	%f10,%f12,%f14
	add	%i5,2,%o2
	sll	%o2,2,%o2
	add	%i5,3,%o5
	sll	%o5,2,%o5
	add	%i5,4,%l1
	sll	%l1,2,%l1
	sethi	%hi(_zero),%l3
	std	%f14,[%fp-136]
	ldd	[%l0+%lo(_twon24)],%f16
	fmuld	%f18,%f16,%f18
	std	%f18,[%fp-64]
	ld	[%i4+%o2],%f19
	fitod	%f19,%f20
	ldd	[%fp-64],%f22
	ldd	[%fp-64],%f28
	fmuld	%f20,%f22,%f24
	ldd	[%fp-16],%f18
	std	%f24,[%fp-128]
	ldd	[%l0+%lo(_twon24)],%f26
	fmuld	%f28,%f26,%f28
	std	%f28,[%fp-64]
	ld	[%i4+%o5],%f29
	fitod	%f29,%f0
	ldd	[%fp-64],%f2
	ldd	[%fp-64],%f8
	fmuld	%f0,%f2,%f4
	std	%f4,[%fp-120]
	ldd	[%l0+%lo(_twon24)],%f6
	fmuld	%f8,%f6,%f8
	std	%f8,[%fp-64]
	ld	[%i4+%l1],%f9
	fitod	%f9,%f10
	ldd	[%fp-64],%f12
	fmuld	%f10,%f12,%f14
	std	%f14,[%fp-112]
	ld	[%l3+%lo(_zero)],%f15
	fitod	%f15,%f16
	fcmpd	%f18,%f16
	nop				! [internal]
	fbne,a	LY21
	cmp	%i5,1
	ldd	[%fp-32],%f22
	sethi	%hi(_zero),%l4
	ld	[%l4+%lo(_zero)],%f19
	fitod	%f19,%f20
	fcmpd	%f22,%f20
	nop				! [internal]
	fbne,a	LY20
	tst	%i5
	ldd	[%fp-24],%f24
	fmuld	%f24,%f30,%f26
	ldd	[%fp-24],%f28
	ldd	[%fp-136],%f0
	ldd	[%fp-24],%f4
	ldd	[%fp-128],%f6
	ldd	[%fp-24],%f10
	fmuld	%f28,%f0,%f2
	ldd	[%fp-120],%f12
	ldd	[%fp-24],%f16
	ldd	[%fp-112],%f18
	std	%f26,[%fp-224]
	fmuld	%f4,%f6,%f8
	std	%f2,[%fp-216]
	fmuld	%f10,%f12,%f14
	std	%f8,[%fp-208]
	fmuld	%f16,%f18,%f20
	std	%f14,[%fp-200]
	b	L77029
	std	%f20,[%fp-192]
LY20:					! [internal]
	ble,a	LY19
	ldd	[%fp-24],%f4
	ldd	[%fp-32],%f22
	ldd	[%fp-48],%f24
	fmuld	%f22,%f24,%f26
	ldd	[%fp-24],%f28
	fmuld	%f28,%f30,%f0
	faddd	%f0,%f26,%f2
	b	L77020
	std	%f2,[%fp-224]
LY19:					! [internal]
	fmuld	%f4,%f30,%f6
	std	%f6,[%fp-224]
L77020:
	ldd	[%fp-32],%f8
	fmuld	%f8,%f30,%f10
	ldd	[%fp-24],%f12
	ldd	[%fp-136],%f14
	ldd	[%fp-32],%f20
	ldd	[%fp-136],%f22
	ldd	[%fp-24],%f26
	fmuld	%f12,%f14,%f16
	ldd	[%fp-128],%f28
	ldd	[%fp-32],%f4
	ldd	[%fp-128],%f6
	faddd	%f16,%f10,%f18
	fmuld	%f20,%f22,%f24
	ldd	[%fp-120],%f12
	std	%f18,[%fp-216]
	ldd	[%fp-24],%f10
	ldd	[%fp-32],%f18
	fmuld	%f26,%f28,%f0
	ldd	[%fp-120],%f20
	faddd	%f0,%f24,%f2
	fmuld	%f4,%f6,%f8
	ldd	[%fp-112],%f26
	ldd	[%fp-24],%f24
	std	%f2,[%fp-208]
	fmuld	%f10,%f12,%f14
	fmuld	%f18,%f20,%f22
	faddd	%f14,%f8,%f16
	fmuld	%f24,%f26,%f28
	std	%f16,[%fp-200]
	faddd	%f28,%f22,%f0
	b	L77029
	std	%f0,[%fp-192]
LY21:					! [internal]
	bl,a	LY18
	ldd	[%fp-32],%f28
	cmp	%i5,1
	ble,a	LY17
	ldd	[%fp-32],%f22
	ldd	[%fp-32],%f2
	ldd	[%fp-48],%f4
	fmuld	%f2,%f4,%f6
	ldd	[%fp-24],%f8
	ldd	[%fp-16],%f14
	ldd	[%fp-56],%f16
	fmuld	%f8,%f30,%f10
	fmuld	%f14,%f16,%f18
	faddd	%f10,%f6,%f12
	faddd	%f12,%f18,%f20
	b	L77026
	std	%f20,[%fp-224]
LY17:					! [internal]
	ldd	[%fp-48],%f24
	fmuld	%f22,%f24,%f26
	ldd	[%fp-24],%f28
	fmuld	%f28,%f30,%f0
	faddd	%f0,%f26,%f2
	std	%f2,[%fp-224]
L77026:
	ldd	[%fp-32],%f4
	fmuld	%f4,%f30,%f6
	ldd	[%fp-24],%f8
	ldd	[%fp-136],%f10
	ldd	[%fp-16],%f16
	ldd	[%fp-48],%f18
	fmuld	%f8,%f10,%f12
	fmuld	%f16,%f18,%f20
	faddd	%f12,%f6,%f14
	faddd	%f14,%f20,%f22
	b	L77028
	std	%f22,[%fp-216]
LY18:					! [internal]
	fmuld	%f28,%f30,%f0
	ldd	[%fp-24],%f2
	ldd	[%fp-136],%f4
	ldd	[%fp-24],%f24
	fmuld	%f2,%f4,%f6
	fmuld	%f24,%f30,%f26
	faddd	%f6,%f0,%f8
	std	%f26,[%fp-224]
	std	%f8,[%fp-216]
L77028:
	ldd	[%fp-32],%f10
	ldd	[%fp-136],%f12
	fmuld	%f10,%f12,%f14
	ldd	[%fp-24],%f16
	ldd	[%fp-128],%f18
	ldd	[%fp-16],%f24
	ldd	[%fp-32],%f0
	ldd	[%fp-128],%f2
	fmuld	%f16,%f18,%f20
	ldd	[%fp-24],%f6
	ldd	[%fp-120],%f8
	faddd	%f20,%f14,%f22
	fmuld	%f24,%f30,%f26
	ldd	[%fp-136],%f16
	fmuld	%f0,%f2,%f4
	faddd	%f22,%f26,%f28
	ldd	[%fp-16],%f14
	ldd	[%fp-120],%f24
	fmuld	%f6,%f8,%f10
	ldd	[%fp-112],%f0
	std	%f28,[%fp-208]
	ldd	[%fp-32],%f22
	ldd	[%fp-24],%f28
	faddd	%f10,%f4,%f12
	fmuld	%f14,%f16,%f18
	ldd	[%fp-128],%f8
	ldd	[%fp-16],%f6
	fmuld	%f22,%f24,%f26
	faddd	%f12,%f18,%f20
	fmuld	%f28,%f0,%f2
	std	%f20,[%fp-200]
	fmuld	%f6,%f8,%f10
	faddd	%f2,%f26,%f4
	faddd	%f4,%f10,%f12
	std	%f12,[%fp-192]
L77029:
	ldd	[%fp-224],%f16
	sethi	%hi(L2000000),%l1	! [internal]
	ldd	[%l1+%lo(L2000000)],%f14
	fmuld	%f16,%f14,%f18
	sethi	%hi(L2000001),%l0	! [internal]
	st	%f18,[%sp+LP192+8]
	ld	[%sp+LP192+8],%o0
	st	%f19,[%sp+LP192+8]
	ld	[%sp+LP192+8],%o1
	st	%f30,[%sp+LP192]
	st	%f31,[%sp+LP192+4]
	call	_aint,2
	nop
	ldd	[%l0+%lo(L2000001)],%f20
	fmuld	%f20,%f0,%f22
	ldd	[%fp-224],%f24
	ldd	[%fp-216],%f28
	fsubd	%f24,%f22,%f24
	std	%f24,[%fp-224]
	ldd	[%l1+%lo(L2000000)],%f26
	fmuld	%f28,%f26,%f0
	st	%f0,[%sp+LP192+8]
	ld	[%sp+LP192+8],%o0
	st	%f1,[%sp+LP192+8]
	call	_aint,2
	ld	[%sp+LP192+8],%o1
	ldd	[%l0+%lo(L2000001)],%f2
	fmuld	%f2,%f0,%f4
	ldd	[%fp-216],%f6
	ldd	[%fp-208],%f10
	fsubd	%f6,%f4,%f6
	std	%f6,[%fp-216]
	ldd	[%l1+%lo(L2000000)],%f8
	fmuld	%f10,%f8,%f12
	fdtoi	%f12,%f13
	st	%f13,[%sp+LP192+8]
	ld	[%sp+LP192+8],%i1
	tst	%i1
	be,a	LY16
	ldd	[%fp-192],%f20
	sll	%i1,3,%i1
	st	%i1,[%sp+LP192+8]
	ld	[%sp+LP192+8],%f14
	fitod	%f14,%f16
	ldd	[%fp-208],%f18
	fsubd	%f18,%f16,%f18
	std	%f18,[%fp-208]
	ldd	[%fp-192],%f20
LY16:					! [internal]
	ldd	[%fp-200],%f22
	faddd	%f20,%f22,%f24
	ldd	[%fp-208],%f26
	ldd	[%fp-216],%f0
	ldd	[%fp-224],%f4
	ldd	[%fp-224],%f8
	ldd	[%fp-216],%f12
	fmovs	%f25,%f31
	fmovs	%f24,%f30
	faddd	%f30,%f26,%f28
	ldd	[%fp-208],%f16
	ldd	[%fp-200],%f20
	ldd	[%fp-192],%f24
	sethi	%hi(L2000002),%o4
	fmovs	%f29,%f31
	fmovs	%f28,%f30
	faddd	%f30,%f0,%f2
	fmovs	%f3,%f31
	fmovs	%f2,%f30
	faddd	%f30,%f4,%f6
	fmovs	%f7,%f31
	fmovs	%f6,%f30
	fsubd	%f8,%f30,%f10
	std	%f10,[%fp-240]
	ldd	[%fp-240],%f14
	faddd	%f14,%f12,%f14
	std	%f14,[%fp-240]
	ldd	[%fp-240],%f18
	faddd	%f18,%f16,%f18
	std	%f18,[%fp-240]
	ldd	[%fp-240],%f22
	faddd	%f22,%f20,%f22
	std	%f22,[%fp-240]
	ldd	[%fp-240],%f26
	faddd	%f26,%f24,%f26
	std	%f26,[%fp-240]
	fdtoi	%f30,%f27
	st	%f27,[%sp+LP192+8]
	ld	[%sp+LP192+8],%i1
	st	%i1,[%sp+LP192+8]
	ld	[%sp+LP192+8],%f28
	fitod	%f28,%f0
	ldd	[%fp-240],%f6
	ldd	[%fp-240],%f14
	fsubd	%f30,%f0,%f2
	std	%f2,[%fp-232]
	ldd	[%fp-232],%f4
	faddd	%f4,%f6,%f8
	ldd	[%fp-232],%f10
	fmovs	%f9,%f31
	fmovs	%f8,%f30
	fsubd	%f10,%f30,%f12
	faddd	%f14,%f12,%f14
	std	%f14,[%fp-240]
	ldd	[%o4+%lo(L2000002)],%f16
	fcmped	%f30,%f16
	nop				! [internal]
	fbg,a	LY15
	ldd	[%fp-240],%f0
	sethi	%hi(L2000002),%o5
	ldd	[%o5+%lo(L2000002)],%f18
	fcmpd	%f30,%f18
	nop				! [internal]
	fbne,a	LY14
	ld	[%fp-196],%o1
	ldd	[%fp-240],%f22
	sethi	%hi(_zero),%o7
	ld	[%o7+%lo(_zero)],%f19
	fitod	%f19,%f20
	fcmped	%f22,%f20
	nop				! [internal]
	fbule,a	LY14
	ld	[%fp-196],%o1
	ldd	[%fp-240],%f0
LY15:					! [internal]
	ldd	[%fp-240],%f8
	sethi	%hi(L2000003),%l0
	ldd	[%l0+%lo(L2000003)],%f24
	fsubd	%f30,%f24,%f26
	inc	%i1
	std	%f26,[%fp-232]
	ldd	[%fp-232],%f28
	faddd	%f28,%f0,%f2
	ldd	[%fp-232],%f4
	fmovs	%f3,%f31
	fmovs	%f2,%f30
	fsubd	%f4,%f30,%f6
	faddd	%f8,%f6,%f8
	std	%f8,[%fp-240]
	ld	[%fp-196],%o1
LY14:					! [internal]
	ld	[%fp-200],%o0
	st	%f30,[%sp+LP192]
	st	%f31,[%sp+LP192+4]
	call	_ilogb,2
	nop
	ld	[%sp+LP192],%f30
	ld	[%sp+LP192+4],%f31
	std	%f30,[%sp+LP192+8]
	mov	%o0,%l6
	ld	[%sp+LP192+8],%o0
	ld	[%sp+LP192+12],%o1
	st	%f30,[%sp+LP192]
	call	_ilogb,2
	nop
	ld	[%sp+LP192],%f30
	ld	[%sp+LP192+4],%f31
	sub	%o0,%l6,%o0
	cmp	%o0,20
	bge,a	LY13
	std	%f30,[%fp-8]
	st	%i1,[%sp+LP192+8]
	ld	[%sp+LP192+8],%f9
	fitod	%f9,%f10
	ldd	[%fp-224],%f12
	ldd	[%fp-216],%f16
	fsubd	%f12,%f10,%f14
	ldd	[%fp-216],%f26
	ldd	[%fp-208],%f0
	sethi	%hi(_twon24),%l2
	add	%i5,5,%l3
	sll	%l3,2,%l3
	sethi	%hi(_zero),%l5
	std	%f14,[%fp-232]
	ldd	[%fp-232],%f18
	faddd	%f16,%f18,%f20
	ldd	[%fp-232],%f22
	fmovs	%f21,%f31
	fmovs	%f20,%f30
	fsubd	%f30,%f22,%f24
	std	%f30,[%fp-232]
	ldd	[%fp-232],%f8
	ldd	[%fp-200],%f16
	fsubd	%f24,%f26,%f28
	fsubd	%f0,%f28,%f2
	std	%f2,[%fp-240]
	ldd	[%fp-240],%f4
	faddd	%f4,%f30,%f6
	ldd	[%fp-240],%f12
	ldd	[%fp-192],%f2
	fmovs	%f7,%f31
	fmovs	%f6,%f30
	fsubd	%f30,%f8,%f10
	std	%f30,[%fp-232]
	ldd	[%fp-232],%f24
	fsubd	%f10,%f12,%f14
	ldd	[%fp-64],%f8
	fsubd	%f16,%f14,%f18
	std	%f18,[%fp-240]
	ldd	[%fp-240],%f20
	faddd	%f20,%f30,%f22
	ldd	[%fp-240],%f28
	ldd	[%fp-16],%f18
	fmovs	%f23,%f31
	fmovs	%f22,%f30
	fsubd	%f30,%f24,%f26
	fsubd	%f26,%f28,%f0
	fsubd	%f2,%f0,%f4
	std	%f4,[%fp-240]
	ldd	[%l2+%lo(_twon24)],%f6
	fmuld	%f8,%f6,%f8
	std	%f8,[%fp-64]
	ld	[%i4+%l3],%f9
	fitod	%f9,%f10
	ldd	[%fp-64],%f12
	fmuld	%f10,%f12,%f14
	std	%f14,[%fp-104]
	ld	[%l5+%lo(_zero)],%f15
	fitod	%f15,%f16
	fcmpd	%f18,%f16
	nop				! [internal]
	fbne,a	LY12
	ldd	[%fp-32],%f14
	ldd	[%fp-32],%f22
	sethi	%hi(_zero),%l6
	ld	[%l6+%lo(_zero)],%f19
	fitod	%f19,%f20
	fcmpd	%f22,%f20
	nop				! [internal]
	fbne,a	LY11
	ldd	[%fp-32],%f0
	ldd	[%fp-24],%f24
	ldd	[%fp-104],%f26
	fmuld	%f24,%f26,%f28
	b	L77043
	std	%f28,[%fp-184]
LY11:					! [internal]
	ldd	[%fp-112],%f2
	fmuld	%f0,%f2,%f4
	ldd	[%fp-24],%f6
	ldd	[%fp-104],%f8
	fmuld	%f6,%f8,%f10
	faddd	%f10,%f4,%f12
	b	L77043
	std	%f12,[%fp-184]
LY12:					! [internal]
	ldd	[%fp-112],%f16
	fmuld	%f14,%f16,%f18
	ldd	[%fp-24],%f20
	ldd	[%fp-104],%f22
	ldd	[%fp-16],%f28
	ldd	[%fp-120],%f0
	fmuld	%f20,%f22,%f24
	fmuld	%f28,%f0,%f2
	faddd	%f24,%f18,%f26
	faddd	%f26,%f2,%f4
	std	%f4,[%fp-184]
L77043:
	ld	[%fp-188],%o1
	ld	[%fp-192],%o0
	st	%f30,[%sp+LP192]
	st	%f31,[%sp+LP192+4]
	call	_ilogb,2
	nop
	ld	[%sp+LP192],%f30
	ld	[%sp+LP192+4],%f31
	std	%f30,[%sp+LP192+8]
	mov	%o0,%l6
	ld	[%sp+LP192+8],%o0
	ld	[%sp+LP192+12],%o1
	st	%f30,[%sp+LP192]
	call	_ilogb,2
	nop
	ld	[%sp+LP192],%f30
	ld	[%sp+LP192+4],%f31
	sub	%o0,%l6,%o0
	cmp	%o0,20
	bl,a	LY10
	std	%f30,[%fp-232]
	ldd	[%fp-184],%f6
	ldd	[%fp-240],%f8
	faddd	%f8,%f6,%f8
	b	L77060
	std	%f8,[%fp-240]
LY10:					! [internal]
	ldd	[%fp-240],%f10
	faddd	%f30,%f10,%f30
	ldd	[%fp-232],%f12
	ldd	[%fp-240],%f16
	ldd	[%fp-184],%f20
	ldd	[%fp-64],%f26
	fsubd	%f30,%f12,%f14
	ldd	[%fp-16],%f6
	sethi	%hi(_twon24),%o1
	add	%i5,6,%o2
	sll	%o2,2,%o2
	sethi	%hi(_zero),%o4
	fsubd	%f14,%f16,%f18
	fsubd	%f20,%f18,%f22
	std	%f22,[%fp-240]
	ldd	[%o1+%lo(_twon24)],%f24
	fmuld	%f26,%f24,%f26
	std	%f26,[%fp-64]
	ld	[%i4+%o2],%f27
	fitod	%f27,%f28
	ldd	[%fp-64],%f0
	fmuld	%f28,%f0,%f2
	std	%f2,[%fp-96]
	ld	[%o4+%lo(_zero)],%f3
	fitod	%f3,%f4
	fcmpd	%f6,%f4
	nop				! [internal]
	fbne,a	LY9
	ldd	[%fp-32],%f2
	ldd	[%fp-32],%f10
	sethi	%hi(_zero),%o5
	ld	[%o5+%lo(_zero)],%f7
	fitod	%f7,%f8
	fcmpd	%f10,%f8
	nop				! [internal]
	fbne,a	LY8
	ldd	[%fp-32],%f18
	ldd	[%fp-24],%f12
	ldd	[%fp-96],%f14
	fmuld	%f12,%f14,%f16
	b	L77051
	std	%f16,[%fp-176]
LY8:					! [internal]
	ldd	[%fp-104],%f20
	fmuld	%f18,%f20,%f22
	ldd	[%fp-24],%f24
	ldd	[%fp-96],%f26
	fmuld	%f24,%f26,%f28
	faddd	%f28,%f22,%f0
	b	L77051
	std	%f0,[%fp-176]
LY9:					! [internal]
	ldd	[%fp-104],%f4
	fmuld	%f2,%f4,%f6
	ldd	[%fp-24],%f8
	ldd	[%fp-96],%f10
	ldd	[%fp-16],%f16
	ldd	[%fp-112],%f18
	fmuld	%f8,%f10,%f12
	fmuld	%f16,%f18,%f20
	faddd	%f12,%f6,%f14
	faddd	%f14,%f20,%f22
	std	%f22,[%fp-176]
L77051:
	ld	[%fp-180],%o1
	ld	[%fp-184],%o0
	st	%f30,[%sp+LP192]
	st	%f31,[%sp+LP192+4]
	call	_ilogb,2
	nop
	ld	[%sp+LP192],%f30
	ld	[%sp+LP192+4],%f31
	std	%f30,[%sp+LP192+8]
	mov	%o0,%l6
	ld	[%sp+LP192+8],%o0
	ld	[%sp+LP192+12],%o1
	st	%f30,[%sp+LP192]
	call	_ilogb,2
	nop
	ld	[%sp+LP192],%f30
	ld	[%sp+LP192+4],%f31
	sub	%o0,%l6,%o0
	cmp	%o0,20
	bl,a	LY7
	std	%f30,[%fp-232]
	ldd	[%fp-176],%f24
	ldd	[%fp-240],%f26
	faddd	%f26,%f24,%f26
	b	L77060
	std	%f26,[%fp-240]
LY7:					! [internal]
	ldd	[%fp-240],%f28
	faddd	%f30,%f28,%f30
	ldd	[%fp-232],%f0
	ldd	[%fp-240],%f4
	ldd	[%fp-176],%f8
	ldd	[%fp-64],%f14
	fsubd	%f30,%f0,%f2
	ldd	[%fp-16],%f24
	sethi	%hi(_twon24),%l0
	inc	7,%i5
	sll	%i5,2,%i5
	sethi	%hi(_zero),%l3
	fsubd	%f2,%f4,%f6
	fsubd	%f8,%f6,%f10
	std	%f10,[%fp-240]
	ldd	[%l0+%lo(_twon24)],%f12
	fmuld	%f14,%f12,%f14
	std	%f14,[%fp-64]
	ld	[%i4+%i5],%f15
	fitod	%f15,%f16
	ldd	[%fp-64],%f18
	fmuld	%f16,%f18,%f20
	std	%f20,[%fp-88]
	ld	[%l3+%lo(_zero)],%f21
	fitod	%f21,%f22
	fcmpd	%f24,%f22
	nop				! [internal]
	fbne,a	LY6
	ldd	[%fp-32],%f20
	ldd	[%fp-32],%f28
	sethi	%hi(_zero),%l4
	ld	[%l4+%lo(_zero)],%f25
	fitod	%f25,%f26
	fcmpd	%f28,%f26
	nop				! [internal]
	fbne,a	LY5
	ldd	[%fp-32],%f6
	ldd	[%fp-24],%f0
	ldd	[%fp-88],%f2
	fmuld	%f0,%f2,%f4
	b	L77059
	std	%f4,[%fp-168]
LY5:					! [internal]
	ldd	[%fp-96],%f8
	fmuld	%f6,%f8,%f10
	ldd	[%fp-24],%f12
	ldd	[%fp-88],%f14
	fmuld	%f12,%f14,%f16
	faddd	%f16,%f10,%f18
	b	L77059
	std	%f18,[%fp-168]
LY6:					! [internal]
	ldd	[%fp-96],%f22
	fmuld	%f20,%f22,%f24
	ldd	[%fp-24],%f26
	ldd	[%fp-88],%f28
	ldd	[%fp-16],%f4
	ldd	[%fp-104],%f6
	fmuld	%f26,%f28,%f0
	fmuld	%f4,%f6,%f8
	faddd	%f0,%f24,%f2
	faddd	%f2,%f8,%f10
	std	%f10,[%fp-168]
L77059:
	ldd	[%fp-168],%f12
	ldd	[%fp-240],%f14
	faddd	%f14,%f12,%f14
	std	%f14,[%fp-240]
L77060:
	std	%f30,[%fp-232]
	ldd	[%fp-240],%f16
	faddd	%f16,%f30,%f18
	ldd	[%fp-232],%f20
	ldd	[%fp-240],%f24
	fmovs	%f19,%f31
	fmovs	%f18,%f30
	fsubd	%f30,%f20,%f22
	fsubd	%f24,%f22,%f24
	std	%f24,[%fp-240]
	std	%f30,[%fp-8]
LY13:					! [internal]
	ldd	[%fp-240],%f28
	ldd	[%fp-8],%f26
	std	%f28,[%fp-232]
	std	%f26,[%fp-64]
	ld	[%i0],%l5
	sethi	%hi(-0x20000000),%o1
	and	%l5,-32,%l5
	st	%l5,[%i0]
	ldd	[%fp-8],%f30
	ld	[%i0],%o0
	cmp	%l7,1
	and	%o0,%o1,%o0
	st	%o0,[%i0]
	ldd	[%fp-8],%f2
	fsubd	%f30,%f2,%f4
	ldd	[%fp-8],%f0
	std	%f0,[%fp-248]
	ldd	[%fp-64],%f6
	ldd	[%fp-40],%f14
	ldd	[%fp-248],%f16
	fsubd	%f30,%f6,%f8
	fmuld	%f14,%f16,%f18
	std	%f4,[%fp-240]
	ldd	[%fp-240],%f22
	ldd	[%fp-232],%f10
	fsubd	%f10,%f8,%f12
	ldd	[%fp-248],%f6
	sethi	%hi(_p2),%o1		! [internal]
	ldd	[%o1+%lo(_p2)],%f20
	fmuld	%f20,%f22,%f24
	sethi	%hi(_p1),%o0		! [internal]
	fmovs	%f13,%f31
	fmovs	%f12,%f30
	ldd	[%fp-240],%f12
	faddd	%f24,%f18,%f26
	ldd	[%fp-248],%f20
	std	%f26,[%fp-256]
	ldd	[%o0+%lo(_p1)],%f28
	fmuld	%f28,%f30,%f0
	ldd	[%fp-256],%f2
	faddd	%f2,%f0,%f2
	std	%f2,[%fp-256]
	ldd	[%o1+%lo(_p2)],%f4
	fmuld	%f4,%f6,%f8
	ldd	[%o0+%lo(_p1)],%f10
	fmuld	%f10,%f12,%f14
	ldd	[%fp-256],%f6
	faddd	%f14,%f8,%f16
	std	%f16,[%fp-264]
	ldd	[%o0+%lo(_p1)],%f18
	fmuld	%f18,%f20,%f22
	ldd	[%fp-264],%f24
	ldd	[%fp-264],%f2
	fmovs	%f23,%f31
	fmovs	%f22,%f30
	faddd	%f30,%f24,%f26
	std	%f26,[%fp-248]
	ldd	[%fp-248],%f28
	fsubd	%f28,%f30,%f0
	ldd	[%fp-248],%f10
	fsubd	%f0,%f2,%f4
	fsubd	%f6,%f4,%f8
	std	%f8,[%fp-240]
	ldd	[%fp-240],%f12
	faddd	%f10,%f12,%f14
	st	%f14,[%i2]
	st	%f15,[%i2+4]
	ld	[%i2],%f16
	ldd	[%fp-248],%f18
	ld	[%i2+4],%f17
	fsubd	%f16,%f18,%f20
	ldd	[%fp-240],%f22
	fsubd	%f22,%f20,%f24
	st	%f24,[%i3]
	bne	L77063
	st	%f25,[%i3+4]
	ld	[%i2],%f26
	fnegs	%f26,%f26
	ld	[%i2+4],%f27
	st	%f26,[%i2]
	st	%f27,[%i2+4]
	ld	[%i3],%f28
	fnegs	%f28,%f28
	ld	[%i3+4],%f29
	st	%f28,[%i3]
	st	%f29,[%i3+4]
	sub	%g0,%i1,%i1
L77063:
	and	%i1,7,%i0
	ret
	restore
LP192	=	96
	.seg	"data"			! [internal]
	.align	4
_zero:
	.word	0
	.align	4
_one:
	.word	1
	.align	4
_p_53:
	.word	0xa2f983
	.word	0x6e4e44
	.word	0x16f3c4
	.word	0xea69b5
	.word	0xd3e131
	.word	0x60e1d2
	.word	0xd7982a
	.word	0xc031f5
	.word	0xd67bcc
	.word	0xfd1375
	.word	0x60919b
	.word	0x3fa0bb
	.word	0x612abb
	.word	0x714f9b
	.word	0x3da8a
	.word	0xc05948
	.word	0xd023f4
	.word	0x5afa37
	.word	0x51631d
	.word	0xcd7a90
	.word	0xc0474a
	.word	0xf6a6f3
	.word	0x1a52e1
	.word	0x5c3927
	.word	0x3ada45
	.word	0x4e2db5
	.word	0x64e8c4
	.word	0x274a5b
	.word	0xb74adc
	.word	0x1e6591
	.word	0x2822be
	.word	0x4771f5
	.word	0x12a63f
	.word	0x83bd35
	.word	0x2488ca
	.word	0x1fe1be
	.word	0x42c21a
	.word	0x682569
	.word	0x2afb91
	.word	0x68ade1
	.word	0x4a42e5
	.word	0x9be357
	.word	0xb79675
	.word	0xce998a
	.word	0x83af8b
	.word	0xe645e6
	.word	0xdf0789
	.word	0x9e9747
	.word	0xaa15ff
	.word	0x358c3f
	.word	0xaf3141
	.word	0x72a3f7
	.word	0x2bf1d4
	.word	0xf3ad96
	.word	0x7d759f
	.word	0x257fce
	.word	0x29fb69
	.word	0xb1b42c
	.word	0xc32de1
	.word	0x8c0bbd
	.word	0x31ec2f
	.word	0x942026
	.word	0x85dce7
	.word	0x653ff3
	.word	0x136fa7
	.word	0xd7a5f
	.align	4
_p_66:
	.word	0xa2f983
	.word	0x6e4e44
	.word	0x152a00
	.word	0x62bc4
	.word	0xda276
	.word	0xbed4c1
	.word	0xfdf905
	.word	0x5cd5ba
	.word	0x767cec
	.word	0x1f80d6
	.word	0xc26053
	.word	0x3a0070
	.word	0x107c2a
	.word	0xf68ee9
	.word	0x687b7a
	.word	0xb990aa
	.word	0x38de4b
	.word	0x96cff3
	.word	0x92735e
	.word	0x8b34f6
	.word	0x195bfc
	.word	0x27f88e
	.word	0xa93ec5
	.word	0x3958a5
	.word	0x3e5d13
	.word	0x1c55a8
	.word	0x5b4a8b
	.word	0xa42e04
	.word	0x12d105
	.word	0x35580d
	.word	0xf62347
	.word	0x450900
	.word	0xb98bca
	.word	0xf7e8a4
	.word	0xa2e5d5
	.word	0x69bc52
	.word	0xf0381d
	.word	0x1a0a88
	.word	0xfe8714
	.word	0x7f6735
	.word	0xbb7d4d
	.word	0xc6f642
	.word	0xb27e80
	.word	0x6191bf
	.word	0xb6b750
	.word	0x52776e
	.word	0xd60fd0
	.word	0x607dcc
	.word	0x68bfaf
	.word	0xed69fc
	.word	0x6eb305
	.word	0xd2557d
	.word	0x25bdfb
	.word	0x3e4aa1
	.word	0x84472d
	.word	0x8b0376
	.word	0xf77740
	.word	0xd290df
	.word	0x15ec8c
	.word	0x45a5c3
	.word	0x6181ef
	.word	0xc5e7e8
	.word	0xd8909c
	.word	0xf62144
	.word	0x298428
	.word	0x6e5d9d
	.align	4
_p_inf:
	.word	0xa2f983
	.word	0x6e4e44
	.word	0x1529fc
	.word	0x2757d1
	.word	0xf534dd
	.word	0xc0db62
	.word	0x95993c
	.word	0x439041
	.word	0xfe5163
	.word	0xabdebb
	.word	0xc561b7
	.word	0x246e3a
	.word	0x424dd2
	.word	0xe00649
	.word	0x2eea09
	.word	0xd1921c
	.word	0xfe1deb
	.word	0x1cb129
	.word	0xa73ee8
	.word	0x8235f5
	.word	0x2ebb44
	.word	0x84e99c
	.word	0x7026b4
	.word	0x5f7e41
	.word	0x3991d6
	.word	0x398353
	.word	0x39f49c
	.word	0x845f8b
	.word	0xbdf928
	.word	0x3b1ff8
	.word	0x97ffde
	.word	0x5980f
	.word	0xef2f11
	.word	0x8b5a0a
	.word	0x6d1f6d
	.word	0x367ecf
	.word	0x27cb09
	.word	0xb74f46
	.word	0x3f669e
	.word	0x5fea2d
	.word	0x7527ba
	.word	0xc7ebe5
	.word	0xf17b3d
	.word	0x739f7
	.word	0x8a5292
	.word	0xea6bfb
	.word	0x5fb11f
	.word	0x8d5d08
	.word	0x560330
	.word	0x46fc7b
	.word	0x6babf0
	.word	0xcfbc20
	.word	0x9af436
	.word	0x1da9e3
	.word	0x91615e
	.word	0xe61b08
	.word	0x659985
	.word	0x5f14a0
	.word	0x68408d
	.word	0xffd880
	.word	0x4d7327
	.word	0x310606
	.word	0x1556ca
	.word	0x73a8c9
	.word	0x60e27b
	.word	0xc08c6b
	.align	8
_two24:
	.word	0x41700000
	.word	0
	.align	8
_twon24:
	.word	0x3e700000
	.word	0
	.align	8
_p1:
	.word	0x3ff921fb
	.word	0x40000000
	.align	8
_p2:
	.word	0x3e74442d
	.word	0
	.align	8
_p3_inf:
	.word	0x3cf84698
	.word	-0x80000000
	.align	8
_p3_66:
	.word	0x3cf84698
	.word	0
	.align	8
_p3_53:
	.word	0x3cf80000
	.word	0
	.align	8
L2000000:
	.word	0x3fc00000
	.word	0
	.align	8
L2000001:
	.word	0x40200000
	.word	0
	.align	8
L2000002:
	.word	0x3fe00000
	.word	0
	.align	8
L2000003:
	.word	0x3ff00000
	.word	0
