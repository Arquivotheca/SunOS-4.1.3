        .data
        .asciz  "@(#)ctrl.s 1.1 92/07/30 Copyr 1984 Sun Micro"
        .even
        .text
				
|       Copyright (c) 1984 by Sun Microsystems, Inc.

#	attack(int)
#
# list pieces controlling a square

.globl	_attack

.globl	_dir, _board
.globl	_attacv

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

# r0 -> d0
# r1 -> d1
# r4 -> a4
# r5 -> a5

atmp = a0
dtmp = d2

_attack:
	movw	sp@(6),d0		| mov	2(sp),r0
	aslw	#1,d0			| asl	r0
	extl	d0			|	for future use
	movl	#_attacv,a4		| mov	$_attacv,r4

	jsr	patt			| jsr	r5,patt
	.word	u2r1, -15*2, 2, -2
	jsr	patt			| jsr	r5,patt
	.word	u1r2, -6*2, 2, -2
	jsr	patt			| jsr	r5,patt
	.word	d2r1, 17*2, 2, -2
	jsr	patt			| jsr	r5,patt
	.word	d2l1, 15*2, 2, -2
	jsr	patt			| jsr	r5,patt
	.word	d1l2, 6*2, 2, -2
	jsr	patt			| jsr	r5,patt
	.word	u1l2, -10*2, 2, -2
	jsr	patt			| jsr	r5,patt
	.word	u2l1, -17*2, 2, -2

	jsr	satt			| jsr	r5,satt
	.word	uleft, -9*2, 1, 3, -3, 5, -5
	jsr	satt			| jsr	r5,satt
	.word	uright, -7*2, 1, 3, -3, 5, -5
	jsr	satt			| jsr	r5,satt
	.word	dleft, 7*2, -1, 3, -3, 5, -5
	jsr	satt			| jsr	r5,satt
	.word	dright, 9*2, -1, 3, -3, 5, -5
	jsr	satt			| jsr	r5,satt
	.word	up, -8*2, none, 4, -4, 5, -5
	jsr	satt			| jsr	r5,satt
	.word	left, -1*2, none, 4, -4, 5, -5
	jsr	satt			| jsr	r5,satt
	.word	right, 1*2, none, 4, -4, 5, -5
	jsr	satt			| jsr	r5,satt
	.word	down, 8*2, none, 4, -4, 5, -5
	clrw	a4@+			| clr	(r4)+
	rts				| rts	pc

patt:
	movl	sp@,a5			|	implicit in pdp-11 jsr
	movl	d0,atmp			|	ok, because did extl on d0
	addl	#_dir,atmp
	movw	atmp@,dtmp
	andw	a5@+,dtmp		| bit	(r5)+,_dir(r0)
	bne	1f			| bne	1f
	movw	d0,d1			| mov	r0,r1
	addw	a5@+,d1			| add	(r5)+,r1
	jsr	look			| jsr	pc,look
	jsr	look			| jsr	pc,look
	movl	a5,sp@
	rts				| rts	r5
1:
	addw	#6,a5			| add	$6,r5
	movl	a5,sp@
	rts				| rts	r5

satt:
	movl	sp@,a5			|	implicit in pdp-11 jsr
	movl	a5,sp@-			| mov	r5,-(sp)
	movl	d0, atmp		|	ok, because did extl on d0
	addl	#_dir,atmp
	movw	atmp@,dtmp
	andw	a5@+,dtmp		| bit	r5@+,_dir(r0)
	bne	1f			| bne	1f
	movw	d0,d1			| mov	r0,r1
	addw	a5@+,d1			| add	(r5)+,r1
	jsr	look			| jsr	pc,look		/ pawn
	movw	d0,d1			| mov	r0,r1
2:
	movl	sp@,a5			| mov	(sp),r5
	extl	d1
	movl	d1,atmp
	addl	#_dir,atmp
	movw	atmp@,dtmp
	andw	a5@+,dtmp		| bit	(r5)+,_dir(r1)
	bne	1f			| bne	1f
	addw	a5@+,d1			| add	(r5)+,r1
	
	extl	d1
	movl	d1,atmp
	addl	#_board,atmp
	tstw	atmp@			| tst	_board(r1)
	beq	2b			| beq	2b
	tstw	a5@+			| tst	(r5)+
	movl	a4,sp@-			| mov	r4,-(sp)
	jsr	look			| jsr	pc,look
	jsr	look			| jsr	pc,look
	jsr	look			| jsr	pc,look
	jsr	look			| jsr	pc,look
	cmpl	sp@+,a4			| cmp	(sp)+,r4
	bne	2b			| bne	2b
1:
	movl	sp@+,a5			| mov	(sp)+,r5
	addl	#14,a5			| add	$14.,r5
	movl	a5,sp@
	rts				| rts	r5

look:
	extl	d1
	movl	d1,atmp
	addl	#_board,atmp
	movw	atmp@,dtmp
	cmpw	a5@+,dtmp		| cmp	(r5)+,_board(r1)
	bne	1f			| bne	1f
	movw	a5@(-2),a4@+		| mov	-2(r5),(r4)+
1:
	rts				| rts	pc
