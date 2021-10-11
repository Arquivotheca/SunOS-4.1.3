        .data
        .asciz  "@(#)wmove.s 1.1 92/07/30 Copyr 1984 Sun Micro"
        .even
        .text
				
|       Copyright (c) 1984 by Sun Microsystems, Inc.

#	wmove(int), wremove()
#
.globl	_wmove, _wremove
.globl	_board, _pval, _amp, _flag, _eppos, _value, _wkpos
.globl	_game

# r0 -> d0
# r1 -> d1
# r2 -> d2
# r3 -> d3
# r4 -> a4

atmp =  a0
atmp1 = a1
dtmp =  d5

_wmove:
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
	bge	error			| bge	error
	movw	pc@(0f-start+6,d0:w),dtmp
	jmp	pc@(2,dtmp:w)		| jmp	*0f(r0)	/ type of man
start:
	.word	kmove-start
	.word	qmove-start
	.word	rmove-start
	.word	bmove-start
	.word	nmove-start
	.word	pmove-start
0:
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
	addl	#-16,atmp
	movw	#-1,atmp@		| mov	$-1,_board-16.(r3)
	movw	#4,a4@+			| mov	$4,(r4)+
	movl	a4,_amp			| mov	r4,_amp
	rts				| rts	pc
1:
	cmpw	#2*16,d2		| cmp	r2,$2*16.	/ double move
	bne	1f			| bne	1f
	movb	sp@(6),d2
	extw	d2			| movb	3(sp),r2
	subw	#8,d2			| sub	$8,r2
	movw	d2,_eppos		| mov	r2,_eppos
	bra	move			| br	move
1:
	cmpw	#24*2,d3		| cmp	r3,$24.*2
	bge	move			| bge	move
	subw	#25,_value		| sub	$25.,_value
	cmpw	#16*2,d3		| cmp	r3,$16.*2
	bge	move			| bge	move
	subw	#50,_value		| sub	$50.,_value
	cmpw	#8*2,d3			| cmp	r3,$8.*2   / queen promotion
	bge	move			| bge	move
	subw	#625,_value		| sub	$625.,_value
	extl	d3
	movl	d3,atmp
	addl	#_board,atmp
	movw	#-5,atmp@		| mov	$-5,_board(r3)
	movw	#5,a4@+			| mov	$5,(r4)+
	movl	a4,_amp			| mov	r4,_amp
	rts				| rts	pc

rmove:
	cmpw	#2*63,d2		| cmp	r2,$2*63.
	bne	1f			| bne	1f
	movw	#~1,dtmp
	andw	dtmp,_flag		| bic	$1,_flag
	bra	move			| br	move
1:
	cmpw	#2*56,d2		| cmp	r2,$2*56.
	bne	move			| bne	move
	movw	#~2,dtmp
	andw	dtmp,_flag		| bic	$2,_flag
	bra	move			| br	move

kmove:
	asrw	#1,d3			| asr	r3
	movw	d3,_wkpos		| mov	r3,_wkpos
	movw	#~3,dtmp
	andw	dtmp,_flag		| bic	$3,_flag
	cmpw	#2*60,d2		| cmp	r2,$2*60.
	bne	2f			| bne	2f
	cmpw	#62,d3			| cmp	r3,$62.	/ kingside castle
	bne	1f			| bne	1f
	subqw	#1,_value		| dec	_value
	movw	#-4,_board+(2*61)	| mov	$-4,_board+[2*61.]
	clrw	_board+(2*63)		| clr	_board+[2*63.]
	movw	#2,a4@+			| mov	$2,(r4)+
	movl	a4,_amp			| mov	r4,_amp
	rts				| rts	pc
1:
	cmpw	#58,d3			| cmp	r3,$58.	/ queenside castle
	bne	2f			| bne	2f
	subqw	#1,_value		| dec	_value
	movw	#-4,_board+(2*59)	| mov	$-4,_board+[2*59.]
	clrw	_board+(2*56)		| clr	_board+[2*56.]
	movw	#3,a4@+			| mov	$3,(r4)+
	movl	a4,_amp			| mov	r4,_amp
	rts				| rts	pc
2:					| 	/ king move
	tstw	_game			| tst	_game
	bne	1f			| bne	1f
	addw	#2,_value		| add	$2,_value
1:
	clrw	a4@+			| clr	(r4)+
	movl	a4,_amp			| mov	r4,_amp
	rts				| rts	pc

qmove:
	tstw	_game			| tst	_game
	bne	move			| bne	move
	addqw	#1,_value		| inc	_value
	bra	move			| br	move

nmove:
bmove:
move:
	movw	#1,a4@+			| mov	$1,(r4)+
	movl	a4,_amp			| mov	r4,_amp
	rts				| rts	pc

_wremove:
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
	movw	d2,_wkpos		| mov	r2,_wkpos

movex:
	rts				| rts	pc

moveo:
	movw	#-4,_board+(2*63)	| mov	$-4,_board+[2*63.]
	clrw	_board+(2*61)		| clr	_board+[2*61.]
	movw	#60,_wkpos		| mov	$60.,_wkpos	
	rts				| rts	pc

moveoo:
	movw	#-4,_board+(2*56)	| mov	$-4,_board+[2*56.]
	clrw	_board+(2*59)		| clr	_board+[2*59.]
	movw	#60,_wkpos;		| mov	$60.,_wkpos;
	rts				| rts	pc

movep:
	extl	d2
	movl	d2,atmp
	addl	#_board,atmp
	movw	#-1,atmp@		| mov	$-1,_board(r2)
	extl	d3
	movl	d3,atmp
	addl	#_board-(2*8),atmp
	clrw	atmp@			| clr	_board-[2*8.](r3)
	rts				| rts	pc

moveq:
	extl	d2
	movl	d2,atmp
	addl	#_board,atmp
	movw	#-1,atmp@		| mov	$-1,_board(r2)
	rts				| rts	pc
