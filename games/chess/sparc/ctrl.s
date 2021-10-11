        .seg	"data"
        .asciz  "@(#)ctrl.s 1.1 92/07/30 Copyr 1987 Sun Micro"
        .seg	"text"

!       Copyright (c) 1987 by Sun Microsystems, Inc.

#include <sun4/asm_linkage.h>

!	attack(int)
!
! list pieces controlling a square

.global	_attack

.global	_dir, _board
.global	_attacv

none	= 12345
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

! r0 -> i0
! r1 -> i1
! r4 -> i4
! r5 -> i5

#define r_dir	%l0
#define r_board	%l1

_attack:
	save	%sp, -SA(MINFRAME), %sp	! mov	2(sp),r0
	set	_dir, r_dir
	set	_board, r_board
	sll	%i0, 1, %i0		! asl	r0
	set	_attacv, %i4		! mov	$_attacv,r4

	call	patt			! jsr	r5,patt
	nop
	.half	u2r1, -15*2, 2, -2
	call	patt			! jsr	r5,patt
	nop
	.half	u1r2, -6*2, 2, -2
	call	patt			! jsr	r5,patt
	nop
	.half	d2r1, 17*2, 2, -2
	call	patt			! jsr	r5,patt
	nop
	.half	d2l1, 15*2, 2, -2
	call	patt			! jsr	r5,patt
	nop
	.half	d1l2, 6*2, 2, -2
	call	patt			! jsr	r5,patt
	nop
	.half	u1l2, -10*2, 2, -2
	call	patt			! jsr	r5,patt
	nop
	.half	u2l1, -17*2, 2, -2

	call	satt			! jsr	r5,satt
	nop
	.half	uleft, -9*2, 1, 3, -3, 5, -5
	.half	0			!	align instruction
	call	satt			! jsr	r5,satt
	nop
	.half	uright, -7*2, 1, 3, -3, 5, -5
	.half	0			!	align instruction
	call	satt			! jsr	r5,satt
	nop
	.half	dleft, 7*2, -1, 3, -3, 5, -5
	.half	0			!	align instruction
	call	satt			! jsr	r5,satt
	nop
	.half	dright, 9*2, -1, 3, -3, 5, -5
	.half	0			!	align instruction
	call	satt			! jsr	r5,satt
	nop
	.half	up, -8*2, none, 4, -4, 5, -5
	.half	0			!	align instruction
	call	satt			! jsr	r5,satt
	nop
	.half	left, -1*2, none, 4, -4, 5, -5
	.half	0			!	align instruction
	call	satt			! jsr	r5,satt
	nop
	.half	right, 1*2, none, 4, -4, 5, -5
	.half	0			!	align instruction
	call	satt			! jsr	r5,satt
	nop
	.half	down, 8*2, none, 4, -4, 5, -5
	.half	0			!	align instruction
	clrh	[%i4]			! clr	(r4)+
	ret
	restore				! rts	pc

patt:
	add	%o7, 8, %i5		!	fake r5
	lduh	[%i5], %g1
	lduh	[r_dir + %i0], %g2
	btst	%g1, %g2		! bit	(r5)+,_dir(r0)
	bne	1f			! bne	1f
	add	%i5, 2, %i5

	ldsh	[%i5], %i1		! mov	r0,r1
	add	%i1, %i0, %i1		! add	(r5)+,r1
	call	look			! jsr	pc,look
	add	%i5, 2, %i5
	call	look			! jsr	pc,look
	nop
	jmp	%i5			! rts	r5
	nop
1:
					! add	$6,r5
	jmp	%i5 + 6			! rts	r5
	nop

satt:
	add	%o7, 8, %i5		!	fake r5
	mov	%i5, %l5		! mov	r5,-(sp)
	lduh	[%i5], %g1
	lduh	[r_dir + %i0], %g2
	btst	%g1, %g2		! bit	(r5)+,_dir(r0)
	bne	1f			! bne	1f
	add	%i5, 2, %i5

	ldsh	[%i5], %i1		! mov	r0,r1
	add	%i1, %i0, %i1		! add	(r5)+,r1
	call	look			! jsr	pc,look		/ pawn
	add	%i5, 2, %i5
	mov	%i0, %i1		! mov	r0,r1
2:
	mov	%l5, %i5		! mov	(sp),r5
	lduh	[%i5], %g1
	lduh	[r_dir + %i1], %g2
	btst	%g1, %g2		! bit	(r5)+,_dir(r1)
	bne	1f			! bne	1f
	add	%i5, 2, %i5

	ldsh	[%i5], %g1
	add	%i1, %g1, %i1		! add	(r5)+,r1
	ldsh	[r_board + %i1], %g1
	tst	%g1			! tst	_board(r1)
	be	2b			! beq	2b
	add	%i5, 2, %i5

	add	%i5, 2, %i5		! tst	(r5)+
	mov	%i4, %l4		! mov	r4,-(sp)
	call	look			! jsr	pc, look
	nop
	call	look			! jsr	pc, look
	nop
	call	look			! jsr	pc, look
	nop
	call	look			! jsr	pc, look
	nop
	cmp	%l4, %i4		! cmp	(sp)+,r4
	bne	2b			! bne	2b
	nop
1:
					! mov	(sp)+,r5
					! add	$14.,r5
	jmp	%l5 + 16		! rts	r5
	nop

look:
	ldsh	[%i5], %g1
	ldsh	[r_board + %i1], %g2
	cmp	%g1, %g2		! cmp	(r5)+,_board(r1)
	bne	1f			! bne	1f
	add	%i5, 2, %i5

	ldsh	[%i5 - 2], %g1
	sth	%g1, [%i4]
	add	%i4, 2, %i4		! mov	-2(r5),(r4)+
1:
	retl				! rts	pc
	nop
