	.seg	"data"
	.asciz	"@(#)wmove.s 1.1 92/07/30 Copyr 1987 Sun Micro"
	.seg	"text"

!	Copyright (c) 1987 by Sun Microsystems, Inc.

#include <sun4/asm_linkage.h>

!	wmove(int), wremove()
!
.global	_wmove, _wremove
.global	_board, _pval, _amp, _flag, _eppos, _value, _wkpos
.global	_game

! r0 -> o0
! r1 -> i1
! r2 -> i2
! r3 -> i3
! r4 -> i4

#define r_board	%l0
#define r_amp	%l1
#define r_flag	%l2
#define	r_eppos	%l3
#define r_value	%l4

_wmove:
	save	%sp, -SA(MINFRAME), %sp
	set	_board, r_board
	set	_amp, r_amp
	set	_flag, r_flag
	set	_eppos, r_eppos
	set	_value, r_value
	ld	[r_amp], %i4		! mov	_amp,r4
	and	%i0, 0xff, %i3		! movb	2(sp),r3	/ to
	srl	%i0, 8, %i2
	and	%i2, 0xff, %i2		! movb	3(sp),r2	/ from
	ldsh	[r_value], %g1
	sth	%g1, [%i4]		! mov	_value,(r4)+
	lduh	[r_flag], %g1
	sth	%g1, [%i4 + 2]		! mov	_flag,(r4)+
	ldsh	[r_eppos], %g1
	sth	%g1, [%i4 + 4]		! mov	_eppos,(r4)+
	sth	%i2, [%i4 + 6]		! mov	r2,(r4)+
	sth	%i3, [%i4 + 8]		! mov	r3,(r4)+
	sll	%i2, 1, %i2		! asl	r2	/ from as a word index
	sll	%i3, 1, %i3		! asl	r3	/ to as word index
	ldsh	[r_board + %i3], %o0	! mov	_board(r3),r0
	sth	%o0, [%i4 + 10]		! mov	r0,(r4)+
	tst	%o0
	be	1f			! beq	1f
	add	%i4, 12, %i4

	sll	%o0, 1, %o0		! asl	r0
	ldsh	[r_value], %g1
	set	_pval+12, %g2
	ldsh	[%g2 + %o0], %g2
	sub	%g1, %g2, %g1
	sth	%g1, [r_value]		! sub	_pval+12.(r0),_value
1:
	ldsh	[r_board + %i2], %o0	! mov	_board(r2),r0
	sth	%o0, [r_board + %i3]	! mov	r0,_board(r3)
	clrh	[r_board + %i2]		! clr	_board(r2)
	mov	-1, %g1
	sth	%g1, [r_eppos]		! mov	$-1,_eppos
	sll	%o0, 2, %o0		! asl	r0
	tst	%o0
	bge	error			! bge	error
	.empty

	set	0f, %g1
	ld	[%g1 + %o0], %g1
	jmp	%g1			! jmp	*0f(r0)	/ type of man
	nop
					! start:
	.word	kmove			! .word	kmove-start
	.word	qmove			! .word	qmove-start
	.word	rmove			! .word	rmove-start
	.word	bmove			! .word	bmove-start
	.word	nmove			! .word	nmove-start
	.word	pmove			! .word	pmove-start
0:

error:
	call	_abort			! 3
	nop

pmove:
	subcc	%i2, %i3, %i2		! sub	r3,r2
	bl,a	1f			! bge	1f
	neg	%i2			! neg	r2
1:
	cmp	%i2, 2*1		! cmp	r2,$2*1		/ ep capture
	bne	1f			! bne	1f
	nop

	clrh	[r_board + %i3]		! clr	_board(r3)
	add	r_board, -16, %g1
	mov	-1, %g2
	sth	%g2, [%g1 + %i3]	! mov	$-1,_board-16.(r3)
	mov	4, %g1
	sth	%g1, [%i4]		! mov	$4,(r4)+
	add	%i4, 2, %i4
	st	%i4, [r_amp]		! mov	r4,_amp
	ret
	restore				! rts	pc
