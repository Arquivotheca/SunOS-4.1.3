	.seg	"data"
	.asciz	"@(#)bgen.s 1.1 92/07/30 Copyr 1987 Sun Micro"
	.seg	"text"

!	Copyright (c) 1987 by Sun Microsystems, Inc.

#include <sun4/asm_linkage.h>

!	bgen()
!
! generate moves

.global	_bgen

.global	_pval, _board, _dir
.global	_flag, _lmp, _bkpos
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

_bgen:
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
	ble	1f			! ble	1f
	.empty

	set	2f-4, %g1
	sll	%i0, 2, %i0		! asl	r0
	ld	[%g1 + %i0], %g1
	jmp	%g1			! jmp	*2f-2(r0)
	nop
2:
	.word	pawn			! .word	pawn-2b
	.word	knight			! .word	knight-2b
	.word	bishop			! .word	bishop-2b
	.word	rook			! .word	rook-2b
	.word	queen			! .word	queen-2b
	.word	king			! .word	king-2b

pawn:
	lduh	[%i4], %g1
	btst	dleft, %g1		! bit	$dleft,(r4)
	bne	2f			! bne	2f
	nop

	ldsh	[%i3 + (2*7)], %g1
	tst	%g1			! tst	2*7.(r3)
	bge	3f			! bge	3f
	nop

	call	btry
	nop
	.half	0,7*2			! jsr	r5,btry; 0; 7.*2
3:
	ldsh	[r_eppos], %g1		! mov	r1,r0
	add	%i1, 7, %i0		! add	$7,r0
	cmp	%i0, %g1		! cmp	r0,_eppos
	bne	2f			! bne	2f
	nop

	call	btry
	nop
	.half	0,-1*2			! jsr	r5,btry; 0; -1*2
2:
	lduh	[%i4], %g1
	btst	dright, %g1		! bit	$dright,(r4)
	bne	2f			! bne	2f
	nop

	ldsh	[%i3 + (9*2)], %g1
	tst	%g1			! tst	9.*2(r3)
	bge	3f			! bge	3f
	nop

	call	btry
	nop
	.half	0, 9*2			! jsr	r5,btry; 0; 9.*2
3:
	ldsh	[r_eppos], %g1		! mov	r1,r0
	add	%i1, 9, %i0		! add	$9,r0
	cmp	%i0, %g1		! cmp	r0,_eppos
	bne	2f			! bne	2f
	nop

	call	btry
	nop
	.half	0,2*1			! jsr	r5,btry; 0; 2*1
2:
	ldsh	[%i3 + (2*8)], %g1
	tst	%g1			! tst	2*8.(r3)
	bne	1f			! bne	1f
	nop

	call	btry
	nop
	.half	0,2*8			! jsr	r5,btry; 0; 2*8.

	lduh	[%i4], %g1
	btst	rank7, %g1		! bit	$rank7,(r4)
	be	1f			! beq	1f
	nop

	ldsh	[%i3 + (2*16)], %g1
	tst	%g1			! tst	2*16.(r3)
	bne	1f			! bne	1f
	nop

	call	btry
	nop
	.half	0,16*2			! jsr	r5,btry; 0; 16.*2

	ba,a	1f			! br	1f

knight:
	call	btry
	nop
	.half	u2r1, -15*2		! jsr	r5,btry; u2r1; -15.*2
	call	btry
	nop
	.half	u1r2, -6*2		! jsr	r5,btry; u1r2; -6.*2
	call	btry
	nop
	.half	d1r2, 10*2		! jsr	r5,btry; d1r2; 10.*2
	call	btry
	nop
	.half	d2r1, 17*2		! jsr	r5,btry; d2r1; 17.*2
	call	btry
	nop
	.half	d2l1, 15*2		! jsr	r5,btry; d2l1; 15.*2
	call	btry
	nop
	.half	d1l2, 6*2		! jsr	r5,btry; d1l2; 6.*2
	call	btry
	nop
	.half	u1l2, -10*2		! jsr	r5,btry; u1l2; -10.*2
	call	btry
	nop
	.half	u2l1, -17*2		! jsr	r5,btry; u2l1; -17.*2
