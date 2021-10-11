        .data
        .asciz  "@(#)wgen.s 1.1 92/07/30 Copyr 1984 Sun Micro"
        .even
        .text
				
|       Copyright (c) 1984 by Sun Microsystems, Inc.

#	wgen()
#
# generate moves

.globl	_wgen

.globl	_pval, _board, _dir
.globl	_flag, _lmp, _wkpos
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

_wgen:
	movl	#_dir+126,a4		| mov	$_dir+126.,r4
	movl	#_board+126,a3		| mov	$_board+126.,r3
	movl	_lmp,a2			| mov	_lmp,r2
	movw	#63,d1			| mov	$63.,r1
0:
	movw	a3@,d0			| mov	(r3),r0
	bge	1f			| bge	1f
	aslw	#1,d0			| asl	r0
	movw	pc@(2f-start+6,d0:w),dtmp
	jmp	pc@(2,dtmp:w)		| jmp	*2f(r0)
start:
	.word	wking-start
	.word	wqueen-start
	.word	wrook-start
	.word	wbishop-start
	.word	wknight-start
	.word	wpawn-start
2:

wpawn:
	movw	#uleft,dtmp
	andw	a4@,dtmp		| bit	$uleft,(r4)
	bne	2f			| bne	2f
	tstw	a3@(-2*9)		| tst	-2*9.(r3)
	ble	3f			| ble	3f
	jsr	wwtry	
	.word	0,-9*2			| jsr	r5,wwtry; 0; -9.*2
3:
	movw	d1,d0			| mov	r1,r0
	subw	#9,d0			| sub	$9,r0
	cmpw	_eppos,d0		| cmp	r0,_eppos
	bne	2f			| bne	2f
	jsr	wwtry
	.word	0,-1*2			| jsr	r5,wwtry; 0; -1*2
2:
	movw	#uright,dtmp
	andw	a4@,dtmp		| bit	$uright,(r4)
	bne	2f			| bne	2f
	tstw	a3@(-7*2)		| tst	-7.*2(r3)
	ble	3f			| ble	3f
	jsr	wwtry	
	.word	0, -7*2 		| jsr	r5,wwtry; 0; -7.*2
3:
	movw	d1,d0			| mov	r1,r0
	subw	#7,d0			| sub	$7,r0
	cmpw	_eppos,d0		| cmp	r0,_eppos
	bne	2f			| bne	2f
	jsr	wwtry	
	.word	0,2*1			| jsr	r5,wwtry; 0; 2*1
2:
	tstw	a3@(-2*8)		| tst	-2*8.(r3)
	bne	1f			| bne	1f
	jsr	wwtry	
	.word	0,-2*8			| jsr	r5,wwtry; 0; -2*8.
	movw	#rank2,dtmp
	andw	a4@,dtmp		| bit	$rank2,(r4)
	beq	1f			| beq	1f
	tstw	a3@(-2*16)		| tst	-2*16.(r3)
	bne	1f			| bne	1f
	jsr	wwtry 	
	.word	0,-16*2			| jsr	r5,wwtry; 0; -16.*2
	bra	1f			| br	1f

wknight:
	jsr	wwtry	
	.word	u2r1, -15*2		| jsr	r5,wwtry; u2r1; -15.*2
	jsr	wwtry	
	.word	u1r2, -6*2		| jsr	r5,wwtry; u1r2; -6.*2
	jsr	wwtry	
	.word	d1r2, 10*2		| jsr	r5,wwtry; d1r2; 10.*2
	jsr	wwtry	
	.word	d2r1, 17*2		| jsr	r5,wwtry; d2r1; 17.*2
	jsr	wwtry	
	.word	d2l1, 15*2		| jsr	r5,wwtry; d2l1; 15.*2
	jsr	wwtry	
	.word	d1l2, 6*2		| jsr	r5,wwtry; d1l2; 6.*2
	jsr	wwtry	
	.word	u1l2, -10*2		| jsr	r5,wwtry; u1l2; -10.*2
	jsr	wwtry	
	.word	u2l1, -17*2		| jsr	r5,wwtry; u2l1; -17.*2
	bra	1f			| br	1f


