        .seg	"data"
        .asciz  "@(#)attb.s 1.1 92/07/30 Copyr 1986 Sun Micro"
        .seg	"text"
				
!       Copyright (c) 1984 by Sun Microsystems, Inc.

#include <sun4/asm_linkage.h>

!	battack(int)
!
! does white/black attack position?

.global	_battack

.global	_dir, _board

uleft	= 04040;
uright	= 04004;
dleft	= 00440;
dright	= 00404;
left	= 00040;
right	= 00004;
up	= 04000;
down	= 00400;
u2r1	= 06004;
u1r2	= 04006;
d1r2	= 00406;
d2r1	= 00604;
d2l1	= 00640;
d1l2	= 00460;
u1l2	= 04060;
u2l1	= 06040;

! r0 i0
! r1 i1
! r2 i2
! r3 i3
! r4 i4
! r5 ~o7 

#define r_board	%l0
#define r_dir	%l1

_battack:
	save	%sp, -SA(MINFRAME), %sp	! mov	2(sp),r0
	set	_board, r_board
	set	_dir, r_dir
	sll	%i0, 1, %i0		! asl	r0
	lduh	[r_dir + %i0], %i1	! mov	_dir(r0),r1
	! moved to delay slot below	! mov	$2,r2
	btst	u2r1, %i1		! bit	$u2r1,r1
	bne	1f			! bne	1f
	mov	2, %i2

	add	r_board, (-15*2), %g1
	ldsh	[%g1 + %i0], %g1
	cmp	%g1, %i2		! cmp	_board+[-15.*2](r0),r2
	be	2f			! beq	2f
	.empty
1:
	btst	u1r2, %i1		! bit	$u1r2,r1
	bne	1f			! bne	1f
	.empty

	add	r_board, (-6*2), %g1
	ldsh	[%g1 + %i0], %g1
	cmp	%g1, %i2		! cmp	_board+[-6.*2](r0),r2
	be	2f			! beq	2f
	.empty
1:
	btst	d1r2, %i1		! bit	$d1r2,r1
	bne	1f			! bne	1f
	.empty

	add	r_board, (10*2), %g1
	ldsh	[%g1 + %i0], %g1
	cmp	%g1, %i2		! cmp	_board+[+10.*2](r0),r2
	be	2f			! beq	2f
	.empty
1:
	btst	d2r1, %i1		! bit	$d2r1,r1
	bne	1f			! bne	1f
	.empty

	add	r_board, (17*2), %g1
	ldsh	[%g1 + %i0], %g1
	cmp	%g1, %i2		! cmp	_board+[+17.*2](r0),r2
	be	2f			! beq	2f
	.empty
1:
	btst	d2l1, %i1		! bit	$d2l1,r1
	bne	1f			! bne	1f
	.empty

	add	r_board, (15*2), %g1
	ldsh	[%g1 + %i0], %g1
	cmp	%g1, %i2		! cmp	_board+[+15.*2](r0),r2
	be	2f			! beq	2f
	.empty
1:
	btst	d1l2, %i1		! bit	$d1l2,r1
	bne	1f			! bne	1f
	.empty

	add	r_board, (6*2), %g1
	ldsh	[%g1 + %i0], %g1
	cmp	%g1, %i2		! cmp	_board+[+6.*2](r0),r2
	be	2f			! beq	2f
	.empty
1:
	btst	u1l2, %i1		! bit	$u1l2,r1
	bne	1f			! bne	1f
	.empty

	add	r_board, (-10*2), %g1
	ldsh	[%g1 + %i0], %g1
	cmp	%g1, %i2		! cmp	_board+[-10.*2](r0),r2
	be	2f			! beq	2f
	.empty
1:
	btst	u2l1, %i1		! bit	$u2l1,r1
	bne	1f			! bne	1f
	.empty

	add	r_board, (-17*2), %g1
	ldsh	[%g1 + %i0], %g1
	cmp	%g1, %i2		! cmp	_board+[-17.*2](r0),r2
	be	2f			! beq	2f
	nop