1:					! bra	1f
	sub	%i4, 2, %i4		! cmp	-(r4),-(r3)
	deccc	%i1			! dec	r1
	bpos	0b			! bpl	0b
	sub	%i3, 2, %i3

	sethi	%hi(_lmp), %g1
	st	%i2, [%g1 + %lo(_lmp)]	! mov	r2,_lmp
	ret
	restore				! rts	pc

bishop:
	call	bslide
	nop
	.half	uleft, -9*2		! jsr	r5,bslide; uleft; -9.*2
	call	bslide
	nop
	.half	uright, -7*2		! jsr	r5,bslide; uright; -7.*2
	call	bslide
	nop
	.half	dleft, 7*2		! jsr	r5,bslide; dleft; 7.*2
	call	bslide
	nop
	.half	dright, 9*2		! jsr	r5,bslide; dright; 9.*2
	ba,a	1b			! br	1b

rook:
	call	bslide
	nop
	.half	up, -8*2		! jsr	r5,bslide; up; -8.*2
	call	bslide
	nop
	.half	down, 8*2		! jsr	r5,bslide; down; 8.*2
	call	bslide
	nop
	.half	left, -1*2		! jsr	r5,bslide; left; -1.*2.
	call	bslide
	nop
	.half	right, 1*2		! jsr	r5,bslide; right; 1.*2
	ba,a	1b			! br	1b
queen:
	call	bslide
	nop
	.half	uleft, -9*2		! jsr	r5,bslide; uleft; -9.*2
	call	bslide
	nop
	.half	uright, -7*2		! jsr	r5,bslide; uright; -7.*2
	call	bslide
	nop
	.half	dleft, 7*2		! jsr	r5,bslide; dleft; 7.*2
	call	bslide
	nop
	.half	dright, 9*2		! jsr	r5,bslide; dright; 9.*2
	call	bslide
	nop
	.half	up, -8*2		! jsr	r5,bslide; up; -8.*2
	call	bslide
	nop
	.half	left, -1*2		! jsr	r5,bslide; left; -1.*2
	call	bslide
	nop
	.half	right, 1*2		! jsr	r5,bslide; right; 1.*2
	call	bslide
	nop
	.half	down, 8*2		! jsr	r5,bslide; down; 8.*2
	ba,a	1b			! br	1b

king:
	call	btry
	nop
	.half	uleft, -9*2		! jsr	r5,btry; uleft; -9.*2
	call	btry
	nop
	.half	uright, -7*2		! jsr	r5,btry; uright; -7.*2
	call	btry
	nop
	.half	dleft, 7*2		! jsr	r5,btry; dleft; 7.*2
	call	btry
	nop
	.half	dright, 9*2		! jsr	r5,btry; dright; 9.*2
	call	btry
	nop
	.half	up, -8*2		! jsr	r5,btry; up; -8.*2
	call	btry
	nop
	.half	left, -1*2		! jsr	r5,btry; left; -1.*2
	call	btry
	nop
	.half	right, 1*2		! jsr	r5,btry; right; 1.*2
	call	btry
	nop
	.half	down, 8*2		! jsr	r5,btry; down; 8.*2
	ba,a	1b			! br	1b

btry:
	lduh	[%o7 + 8], %g1
	lduh	[%i4], %g2
	btst	%g1, %g2		! bit	(r5)+,(r4)
	bne	1f			! bne	1f
	nop

	ldsh	[%o7 + 10], %l0		! mov	r3,r0
					! add	(r5),r0
	ldsh	[%l0 + %i3], %i0	! mov	(r0),r0
	tst	%i0
	bg	1f			! bgt	1f
	nop

	sll	%i0, 1, %i0		! asl	r0
	ldsh	[r_pval12 + %i0], %g1	! mov	_pval+12.(r0),(r2)
	ldsh	[r_value], %g2
	sub	%g1, %g2, %g1
	sth	%g1, [%i2]		! sub	_value,(r2)+
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

bslide:
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
	bg	1f			! bgt	1f
	nop
	bl	2f			! blt	2f
	nop

	ldsh	[r_value], %g1
	neg	%g1			! clr	(r2)
	sth	%g1, [%i2]		! sub	_value,(r2)+
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
	ldsh	[r_pval12 + %i0], %g1	! mov	_pval+12.(r0),(r2)
	ldsh	[r_value], %g2
	sub	%g1, %g2, %g1
	sth	%g1, [%i2]		! sub	_value,(r2)+
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
