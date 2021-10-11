        .data
        .asciz  "@(#)attw.s 1.1 92/07/30 Copyr 1984 Sun Micro"
        .even
        .text
				
|       Copyright (c) 1984 by Sun Microsystems, Inc.

#	wattack(int)
#
# does white/black attack position?

.globl	_wattack

.globl	_dir, _board

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

# r0 d0
# r1 d1
# r2 d2
# r3 d3
# r4 d4
# r5 a5

atmp = a0
dtmp = d6

_wattack:
	movw	sp@(6),d0		| mov	2(sp),r0
	aslw	#1,d0			| asl	r0
	extl	d0			|	from now on, is extled
	movl	d0,atmp
	addl	#_dir,atmp
	movw	atmp@,d1		| mov	_dir(r0),r1
	movw	#-2,d2			| mov	$-2,r2
	movw	#u2r1,dtmp
	andw	d1,dtmp			| bit	$u2r1,r1
	bne	1f			| bne	1f
	movl	d0,atmp
	addl	#_board+(-15*2),atmp
	cmpw	atmp@,d2		| cmp	_board+[-15.*2](r0),r2
	beq	2f			| beq	2f
1:
	movw	#u1r2,dtmp
	andw	d1,dtmp			| bit	$u1r2,r1
	bne	1f			| bne	1f
	movl	d0,atmp
	addl	#_board+(-6*2),atmp
	cmpw	atmp@,d2		| cmp	_board+[-6.*2](r0),r2
	beq	2f			| beq	2f
1:
	movw	#d1r2,dtmp
	andw	d1,dtmp			| bit	$d1r2,r1
	bne	1f			| bne	1f
	movl	d0,atmp
	addl	#_board+(10*2),atmp
	cmpw	atmp@,d2		| cmp	_board+[+10.*2](r0),r2
	beq	2f			| beq	2f
1:
	movw	#d2r1,dtmp
	andw	d1,dtmp			| bit	$d2r1,r1
	bne	1f			| bne	1f
	movl	d0,atmp
	addl	#_board+(17*2),atmp
	cmpw	atmp@,d2		| cmp	_board+[+17.*2](r0),r2
	beq	2f			| beq	2f
1:
	movw	#d2l1,dtmp
	andw	d1,dtmp			| bit	$d2l1,r1
	bne	1f			| bne	1f
	movl	d0,atmp
	addl	#_board+(15*2),atmp
	cmpw	atmp@,d2		| cmp	_board+[+15.*2](r0),r2
	beq	2f			| beq	2f
1:
	movw	#d1l2,dtmp
	andw	d1,dtmp			| bit	$d1l2,r1
	bne	1f			| bne	1f
	movl	d0,atmp
	addl	#_board+(6*2),atmp
	cmpw	atmp@,d2		| cmp	_board+[+6.*2](r0),r2
	beq	2f			| beq	2f
1:
	movw	#u1l2,dtmp
	andw	d1,dtmp			| bit	$u1l2,r1
	bne	1f			| bne	1f
	movl	d0,atmp
	addl	#_board+(-10*2),atmp
	cmpw	atmp@,d2		| cmp	_board+[-10.*2](r0),r2
	beq	2f			| beq	2f
1:
	movw	#u2l1,dtmp
	andw	d1,dtmp			| bit	$u2l1,r1
	bne	1f			| bne	1f
	movl	d0,atmp
	addl	#_board+(-17*2),atmp
	cmpw	atmp@,d2		| cmp	_board+[-17.*2](r0),r2
	beq	2f			| beq	2f
1:
	jsr	wadiag
	.word	uleft, -9*2		| jsr	r5,wadiag; uleft; -9.*2
	jsr	wadiag
	.word	uright, -7*2		| jsr	r5,wadiag; uright; -7.*2
	jsr	wadiag
	.word	dleft, 7*2		| jsr	r5,wadiag; dleft; 7.*2
	jsr	wadiag
	.word	dright, 9*2		| jsr	r5,wadiag; dright; 9.*2
	jsr	warank
	.word	up, -8*2		| jsr	r5,warank; up; -8.*2
	jsr	warank
	.word	left, -1*2		| jsr	r5,warank; left; -1.*2
	jsr	warank
	.word	right, 1*2		| jsr	r5,warank; right; 1.*2
	jsr	warank
	.word	down, 8*2		| jsr	r5,warank; down; 8.*2

	movl	d0,atmp
	addl	#_dir,atmp
	movw	atmp@,dtmp
	andw	#dleft,dtmp		| bit	$dleft,_dir(r0)
	
	bne	1f			| bne	1f
	movl	d0,atmp
	addl	#_board+14,atmp
	movw	atmp@,dtmp
	cmpw	#-1,dtmp		| cmp	_board+14.(r0),$-1  / pawn?
	beq	2f			| beq	2f