1:
	cmp	%i2, 2*16		! cmp	r2,$2*16.	/ double move
	bne	1f			! bne	1f
	nop

	srl	%i0, 8, %i2
	and	%i2, 0xff, %i2		! movb	3(sp),r2
	sub	%i2, 8, %i2		! sub	$8,r2
	b	move
	sth	%i2, [r_eppos]		! mov	r2,_eppos
					! br	move
1:
	cmp	%i3, 24*2		! cmp	r3,$24.*2
	bge	move			! bge	move
	nop

	ldsh	[r_value], %g1
	sub	%g1, 25, %g1
	sth	%g1, [r_value]		! sub	$25.,_value
	cmp	%i3, 16*2		! cmp	r3,$16.*2
	bge	move			! bge	move
	nop

	ldsh	[r_value], %g1
	sub	%g1, 50, %g1
	sth	%g1, [r_value]		! sub	$50.,_value
	cmp	%i3, 8*2		! cmp	r3,$8.*2    / queen promotion
	bge	move			! bge	move
	nop

	ldsh	[r_value], %g1
	sub	%g1, 625, %g1
	sth	%g1, [r_value]		! sub	$625.,_value
	mov	-5, %g1
	sth	%g1, [r_board + %i3]	! mov	$-5,_board(r3)
	mov	5, %g1
	sth	%g1, [%i4]
	add	%i4, 2, %i4		! mov	$5,(r4)+
	st	%i4, [r_amp]		! mov	r4,_amp
	ret
	restore				! rts	pc

rmove:
	cmp	%i2, 2*63		! cmp	r2,$2*63.
	bne	1f			! bne	1f
	nop

	lduh	[r_flag], %g1
	bclr	01, %g1
	b	move
	sth	%g1, [r_flag]		! bic	$1,_flag
					! br	move
1:
	cmp	%i2, (2*56)		! cmp	r2,$2*56.
	bne	move			! bne	move
	nop

	lduh	[r_flag], %g1
	bclr	02, %g1
	b	move
	sth	%g1, [r_flag]		! bic	$2,_flag
					! br	move

kmove:
	srl	%i3, 1, %i3		! asr	r3
	sethi	%hi(_wkpos), %g1
	sth	%i3, [%g1 + %lo(_wkpos)]! mov	r3,_wkpos
	lduh	[r_flag], %g1
	bclr	03, %g1
	sth	%g1, [r_flag]		! bic	$3,_flag
	cmp	%i2, 2*60		! cmp	r2,$2*60.
	bne	2f			! bne	2f
	cmp	%i3, 62			! cmp	r3,$62	/ kingside castle
	bne	1f			! bne	1f
	nop

	ldsh	[r_value], %g1
	sub	%g1, 1, %g1
	sth	%g1, [r_value]		! dec	_value
	mov	-4, %g1
	sth	%g1, [r_board + (2*61)]	! mov	$-4,_board+[2*61.]
	clrh	[r_board + (2*63)]	! clr	_board+[2*63.]
	mov	2, %g1
	sth	%g1, [%i4]
	add	%i4, 2, %i4		! mov	$2,(r4)+
	st	%i4, [r_amp]		! mov	r4,_amp
	ret
	restore				! rts	pc
1:
	cmp	%i3, 58			! cmp	r3,$58.	/ queenside castle
	bne	2f			! bne	2f
	nop

	ldsh	[r_value], %g1
	sub	%g1, 1, %g1
	sth	%g1, [r_value]		! dec	_value
	mov	-4, %g1
	sth	%g1, [r_board + (2*59)]	! mov	$-4,_board+[2*59.]
	clrh	[r_board + (2*56)]	! clr	_board+[2*56.]
	mov	3, %g1
	sth	%g1, [%i4]
	add	%i4, 2, %i4		! mov	$3,(r4)+
	st	%i4, [r_amp]		! mov	r4,_amp
	ret
	restore				! rts	pc
2:					! 	/ king move
	sethi	%hi(_game), %g1
	ldsh	[%g1 + %lo(_game)], %g1
	tst	%g1			! tst	_game
	bne	1f			! bne	1f
	clrh	[%i4]			!	delay slot

	ldsh	[r_value], %g1
	add	%g1, 2, %g1
	sth	%g1, [r_value]		! add	$2,_value
