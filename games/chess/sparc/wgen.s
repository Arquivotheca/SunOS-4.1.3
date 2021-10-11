	.seg	"data"
	.asciz	"@(#)wgen.s 1.1 92/07/30 Copyr 1987 Sun Micro"
	.seg	"text"

!	Copyright (c) 1987 by Sun Microsystems, Inc.

#include <sun4/asm_linkage.h>

!	wgen()
!
! generate moves

.global	_wgen

.global	_pval, _board, _dir
.global	_flag, _lmp, _wkpos
.global	_eppos
.global	_value

uleft	= 04040
uright	= 04004
dleft	= 00440
dright	= 00404
left	= 00040
right	= 00004
up	= 04000
down	= 00400
u2r1	= 06004
u1r2	= 04006
d1r2	= 00406
d2r1	= 00604
d2l1	= 00640
d1l2	= 00460
u1l2	= 04060
u2l1	= 06040
rank2	= 00200
rank7	= 02000

! r0 -> i0 (this is used as a temporary)
! r1 -> i1
! r2 -> i2
! r3 -> i3
! r4 -> i4
! r5 -> ~o7

#define r_board		%l1
#define r_value		%l2
#define r_pval12	%l5
#define r_eppos		%l6

_wgen:
	save	%sp, -SA(MINFRAME), %sp
	set	_dir+126, %i4		! mov	$_dir+126.,r4
	set	_board+126, %i3		! mov	$_board+126.,r3
	sethi	%hi(_lmp), %g1
	ld	[%g1 + %lo(_lmp)], %i2	! mov	_lmp,r2
	mov	63, %i1			! mov	63.,r1
	set	_board, r_board
	set	_value, r_value
	set	_pval+12, r_pval12
	set	_eppos, r_eppos
0:
	ldsh	[%i3], %i0		! mov	(r3),r0
	tst	%i0
	bge	1f			! bge	1f
	.empty

	set	2f, %g1
	sll	%i0, 2, %i0		! asl	r0
	ld	[%g1 + %i0], %g1
	jmp	%g1			! jmp	*2f(r0)
	nop
					! start:
	.word	wking			! .word	wking-start
	.word	wqueen			! .word	wqueen-start
	.word	wrook			! .word	wrook-start
	.word	wbishop			! .word	wbishop-start
	.word	wknight			! .word	wknight-start
	.word	wpawn			! .word	wpawn-start
2:

wpawn:
	lduh	[%i4], %g1
	btst	uleft, %g1		! bit	$uleft,(r4)
	bne	2f			! bne	2f
	nop

	ldsh	[%i3 + (-2*9)], %g1
	tst	%g1			! tst	-2*9.(r3)
	ble	3f			! ble	3f
	nop

	call	wwtry
	nop
	.half	0,-9*2			! jsr	r5,wwtry; 0; -9.*2
3:
	ldsh	[r_eppos], %g1		! mov	r1,r0
	sub	%i1, 9, %i0		! sub	$9,r0
	cmp	%i0, %g1		! cmp	r0,_eppos
	bne	2f			! bne	2f
	nop

	call	wwtry
	nop
	.half	0,-1*2			! jsr	r5,wwtry; 0; -1*2
2:
	lduh	[%i4], %g1
	btst	uright, %g1		! bit	$uright,(r4)
	bne	2f			! bne	2f
	nop

	ldsh	[%i3 + (-7*2)], %g1
	tst	%g1			! tst	-7.*2(r3)
	ble	3f			! ble	3f
	nop

	call	wwtry
	nop
	.half	0, -7*2			! jsr	r5,wwtry; 0; -7.*2
3:
	ldsh	[r_eppos], %g1		! mov	r1,r0
	sub	%i1, 7, %i0		! sub	$7,r0
	cmp	%i0, %g1		! cmp	r0,_eppos
	bne	2f			! bne	2f
	nop

	call	wwtry
	nop
	.half	0,2*1			! jsr	r5,wwtry; 0; 2*1
