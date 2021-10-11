        .data
        .asciz  "@(#)bgen.s 1.1 92/07/30 Copyr 1984 Sun Micro"
        .even
        .text
				
|       Copyright (c) 1984 by Sun Microsystems, Inc.

#	bgen()
#
# generate moves

.globl	_bgen

.globl	_pval, _board, _dir
.globl	_flag, _lmp, _bkpos
.globl	_eppos
.globl	_value

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

# r0 -> d0 (this is used as a temporary)
# r1 -> d1
# r2 -> a2
# r3 -> a3
# r4 -> a4
# r5 -> a5

atmp = a0
dtmp = d2

_bgen:
	movl	#_dir+126,a4		| mov	$_dir+126.,r4
	movl	#_board+126,a3		| mov	$_board+126.,r3
	movl	_lmp,a2			| mov	_lmp,r2
	movw	#63,d1			| mov	$63.,r1
0:
	movw	a3@,d0			| mov	(r3),r0
	ble	1f			| ble	1f
	aslw	#1,d0			| asl	r0
	movw	pc@(6-2,d0:w),dtmp
	jmp	pc@(2,dtmp:w)		| jmp	*2f(r0)

2:
	.word	pawn-2b
	.word	knight-2b
	.word	bishop-2b
	.word	rook-2b
	.word	queen-2b
	.word	king-2b

pawn:
	movw	#dleft,dtmp
	andw	a4@,dtmp		| bit	$dleft,(r4)
	bne	2f			| bne	2f
	tstw	a3@(2*7)		| tst	2*7.(r3)
	bge	3f			| bge	3f
	jsr	btry	
	.word	0,7*2			| jsr	r5,btry; 0; 7.*2
3:
	movw	d1,d0			| mov	r1,r0
	addw	#7,d0			| add	$7,r0
	cmpw	_eppos,d0		| cmp	r0,_eppos
	bne	2f			| bne	2f
	jsr	btry
	.word	0,-1*2			| jsr	r5,btry; 0; -1*2
2:
	movw	#dright,dtmp
	andw	a4@,dtmp		| bit	$dright,(r4)
	bne	2f			| bne	2f
	tstw	a3@(9*2)		| tst	9.*2(r3)
	bge	3f			| bge	3f
	jsr	btry	
	.word	0, 9*2			| jsr	r5,btry; 0; 9.*2
3:
	movw	d1,d0			| mov	r1,r0
	addw	#9,d0			| add	$9,r0
	cmpw	_eppos,d0		| cmp	r0,_eppos
	bne	2f			| bne	2f
	jsr	btry	
	.word	0,2*1			| jsr	r5,btry; 0; 2*1
2:
	tstw	a3@(2*8)		| tst	2*8.(r3)
	bne	1f			| bne	1f
	jsr	btry	
	.word	0,2*8			| jsr	r5,btry; 0; 2*8.
	movw	#rank7,dtmp
	andw	a4@,dtmp		| bit	$rank7,(r4)
	beq	1f			| beq	1f
	tstw	a3@(2*16)		| tst	2*16.(r3)
	bne	1f			| bne	1f
	jsr	btry 	
	.word	0,16*2			| jsr	r5,btry; 0; 16.*2
	bra	1f			| br	1f

knight:
	jsr	btry	
	.word	u2r1, -15*2		| jsr	r5,btry; u2r1; -15.*2
	jsr	btry	
	.word	u1r2, -6*2		| jsr	r5,btry; u1r2; -6.*2
	jsr	btry	
	.word	d1r2, 10*2		| jsr	r5,btry; d1r2; 10.*2
	jsr	btry	
	.word	d2r1, 17*2		| jsr	r5,btry; d2r1; 17.*2
	jsr	btry	
	.word	d2l1, 15*2		| jsr	r5,btry; d2l1; 15.*2
	jsr	btry	
	.word	d1l2, 6*2		| jsr	r5,btry; d1l2; 6.*2
	jsr	btry	
	.word	u1l2, -10*2		| jsr	r5,btry; u1l2; -10.*2
	jsr	btry	
	.word	u2l1, -17*2		| jsr	r5,btry; u2l1; -17.*2
	bra	1f			| br	1f


1:
	movw	a3@-,dtmp
	cmpw	a4@-,dtmp		| cmp	-(r4),-(r3)
	subqw	#1,d1			| dec	r1
	bpl	0b			| bpl	0b
	movl	a2,_lmp			| mov	r2,_lmp
	rts				| rts	pc

bishop:
	jsr	bslide
	.word	uleft, -9*2		| jsr	r5,bslide; uleft; -9.*2
	jsr	bslide
	.word	uright, -7*2		| jsr	r5,bslide; uright; -7.*2
	jsr	bslide
	.word	dleft, 7*2		| jsr	r5,bslide; dleft; 7.*2
	jsr	bslide
	.word	dright, 9*2		| jsr	r5,bslide; dright; 9.*2
	bra	1b			| br	1b

rook:
	jsr	bslide
	.word	up, -8*2		| jsr	r5,bslide; up; -8.*2
	jsr	bslide
	.word	down, 8*2		| jsr	r5,bslide; down; 8.*2
	jsr	bslide
	.word	left, -1*2		| jsr	r5,bslide; left; -1.*2.
	jsr	bslide
	.word	right, 1*2		| jsr	r5,bslide; right; 1.*2
	bra	1b			| br	1b