1:
	add	%i4, 2, %i4		! clr	(r4)+
	st	%i4, [r_amp]		! mov	r4,_amp
	ret
	restore				! rts	pc

qmove:
	sethi	%hi(_game), %g1
	ldsh	[%g1 + %lo(_game)], %g1
	tst	%g1			! tst	_game
	bne	move			! bne	move
	nop

	ldsh	[r_value], %g1
	add	%g1, 1, %g1
	sth	%g1, [r_value]		! inc	_value
					! br	move

nmove:
bmove:
move:
	mov	1, %g1
	sth	%g1, [%i4]
	add	%i4, 2, %i4		! mov	$1,(r4)+
	st	%i4, [r_amp]		! mov	r4,_amp
	ret
	restore				! rts	pc

_wremove:
	save	%sp, -SA(MINFRAME), %sp
	set	_board, r_board
	set	_amp, r_amp
	ld	[r_amp], %i4		! mov	_amp,r4
	ldsh	[%i4 - 2], %o0		! mov	-(r4),r0
	ldsh	[%i4 - 4], %i1		! mov	-(r4),r1
	ldsh	[%i4 - 6], %i3		! mov	-(r4),r3
	ldsh	[%i4 - 8], %i2		! mov	-(r4),r2
	ldsh	[%i4 - 10], %g1
	sethi	%hi(_eppos), %g2
	sth	%g1, [%g2 + %lo(_eppos)]! mov	-(r4),_eppos
	lduh	[%i4 - 12], %g1
	sethi	%hi(_flag), %g2
	sth	%g1, [%g2 + %lo(_flag)]	! mov	-(r4),_flag
	ldsh	[%i4 - 14], %g1
	sethi	%hi(_value), %g2
	sth	%g1, [%g2 + %lo(_value)]! mov	-(r4),_value
	sub	%i4, 14, %i4
	st	%i4, [r_amp]		! mov	r4,_amp
	sll	%i2, 1, %i2		! asl	r2
	sll	%i3, 1, %i3		! asl	r3
	ldsh	[r_board + %i3], %g1
	sth	%g1, [r_board + %i2]	! mov	_board(r3),_board(r2)
	sth	%i1, [r_board + %i3]	! mov	r1,_board(r3)
	sll	%o0, 2, %o0		! asl	r0
	set	0f, %g1
	ld	[%g1 + %o0], %g1
	jmp	%g1			! jmp	*0f(r0)
	nop
0:
	.word	movek			! .word	movek-0b
	.word	movex			! .word	movex-0b
	.word	moveo			! .word	moveo-0b
	.word	moveoo			! .word	moveoo-0b
	.word	movep			! .word	movep-0b
	.word	moveq			! .word	moveq-0b

movek:
	srl	%i2, 1, %i2		! asr	r2
	sethi	%hi(_wkpos), %g1
	sth	%i2, [%g1 + %lo(_wkpos)]! mov	r2,_wkpos

movex:
	ret
	restore				! rts	pc

moveo:
	mov	-4, %g2
	sth	%g2, [r_board + (2*63)]	! mov	$-4,_board+[2*63.]
	clrh	[r_board + (2*61)]	! clr	_board+[2*61.]
	mov	60, %g2
	sethi	%hi(_wkpos), %g1
	sth	%g2, [%g1 + %lo(_wkpos)]! mov	$60.,_wkpos
	ret
	restore				! rts	pc

moveoo:
	mov	-4, %g2
	sth	%g2, [r_board + (2*56)]	! mov	$-4,_board+[2*56.]
	clrh	[r_board + (2*59)]	! clr	_board+[2*59.]
	mov	60, %g2
	sethi	%hi(_wkpos), %g1
	sth	%g2, [%g1 + %lo(_wkpos)]! mov	$60.,_wkpos
	ret
	restore				! rts	pc

movep:
	mov	-1, %g2
	sth	%g2, [r_board + %i2]	! mov	$-1,_board(r2)
	sub	r_board, (2*8), %g1
	clrh	[%g1 + %i3]		! clr	_board-[2*8.](r3)
	ret
	restore				! rts	pc

moveq:
	mov	-1, %g2
	sth	%g2, [r_board + %i2]	! mov	$-1,_board(r2)
	ret
	restore				! rts	pc