2:
	ldsh	[%i3 + (-2*8)], %g1
	tst	%g1			! tst	-2*8.(r3)
	bne	1f			! bne	1f
	nop

	call	wwtry
	nop
	.half	0,-2*8			! jsr	r5,wwtry; 0; -2*8.

	lduh	[%i4], %g1
	btst	rank2, %g1		! bit	$rank2,(r4)
	be	1f			! beq	1f
	nop

	ldsh	[%i3 + (-2*16)], %g1
	tst	%g1			! tst	-2*16.(r3)
	bne	1f			! bne	1f
	nop

	call	wwtry
	nop
	.half	0,-16*2			! jsr	r5,wwtry; 0; -16.*2

	ba,a	1f			! br	1f

wknight:
	call	wwtry
	nop
	.half	u2r1, -15*2		! jsr	r5,wwtry; u2r1; -15.*2
	call	wwtry
	nop
	.half	u1r2, -6*2		! jsr	r5,wwtry; u1r2; -6.*2
	call	wwtry
	nop
	.half	d1r2, 10*2		! jsr	r5,wwtry; d1r2; 10.*2
	call	wwtry
	nop
	.half	d2r1, 17*2		! jsr	r5,wwtry; d2r1; 17.*2
	call	wwtry
	nop
	.half	d2l1, 15*2		! jsr	r5,wwtry; d2l1; 15.*2
	call	wwtry
	nop
	.half	d1l2, 6*2		! jsr	r5,wwtry; d1l2; 6.*2
	call	wwtry
	nop
	.half	u1l2, -10*2		! jsr	r5,wwtry; u1l2; -10.*2
	call	wwtry
	nop
	.half	u2l1, -17*2		! jsr	r5,wwtry; u2l1; -17.*2
1:					! bra	1f
	sub	%i4, 2, %i4		! cmp	-(r4),-(r3)
	deccc	%i1			! dec	r1
	bpos	0b			! bpl	0b
	sub	%i3, 2, %i3

	sethi	%hi(_lmp), %g1
	st	%i2, [%g1 + %lo(_lmp)]	! mov	r2,_lmp
	ret
	restore				! rts	pc

wbishop:
	call	wslide
	nop
	.half	uleft, -9*2		! jsr	r5,wslide; uleft; -9.*2
	call	wslide
	nop
	.half	uright, -7*2		! jsr	r5,wslide; uright; -7.*2
	call	wslide
	nop
	.half	dleft, 7*2		! jsr	r5,wslide; dleft; 7.*2
	call	wslide
	nop
	.half	dright, 9*2		! jsr	r5,wslide; dright; 9.*2
	ba,a	1b			! br	1b

wrook:
	call	wslide
	nop
	.half	up, -8*2		! jsr	r5,wslide; up; -8.*2
	call	wslide
	nop
	.half	down, 8*2		! jsr	r5,wslide; down; 8.*2
	call	wslide
	nop
	.half	left, -1*2		! jsr	r5,wslide; left; -1.*2.
	call	wslide
	nop
	.half	right, 1*2		! jsr	r5,wslide; right; 1.*2
	ba,a	1b			! br	1b
wqueen:
	call	wslide
	nop
	.half	uleft, -9*2		! jsr	r5,wslide; uleft; -9.*2
	call	wslide
	nop
	.half	uright, -7*2		! jsr	r5,wslide; uright; -7.*2
	call	wslide
	nop
	.half	dleft, 7*2		! jsr	r5,wslide; dleft; 7.*2
	call	wslide
	nop
	.half	dright, 9*2		! jsr	r5,wslide; dright; 9.*2
	call	wslide
	nop
	.half	up, -8*2		! jsr	r5,wslide; up; -8.*2
	call	wslide
	nop
	.half	left, -1*2		! jsr	r5,wslide; left; -1.*2
	call	wslide
	nop
	.half	right, 1*2		! jsr	r5,wslide; right; 1.*2
	call	wslide
	nop
	.half	down, 8*2		! jsr	r5,wslide; down; 8.*2
	ba,a	1b			! br	1b