queen:
	jsr	bslide
	.word	uleft, -9*2		| jsr	r5,bslide; uleft; -9.*2
	jsr	bslide
	.word	uright, -7*2		| jsr	r5,bslide; uright; -7.*2
	jsr	bslide
	.word	dleft, 7*2		| jsr	r5,bslide; dleft; 7.*2
	jsr	bslide
	.word	dright, 9*2		| jsr	r5,bslide; dright; 9.*2
	jsr	bslide
	.word	up, -8*2		| jsr	r5,bslide; up; -8.*2
	jsr	bslide
	.word	left, -1*2		| jsr	r5,bslide; left; -1.*2
	jsr	bslide
	.word	right, 1*2		| jsr	r5,bslide; right; 1.*2
	jsr	bslide
	.word	down, 8*2		| jsr	r5,bslide; down; 8.*2
	bra	1b			| br	1b

king:
	jsr	btry
	.word	uleft, -9*2		| jsr	r5,btry; uleft; -9.*2
	jsr	btry
	.word	uright, -7*2		| jsr	r5,btry; uright; -7.*2
	jsr	btry
	.word	dleft, 7*2		| jsr	r5,btry; dleft; 7.*2
	jsr	btry
	.word	dright, 9*2		| jsr	r5,btry; dright; 9.*2
	jsr	btry
	.word	up, -8*2		| jsr	r5,btry; up; -8.*2
	jsr	btry
	.word	left, -1*2		| jsr	r5,btry; left; -1.*2
	jsr	btry
	.word	right, 1*2		| jsr	r5,btry; right; 1.*2
	jsr	btry
	.word	down, 8*2		| jsr	r5,btry; down; 8.*2
	bra	1b			| br	1b

btry:
	movl	sp@,a5			|	implicit in pdp-11 jsr
	movw	a5@+,dtmp
	andw	a4@,dtmp		| bit	(r5)+,(r4)
	bne	1f			| bne	1f
	movw	a5@,d0
	extl	d0
	movl	d0,atmp			| mov	r3,r0
	addl	a3,atmp			| add	(r5),r0
	movw	atmp@,d0		| mov	(r0),r0
	bgt	1f			| bgt	1f
	aslw	#1,d0			| asl	r0
	extl	d0
	movl	d0,atmp
	addl	#_pval+12,atmp
	movw	atmp@,a2@		| mov	_pval+12.(r0),(r2)
	movw	_value,dtmp	
	subw	dtmp,a2@+		| sub	_value,(r2)+
	movw	a5@+,d0			| mov	(r5)+,r0
	asrw	#1,d0			| asr	r0
	addw	d1,d0			| add	r1,r0
					|	switch byte order
	movb	d1,a2@+			| movb	r0,(r2)+
	movb	d0,a2@+			| movb	r1,(r2)+
	movl	a5,sp@
	rts				| rts	r5
1:
	tstw	a5@+			| tst	(r5)+
	movl	a5,sp@
	rts				| rts	r5

bslide:
	movl	sp@,a5			|	implicit in pdp-11 jsr
	movl	a4,sp@-			| mov	r4,-(sp)
	movl	a3,sp@-			| mov	r3,-(sp)
1:
	movw	a5@+,dtmp
	andw	a4@,dtmp		| bit	(r5)+,(r4)
	bne	1f			| bne	1f
	movw	a5@,dtmp
	extl	dtmp
	addl	dtmp,a3			| add	(r5),r3
	addl	dtmp,a4			| add	(r5),r4
	movw	a3@,d0			| mov	(r3),r0
	bgt	1f			| bgt	1f
	blt	2f			| blt	2f
	clrw	a2@			| clr	(r2)
	movw	_value,dtmp
	subw	dtmp,a2@+		| sub	_value,(r2)+
	movl	a3,d0			| mov	r3,r0
	subl	#_board,d0		| sub	$_board,r0
	asrw	#1,d0			| asr	r0
					|	swap bytes
	movb	d1,a2@+			| movb	r0,(r2)+
	movb	d0,a2@+			| movb	r1,(r2)+
	tstw	a5@-			| tst	-(r5)
	bra	1b			| br	1b
2:
	aslw	#1,d0			| asl	r0
	extl	d0
	movl	d0,atmp
	addl	#_pval+12,atmp
	movw	atmp@,a2@		| mov	_pval+12.(r0),(r2)
	movw	_value,dtmp
	subw	dtmp,a2@+		| sub	_value,(r2)+
	movl	a3,d0			| mov	r3,r0
	subl	#_board,d0		| sub	$_board,r0
	asrw	#1,d0			| asr	r0
					|	swap bytes
	movb	d1,a2@+			| movb	r0,(r2)+
	movb	d0,a2@+			| movb	r1,(r2)+
1:
	tstw	a5@+			| tst	(r5)+
	movl	sp@+,a3			| mov	(sp)+,r3
	movl	sp@+,a4			| mov	(sp)+,r4
	movl	a5,sp@
	rts				| rts	r5
