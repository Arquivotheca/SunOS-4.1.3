        .data
        .asciz  "@(#)qsort.s 1.1 92/07/30 Copyr 1984 Sun Micro"
        .even
        .text
				
|       Copyright (c) 1984 by Sun Microsystems, Inc.

#	qsort(*int,*int)
#
# qsort interfact to c
#	qsort(from, to)

# r0 -> a0 or d0 (this is used as a temporary)
# r1 -> a1
# r2 -> a2
# r3 -> a3

atmp = a4
dtmp = d1

.globl	_qsort
_qsort:
	movl	sp@(4),a1		| mov	2(sp),r1
	movl	sp@(8),a2		| mov	4(sp),r2
	jsr	qsort			| jsr	pc,qsort
	rts				| rts	pc


qsort:
	movl	a2,a3			| mov	r2,r3
	subl	a1,a3			| sub	r1,r3
	cmpl	#4,a3			| cmp	r3,$4
	ble	done			| ble	done
	movl	a3,dtmp
	asrl	#1,dtmp			| asr	r3
	andl	#~3,dtmp
	movl	dtmp,a3			| bic	$3,r3

	addl	a1,a3			| add	r1,r3
	movl	a1,sp@-			| mov	r1,-(sp)
	movl	a2,sp@-			| mov	r2,-(sp)

loop:
	cmpl	a3,a1			| cmp	r1,r3
	bcc	loop1			| bhis	loop1
	movw	a3@,dtmp
	cmpw	a1@,dtmp		| cmp	(r1),(r3)
	blt	loop1			| bgt	loop1
	addl	#4,a1			| add	$4,r1
	bra	loop			| br	loop

loop1:
	cmpl	a3,a2			| cmp	r2,r3
	bls	1f			| blos	1f
	subl	#4,a2			| sub	$4,r2
	movl	a2,a0			| mov	r2,r0
	movw	a3@,dtmp
	cmpw	a0@,dtmp		| cmp	(r0),(r3)
	ble	loop1			| bge	loop1

	movw	a1@,d0			| mov	(r1),r0
	movw	a2@,a1@+		| mov	(r2),(r1)+
	movw	d0,a2@+			| mov	r0,(r2)+
	movw	a1@,d0			| mov	(r1),r0
	movw	a2@,a1@			| mov	(r2),(r1)
	movw	d0,a2@			| mov	r0,(r2)
	movw	a2@-,dtmp
	cmpw	a1@-,dtmp		| cmp	-(r1),-(r2)
	cmpl	a1,a3			| cmp	r1,r3
	bne	loop			| bne	loop
	movl	a2,a3			| mov	r2,r3
	bra	loop			| br	loop

1:
	cmpl	a1,a3			| cmp	r1,r3
	beq	1f			| beq	1f
	movw	a1@,d0			| mov	(r1),r0
	movw	a2@,a1@+		| mov	(r2),(r1)+
	movw	d0,a2@+			| mov	r0,(r2)+
	movw	a1@,d0			| mov	(r1),r0
	movw	a2@,a1@			| mov	(r2),(r1)
	movw	d0,a2@			| mov	r0,(r2)
	movw	a2@-,dtmp
	cmpw	a1@-,dtmp		| cmp	-(r1),-(r2)
	movl	a1,a3			| mov	r1,r3
	bra	loop1			| br	loop1

1:
	movl	sp@+,a2			| mov	(sp)+,r2
	movl	a3,sp@-			| mov	r3,-(sp)
	movl	a3,a1			| mov	r3,r1
	addl	#4,a1			| add	$4,r1
	jsr	qsort			| jsr	pc,qsort
	movl	sp@+,a2			| mov	(sp)+,r2
	movl	sp@+,a1			| mov	(sp)+,r1
	bra	qsort			| br	qsort

done:
	rts				| rts	pc

#
#	temporarily disabled
#
#	rti = 2				| rti = 2

#	itinit()

#.globl	_itinit
#.globl	_intrp, _term
#signal = 48			| signal = 48.
#
#_itinit:
#	sys	signal; 1; 1
#	bit	$1,r0
#	bne	1f
#	sys	signal; 1; _onhup
#1:
#	sys	signal; 2; 1
#	bit	$1,r0
#	bne	1f
#	sys	signal; 2; onint
#1:
#	sys	signal; 3; 1
#	rts	pc
#
#.globl	_onhup
#_onhup:
#	sys	signal; 1; 1
#	sys	signal; 2; 1
#	sys	signal; 3; 1
#	jmp	_term
#
#onint:
#	mov	r0,-(sp)
#	sys	signal; 2; onint
#	clr	r0
#	sys	seek; 0; 2
#	mov	2(sp),r0
#	cmp	-6(r0),$sys+read
#	bne	1f
#	sub	$6,2(sp)
#1:
#	inc	_intrp
#	mov	(sp)+,r0
#	rti
#
#
#	replaced by clock.c
#
#/	t = clock()
#
#.globl	_clock
#_clock:
#	sys	time
#	mov	r0,-(sp)
#	mov	r1,-(sp)
#	sub	t+2,r1
#	sbc	r0
#	sub	t,r0
#	mov	r1,r0
#	mov	(sp)+,t+2
#	mov	(sp)+,t
#	rts	pc
#
#.bss
#t:
#	.skip 4				| :	.=.+4