wking:
	call	wwtry
	nop
	.half	uleft, -9*2		! jsr	r5,wwtry; uleft; -9.*2
	call	wwtry
	nop
	.half	uright, -7*2		! jsr	r5,wwtry; uright; -7.*2
	call	wwtry
	nop
	.half	dleft, 7*2		! jsr	r5,wwtry; dleft; 7.*2
	call	wwtry
	nop
	.half	dright, 9*2		! jsr	r5,wwtry; dright; 9.*2
	call	wwtry
	nop
	.half	up, -8*2		! jsr	r5,wwtry; up; -8.*2
	call	wwtry
	nop
	.half	left, -1*2		! jsr	r5,wwtry; left; -1.*2
	call	wwtry
	nop
	.half	right, 1*2		! jsr	r5,wwtry; right; 1.*2
	call	wwtry
	nop
	.half	down, 8*2		! jsr	r5,wwtry; down; 8.*2
	ba,a	1b			! br	1b

wwtry:
	lduh	[%o7 + 8], %g1
	lduh	[%i4], %g2
	btst	%g1, %g2		! bit	(r5)+,(r4)
	bne	1f			! bne	1f
	nop

	ldsh	[%o7 + 10], %l0		! mov	r3,r0
					! add	(r5),r0
	ldsh	[%l0 + %i3], %i0	! mov	(r0),r0
	tst	%i0
	bl	1f			! blt	1f
	nop

	sll	%i0, 1, %i0		! asl	r0
	ldsh	[r_value], %g1		! mov	_value,(r2)
	ldsh	[r_pval12 + %i0], %g2
	sub	%g1, %g2, %g1
	sth	%g1, [%i2]		! sub	_pval+12.(r0),(r2)+
					! mov	(r5)+,r0
	srl	%l0, 1, %i0		! asr	r0
	add	%i0, %i1, %i0		! add	r1, r0
					!	switch byte order
	stb	%i1, [%i2 + 2]		! movb	r0,(r2)+
	stb	%i0, [%i2 + 3]		! movb	r1,(r2)+
	jmp	%o7 + 12		! rts	r5
	add	%i2, 4, %i2
1:
	jmp	%o7 + 12		! tst	(r5)+
	nop				! rts	r5

wslide:
	mov	%i4, %l4		! mov	r4,-(sp)
	mov	%i3, %l3		! mov	r3,-(sp)
1:
	lduh	[%o7 + 8], %g1
	lduh	[%i4], %g2
	btst	%g1, %g2		! bit	(r5)+,(r4)
	bne	1f			! bne	1f
	nop

	ldsh	[%o7 + 10], %g1
	add	%i3, %g1, %i3		! add	(r5),r3
	add	%i4, %g1, %i4		! add	(r5),r4
	ldsh	[%i3], %i0		! mov	(r3),r0
	tst	%i0
	bl	1f			! blt	1f
	nop
	bg	2f			! bgt	2f
	nop

	ldsh	[r_value], %g1
	sth	%g1, [%i2]		! mov	_value,(r2)+
					! mov	r3,r0
	sub	%i3, r_board, %i0	! sub	$_board,r0
	sra	%i0, 1, %i0		! asr	r0
					!	swap bytes
	stb	%i1, [%i2 + 2]		! movb	r0,(r2)+
	stb	%i0, [%i2 + 3]		! movb	r1,(r2)+
	b	1b			! tst	-(r5)
	add	%i2, 4, %i2		! br	1b
2:
	sll	%i0, 1, %i0		! asl	r0
	ldsh	[r_value], %g1		! mov	_value,(r2)
	ldsh	[r_pval12 + %i0], %g2
	sub	%g1, %g2, %g1
	sth	%g1, [%i2]		! sub	_pval+12.(r0),(r2)+
					! mov	r3,r0
	sub	%i3, r_board, %i0	! sub	$_board,r0
	sra	%i0, 1, %i0		! asr	r0
					!	swap bytes
	stb	%i1, [%i2 + 2]		! movb	r0,(r2)+
	stb	%i0, [%i2 + 3]		! movb	r1,(r2)+
	add	%i2, 4, %i2
1:
					! tst	(r5)+
	mov	%l3, %i3		! mov	(sp)+,r3
	mov	%l4, %i4		! mov	(sp)+,r4
	jmp	%o7 + 12		! rts	r5
	nop