1:
	movl	d0,atmp
	addl	#_dir,atmp
	movw	atmp@,dtmp
	andw	#dright,dtmp		| bit	$dright,_dir(r0)
	bne	1f			| bne	1f
	movl	d0,atmp
	addl	#_board+18,atmp
	movw	atmp@,dtmp
	cmpw	#-1,dtmp		| cmp	_board+18.(r0),$-1
	bne	1f			| bne	1f
2:
	clrl	d0			| clr	r0
	rts				| rts	pc
1:
	movl	#1,d0			| mov	$1,r0
	rts				| rts	pc

wadiag:
	movl	sp@,a5			|	implicit in pdp-11 jsr
	movw	d0,d1			| mov	r0,r1
	movw	a5@+,d2			| mov	(r5)+,r2
	movw	a5@+,d3			| mov	(r5)+,r3
	movl	d0,atmp
	addl	#_dir,atmp
	movw	atmp@,dtmp
	andw	d2,dtmp			| bit	r2,_dir(r1)
	bne	1f			| bne	1f
	addw	d3,d1			| add	r3,r1
	extl	d1
	movl	d1,atmp
	addl	#_board,atmp
	movw	atmp@,d4		| mov	_board(r1),r4
	beq	2f			| beq	2f
	cmpw	#-3,d4			| cmp	r4,$-3
	beq	9f			| beq	9f
	cmpw	#-5,d4			| cmp	r4,$-5
	beq	9f			| beq	9f
	cmpw	#-6,d4			| cmp	r4,$-6
	beq	9f			| beq	9f
1:
	movl	a5,sp@
	rts				| rts	r5
2:
	extl	d1
	movl	d1,atmp
	addl	#_dir,atmp
	movw	atmp@,dtmp
	andw	d2,dtmp			| bit	r2,_dir(r1)
	bne	2f			| bne	2f
	addw	d3,d1			| add	r3,r1
	extl	d1
	movl	d1,atmp
	addl	#_board,atmp
	movw	atmp@,d4		| mov	_board(r1),r4
	beq	2b			| beq	2b
	cmpw	#-3,d4			| cmp	r4,$-3
	beq	9f			| beq	9f
	cmpw	#-5,d4			| cmp	r4,$-5
	beq	9f			| beq	9f
2:
	movl	a5,sp@
	rts				| rts	r5

warank:
	movl	sp@,a5			|	implicit in pdp-11 jsr
	movw	d0,d1			| mov	r0,r1
	movw	a5@+,d2			| mov	(r5)+,r2
	movw	a5@+,d3			| mov	(r5)+,r3
	extl	d1
	movl	d1,atmp
	addl	#_dir,atmp
	movw	atmp@,dtmp
	andw	d2,dtmp			| bit	r2,_dir(r1)
	bne	1f			| bne	1f
	addw	d3,d1			| add	r3,r1
	extl	d1
	movl	d1,atmp
	addl	#_board,atmp
	movw	atmp@,d4		| mov	_board(r1),r4
	beq	2f			| beq	2f
	cmpw	#-4,d4			| cmp	r4,$-4
	beq	9f			| beq	9f
	cmpw	#-5,d4			| cmp	r4,$-5
	beq	9f			| beq	9f
	cmpw	#-6,d4			| cmp	r4,$-6
	beq	9f			| beq	9f
1:
	movl	a5,sp@
	rts				| rts	r5
2:
	extl	d1
	movl	d1,atmp
	addl	#_dir,atmp
	movw	atmp@,dtmp
	andw	d2,dtmp			| bit	r2,_dir(r1)
	bne	2f			| bne	2f
	addw	d3,d1			| add	r3,r1
	extl	d1
	movl	d1,atmp
	addl	#_board,atmp
	movw	atmp@,d4		| mov	_board(r1),r4
	beq	2b			| beq	2b
	cmpw	#-4,d4			| cmp	r4,$-4
	beq	9f			| beq	9f
	cmpw	#-5,d4			| cmp	r4,$-5
	beq	9f			| beq	9f
2:
	movl	a5,sp@
	rts				| rts	r5

9:
	movl	sp@+,a5			| mov	(sp)+,r5
	clrl	d0			| clr	r0
	rts				| rts	pc