1:
	movw	a3@-,dtmp
	cmpw	a4@-,dtmp		| cmp	-(r4),-(r3)
	subqw	#1,d1			| dec	r1
	bpl	0b			| bpl	0b
	movl	a2,_lmp			| mov	r2,_lmp
	rts				| rts	pc

wbishop:
	jsr	wslide
	.word	uleft, -9*2		| jsr	r5,wslide; uleft; -9.*2
	jsr	wslide
	.word	uright, -7*2		| jsr	r5,wslide; uright; -7.*2
	jsr	wslide
	.word	dleft, 7*2		| jsr	r5,wslide; dleft; 7.*2
	jsr	wslide
	.word	dright, 9*2		| jsr	r5,wslide; dright; 9.*2
	bra	1b			| br	1b

wrook:
	jsr	wslide
	.word	up, -8*2		| jsr	r5,wslide; up; -8.*2
	jsr	wslide
	.word	down, 8*2		| jsr	r5,wslide; down; 8.*2
	jsr	wslide
	.word	left, -1*2		| jsr	r5,wslide; left; -1.*2.
	jsr	wslide
	.word	right, 1*2		| jsr	r5,wslide; right; 1.*2
	bra	1b			| br	1b
wqueen:
	jsr	wslide
	.word	uleft, -9*2		| jsr	r5,wslide; uleft; -9.*2
	jsr	wslide
	.word	uright, -7*2		| jsr	r5,wslide; uright; -7.*2
	jsr	wslide
	.word	dleft, 7*2		| jsr	r5,wslide; dleft; 7.*2
	jsr	wslide
	.word	dright, 9*2		| jsr	r5,wslide; dright; 9.*2
	jsr	wslide
	.word	up, -8*2		| jsr	r5,wslide; up; -8.*2
	jsr	wslide
	.word	left, -1*2		| jsr	r5,wslide; left; -1.*2
	jsr	wslide
	.word	right, 1*2		| jsr	r5,wslide; right; 1.*2
	jsr	wslide
	.word	down, 8*2		| jsr	r5,wslide; down; 8.*2
	bra	1b			| br	1b

wking:
	jsr	wwtry
	.word	uleft, -9*2		| jsr	r5,wwtry; uleft; -9.*2
	jsr	wwtry
	.word	uright, -7*2		| jsr	r5,wwtry; uright; -7.*2
	jsr	wwtry
	.word	dleft, 7*2		| jsr	r5,wwtry; dleft; 7.*2
	jsr	wwtry
	.word	dright, 9*2		| jsr	r5,wwtry; dright; 9.*2
	jsr	wwtry
	.word	up, -8*2		| jsr	r5,wwtry; up; -8.*2
	jsr	wwtry
	.word	left, -1*2		| jsr	r5,wwtry; left; -1.*2
	jsr	wwtry
	.word	right, 1*2		| jsr	r5,wwtry; right; 1.*2
	jsr	wwtry
	.word	down, 8*2		| jsr	r5,wwtry; down; 8.*2
	bra	1b			| br	1b

wwtry:
	movl	sp@,a5			|	implicit in pdp-11 jsr
	movw	a5@+,dtmp
	andw	a4@,dtmp		| bit	(r5)+,(r4)
	bne	1f			| bne	1f
	movw	a5@,d0
	extl	d0
	movl	d0,atmp			| mov	r3,r0
	addl	a3,atmp			| add	(r5),r0
	movw	atmp@,d0		| mov	(r0),r0
	blt	1f			| blt	1f
	aslw	#1,d0			| asl	r0
	movw	_value,a2@		| mov	_value,(r2)
	extl	d0
	movl	d0,atmp
	addl	#_pval+12,atmp
	movw	atmp@,dtmp
	subw	dtmp,a2@+		| sub	_pval+12.(r0),(r2)+
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

wslide:
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
	blt	1f			| blt	1f
	bgt	2f			| bgt	2f
	movw	_value,a2@+		| mov	_value,(r2)+
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
	movw	_value,a2@		| mov	_value,(r2)
	extl	d0
	movl	d0,atmp
	addl	#_pval+12,atmp
	movw	atmp@,dtmp
	subw	dtmp,a2@+		| sub	_pval+12.(r0),(r2)+
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