1:
	call	badiag
	nop
	.half	uleft, -9*2		! jsr	r5,badiag; uleft; -9.*2
	call	badiag
	nop
	.half	uright, -7*2		! jsr	r5,badiag; uright; -7.*2
	call	badiag
	nop
	.half	dleft, 7*2		! jsr	r5,badiag; dleft; 7.*2
	call	badiag
	nop
	.half	dright, 9*2		! jsr	r5,badiag; dright; 9.*2
	call	barank
	nop
	.half	up, -8*2		! jsr	r5,barank; up; -8.*2
	call	barank
	nop
	.half	left, -1*2		! jsr	r5,barank; left; -1.*2
	call	barank
	nop
	.half	right, 1*2		! jsr	r5,barank; right; 1.*2
	call	barank
	nop
	.half	down, 8*2		! jsr	r5,barank; down; 8.*2

	lduh	[r_dir + %i0], %g2
	btst	uleft, %g2		! bit	$uleft,_dir(r0)
	bne	1f			! bne	1f
	.empty

	add	r_board, -18, %g1
	ldsh	[%g1 + %i0], %g1
	cmp	%g1, 1			! cmp	_board-18.(r0),$1  / pawn?
	be	2f			! beq	2f
	nop
1:
	lduh	[r_dir + %i0], %g2
	btst	uright, %g2		! bit	$uright,_dir(r0)
	bne	1f			! bne	1f
	.empty

	add	r_board, -14, %g1
	ldsh	[%g1 + %i0], %g1
	cmp	%g1, 1			! cmp	_board-14.(r0),$1
	bne	1f			! bne	1f
	nop
2:
	ret				! clr	r0
	restore	%g0, %g0, %o0		! rts	pc
1:
	mov	1, %i0			! mov	$1,r0
	ret
	restore				! rts	pc

badiag:
	mov	%i0, %i1		! mov	r0,r1
	ldsh	[%o7 + 8], %i2		! mov	(r5)+,r2
	ldsh	[%o7 + 10], %i3		! mov	(r5)+,r3
	lduh	[r_dir + %i1], %g1
	btst	%i2, %g1		! bit	r2,_dir(r1)
	bne	1f			! bne	1f
	nop

	add	%i1, %i3, %i1		! add	r3,r1
	ldsh	[r_board + %i1], %i4	! mov	_board(r1),r4
	tst	%i4
	be	2f			! beq	2f
	cmp	%i4, 3			! cmp	r4,$3
	be	9f			! beq	9f
	cmp	%i4, 5			! cmp	r4,$5
	be	9f			! beq	9f
	cmp	%i4, 6			! cmp	r4,$6
	be	9f			! beq	9f
	nop
1:
	jmp	%o7 + 12		! rts	r5
	nop
2:
	lduh	[r_dir + %i1], %g1
	btst	%i2, %g1		! bit	r2,_dir(r1)
	bne	2f			! bne	2f
	nop

	add	%i1, %i3, %i1		! add	r3,r1
	ldsh	[r_board + %i1], %i4	! mov	_board(r1),r4
	tst	%i4
	be	2b			! beq	2b
	cmp	%i4, 3			! cmp	r4,$3
	be	9f			! beq	9f
	cmp	%i4, 5			! cmp	r4,$5
	be	9f			! beq	9f
	nop
2:
	jmp	%o7 + 12		! rts	r5
	nop

barank:
	mov	%i0, %i1		! mov	r0,r1
	ldsh	[%o7 + 8], %i2		! mov	(r5)+,r2
	ldsh	[%o7 + 10], %i3		! mov	(r5)+,r3
	lduh	[r_dir + %i1], %g1
	btst	%i2, %g1		! bit	r2,_dir(r1)
	bne	1f			! bne	1f
	nop

	add	%i1, %i3, %i1		! add	r3,r1
	ldsh	[r_board + %i1], %i4	! mov	_board(r1),r4
	tst	%i4
	be	2f			! beq	2f
	cmp	%i4, 4			! cmp	r4,$4
	be	9f			! beq	9f
	cmp	%i4, 5			! cmp	r4,$5
	be	9f			! beq	9f
	cmp	%i4, 6			! cmp	r4,$6
	be	9f			! beq	9f
	nop
1:
	jmp	%o7 + 12		! rts	r5
	nop
2:
	lduh	[r_dir + %i1], %g1
	btst	%i2, %g1		! bit	r2,_dir(r1)
	bne	2f			! bne	2f
	nop

	add	%i1, %i3, %i1		! add	r3,r1
	ldsh	[r_board + %i1], %i4	! mov	_board(r1),r4
	tst	%i4
	be	2b			! beq	2b
	cmp	%i4, 4			! cmp	r4,$4
	be	9f			! beq	9f
	cmp	%i4, 5			! cmp	r4,$5
	be	9f			! beq	9f
	nop
2:
	jmp	%o7 + 12		! rts	r5
	nop
9:
					! mov	(sp)+,r5
	ret				! clr	r0
	restore	%g0, %g0, %o0		! rts	pc
