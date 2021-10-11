        .data
        .asciz  "@(#)bmove.s 1.1 92/07/30 Copyr 1984 Sun Micro"
        .even
        .text
				
|       Copyright (c) 1984 by Sun Microsystems, Inc.

#	bmove(int), bremove()
#
.globl	_bmove, _bremove
.globl	_board, _pval, _amp, _flag, _eppos, _value, _bkpos
.globl	_game

# r0 -> d0
# r1 -> d1
# r2 -> d2
# r3 -> d3
# r4 -> a4

atmp =  a0
atmp1 = a1
dtmp =  d5

_bmove:
	movl	_amp,a4			| mov	_amp,r4
	movb	sp@(7),d3
	extw	d3			| movb	2(sp),r3	/ to
	movb	sp@(6),d2
	extw	d2			| movb	3(sp),r2	/ from
	movw	_value,a4@+		| mov	_value,(r4)+
	movw	_flag,a4@+		| mov	_flag,(r4)+
	movw	_eppos,a4@+		| mov	_eppos,(r4)+
	movw	d2,a4@+			| mov	r2,(r4)+
	movw	d3,a4@+			| mov	r3,(r4)+
	aslw	#1,d2			| asl	r2	/ from as a word index
	aslw	#1,d3			| asl	r3	/ to as word index
	extl	d3
	movl	d3,atmp
	addl	#_board,atmp
	movw	atmp@,d0		| mov	_board(r3),r0
	movw	d0,a4@+			| mov	r0,(r4)+
	beq	1f			| beq	1f
	aslw	#1,d0			| asl	r0

	extl	d0
	movl	d0,atmp
	addl	#_pval+12,atmp
	movw	atmp@,dtmp
	subw	dtmp,_value		| sub	_pval+12.(r0),_value
1:
	extl	d2
	movl	d2,atmp
	addl	#_board,atmp
	movw	atmp@,d0		| mov	_board(r2),r0
	extl	d3
	movl	d3,atmp1
	addl	#_board,atmp1
	movw	d0,atmp1@		| mov	r0,_board(r3)
	clrw	atmp@			| clr	_board(r2)
	movw	#-1,_eppos		| mov	$-1,_eppos
	aslw	#1,d0			| asl	r0
	ble	error			| ble	error
	movw	pc@(6-2,d0:w),dtmp
	jmp	pc@(2,dtmp:w)		| jmp	*0f-2(r0)	/ type of man
0:
	.word	pmove-0b
	.word	nmove-0b
	.word	bmove-0b
	.word	rmove-0b
	.word	qmove-0b
	.word	kmove-0b

error:
	jsr	_abort			| 3

pmove:
	subw	d3,d2			| sub	r3,r2
	bge	1f			| bge	1f
	negw	d2			| neg	r2
1:
	cmpw	#2*1,d2			| cmp	r2,$2*1		/ ep capture
	bne	1f			| bne	1f
	
	extl	d3
	movl	d3,atmp
	addl	#_board,atmp
	clrw	atmp@			| clr	_board(r3)
	addl	#2*8,atmp
	movw	#1,atmp@		| mov	$1,_board+[2*8.](r3)
	movw	#4,a4@+			| mov	$4,(r4)+
	movl	a4,_amp			| mov	r4,_amp
	rts				| rts	pc
1:
	cmpw	#2*16,d2		| cmp	r2,$2*16.	/ double move
	bne	1f			| bne	1f
	movb	sp@(6),d2
	extw	d2			| movb	3(sp),r2
	addw	#8,d2			| add	$8,r2
	movw	d2,_eppos		| mov	r2,_eppos
	bra	move			| br	move
1:
	cmpw	#40*2,d3		| cmp	r3,$40.*2
	blt	move			| blt	move
	addw	#25,_value		| add	$25.,_value
	cmpw	#48*2,d3		| cmp	r3,$48.*2
	blt	move			| blt	move
	addw	#50,_value		| add	$50.,_value
	cmpw	#56*2,d3		| cmp	r3,$56.*2   / queen promotion
	blt	move			| blt	move
	addw	#625,_value		| add	$625.,_value
	extl	d3
	movl	d3,atmp
	addl	#_board,atmp
	movw	#5,atmp@		| mov	$5,_board(r3)
	movw	#5,a4@+			| mov	$5,(r4)+
	movl	a4,_amp			| mov	r4,_amp
	rts				| rts	pc

rmove:
	cmpw	#2*7,d2			| cmp	r2,$2*7.
	bne	1f			| bne	1f
	movw	#~010,dtmp
	andw	dtmp,_flag		| bic	$10,_flag
	bra	move			| br	move
1:
	tstw	d2			| tst	r2
	bne	move			| bne	move
	movw	#~020,dtmp
	andw	dtmp,_flag		| bic	$20,_flag
	bra	move			| br	move

kmove:
	asrw	#1,d3			| asr	r3
	movw	d3,_bkpos		| mov	r3,_bkpos
	movw	#~030,dtmp
	andw	dtmp,_flag		| bic	$30,_flag
	cmpw	#2*4,d2			| cmp	r2,$2*4.
	bne	2f			| bne	2f
	cmpw	#6,d3			| cmp	r3,$6	/ kingside castle
	bne	1f			| bne	1f
	addqw	#1,_value		| inc	_value
	movw	#4,_board+(2*5)		| mov	$4,_board+[2*5.]
	clrw	_board+(2*7)		| clr	_board+[2*7.]
	movw	#2,a4@+			| mov	$2,(r4)+
	movl	a4,_amp			| mov	r4,_amp
	rts				| rts	pc
1:
	cmpw	#2,d3			| cmp	r3,$2	/ queenside castle
	bne	2f			| bne	2f
	addqw	#1,_value		| inc	_value
	movw	#4,_board+(2*3)		| mov	$4,_board+[2*3.]
	clrw	_board+(2*0)		| clr	_board+[2*0.]
	movw	#3,a4@+			| mov	$3,(r4)+
	movl	a4,_amp			| mov	r4,_amp
	rts				| rts	pc
2:					| 	/ king move
	tstw	_game			| tst	_game
	bne	1f			| bne	1f
	subw	#2,_value		| sub	$2,_value
1:
	clrw	a4@+			| clr	(r4)+
	movl	a4,_amp			| mov	r4,_amp
	rts				| rts	pc

qmove:
	tstw	_game			| tst	_game
	bne	move			| bne	move
	subqw	#1,_value		| dec	_value
	bra	move			| br	move

nmove:
bmove:
move:
	movw	#1,a4@+			| mov	$1,(r4)+
	movl	a4,_amp			| mov	r4,_amp
	rts				| rts	pc

_bremove:
	movl	_amp,a4			| mov	_amp,r4
	movw	a4@-,d0			| mov	-(r4),r0
	movw	a4@-,d1			| mov	-(r4),r1
	movw	a4@-,d3			| mov	-(r4),r3
	movw	a4@-,d2			| mov	-(r4),r2
	movw	a4@-,_eppos		| mov	-(r4),_eppos
	movw	a4@-,_flag		| mov	-(r4),_flag
	movw	a4@-,_value		| mov	-(r4),_value
	movl	a4,_amp			| mov	r4,_amp
	aslw	#1,d2			| asl	r2
	aslw	#1,d3			| asl	r3
	extl	d3
	movl	d3,atmp
	addl	#_board,atmp
	extl	d2
	movl	d2,atmp1
	addl	#_board,atmp1
	movw	atmp@,atmp1@		| mov	_board(r3),_board(r2)
	movw	d1,atmp@		| mov	r1,_board(r3)
	aslw	#1,d0			| asl	r0
	movw	pc@(6,d0:w),dtmp
	jmp	pc@(2,dtmp:w)		| jmp	*0f(r0)
0:
	.word	movek-0b
	.word	movex-0b
	.word	moveo-0b
	.word	moveoo-0b
	.word	movep-0b
	.word	moveq-0b

movek:
	asrw	#1,d2			| asr	r2
	movw	d2,_bkpos		| mov	r2,_bkpos

movex:
	rts				| rts	pc

moveo:
	movw	#4,_board+(2*7)		| mov	$4,_board+[2*7.]
	clrw	_board+(2*5)		| clr	_board+[2*5]
	movw	#4,_bkpos		| mov	$4,_bkpos
	rts				| rts	pc

moveoo:
	movw	#4,_board+(2*0)		| mov	$4,_board+[2*0]
	clrw	_board+(2*3)		| clr	_board+[2*3]
	movw	#4,_bkpos;			| mov	$4,_bkpos;
	rts				| rts	pc

movep:
	extl	d2
	movl	d2,atmp
	addl	#_board,atmp
	movw	#1,atmp@		| mov	$1,_board(r2)
	extl	d3
	movl	d3,atmp
	addl	#_board+(2*8),atmp
	clrw	atmp@			| clr	_board+[2*8.](r3)
	rts				| rts	pc

moveq:
	extl	d2
	movl	d2,atmp
	addl	#_board,atmp
	movw	#1,atmp@		| mov	$1,_board(r2)
	rts				| rts	pc
