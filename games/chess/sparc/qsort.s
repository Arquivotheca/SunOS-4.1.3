	.seg	"data"
	.asciz	"@(#)qsort.s 1.1 92/07/30 Copyr 1987 Sun Micro"
	.seg	"text"

!	Copyright (c) 1987 by Sun Microsystems, Inc.

#include <sun4/asm_linkage.h>

!	qsort(*int,*int)
!
! qsort interface to c
!	qsort(from, to)

! r0 -> i0 (this is used as a temporary)
! r1 -> i1
! r2 -> i2
! r3 -> i3

.global	_qsort
_qsort:
	mov	%o1, %o2		! mov	2(sp),r1
	mov	%o0, %o1		! mov	4(sp),r2
					! jsr	pc,qsort
					! rts	pc

qsort:
	save	%sp, -SA(MINFRAME), %sp
qsort1:					! mov	r2,r3
	sub	%i2, %i1, %i3		! sub	r1,r3
	cmp	%i3, 4			! cmp	r3,$4
	ble	done			! ble	done
	nop

	sra	%i3, 1, %i3		! asr	r3
	bclr	3, %i3			! bic	$3,r3
	add	%i3, %i1, %i3		! add	r1,r3
	mov	%i1, %l1		! mov	r1,-(sp)
	mov	%i2, %l2		! mov	r2,-(sp)
loop:
	cmp	%i1, %i3		! cmp	r1,r3
	bgeu	loop1			! bhis	loop1
	nop

	ldsh	[%i1], %g1
	ldsh	[%i3], %g2
	cmp	%g1, %g2		! cmp	(r1),(r3)
	ble,a	loop			! bgt	loop1
	add	%i1, 4, %i1		! add	$4,r1
					! br	loop
loop1:
	cmp	%i2, %i3		! cmp	r2,r3
	bleu	1f			! blos	1f
	nop

	sub	%i2, 4, %i2		! sub	$4,r2
	mov	%i2, %i0		! mov	r2,r0
	ldsh	[%i0], %g1
	ldsh	[%i3], %g2
	cmp	%g1, %g2		! cmp	(r0),(r3)
	bge	loop1			! bge	loop1
	nop

	ldsh	[%i1], %i0		! mov	(r1),r0
	ldsh	[%i2], %g1
	sth	%g1, [%i1]		! mov	(r2),(r1)+
	sth	%i0, [%i2]		! mov	r0,(r2)+
	ldsh	[%i1 + 2], %i0		! mov	(r1),r0
	ldsh	[%i2 + 2], %g1
	sth	%g1, [%i1 + 2]		! mov	(r2),(r1)
	sth	%i0, [%i2 + 2]		! mov	r0,(r2)
					! cmp	-(r1),-(r2)
	cmp	%i1, %i3		! cmp	r1,r3
	bne	loop			! bne	loop
	nop

	b	loop			! mov	r2,r3
	mov	%i2, %i3		! br	loop
1:
	cmp	%i1, %i3		! cmp	r1,r3
	be	1f			! beq	1f
	nop

	ldsh	[%i1], %i0		! mov	(r1),r0
	ldsh	[%i2], %g1
	sth	%g1, [%i1]		! mov	(r2),(r1)+
	sth	%i0, [%i2]		! mov	r0,(r2)+
	ldsh	[%i1 + 2], %i0		! mov	(r1),r0
	ldsh	[%i2 + 2], %g1
	sth	%g1, [%i1 + 2]		! mov	(r2),(r1)
	sth	%i0, [%i2 + 2]		! mov	r0,(r2)
	b	loop1			! cmp	-(r1),-(r2)
	mov	%i1, %i3		! mov	r1,r3
					! br	loop1
1:
	mov	%l2, %o2		! mov	(sp)+,r2
	mov	%i3, %l2		! mov	r3,-(sp)
	call	qsort			! mov	r3,r1
	add	%i3, 4, %o1		! add	$4,r1
					! jsr	pc,qsort

	mov	%l2, %i2		! mov	(sp)+,r2
	b	qsort1
	mov	%l1, %i1		! mov	(sp)+,r1
					! br	qsort
done:
	ret
	restore				! rts	pc

!
!	temporarily disabled
!
!	rti = 2				! rti = 2

!	itinit()

!.globl	_itinit
!.globl	_intrp, _term
!signal = 48			! signal = 48.
!
!_itinit:
!	sys	signal; 1; 1
!	bit	$1,r0
!	bne	1f
!	sys	signal; 1; _onhup
!1:
!	sys	signal; 2; 1
!	bit	$1,r0
!	bne	1f
!	sys	signal; 2; onint
!1:
!	sys	signal; 3; 1
!	rts	pc
!
!.globl	_onhup
!_onhup:
!	sys	signal; 1; 1
!	sys	signal; 2; 1
!	sys	signal; 3; 1
!	jmp	_term
!
!onint:
!	mov	r0,-(sp)
!	sys	signal; 2; onint
!	clr	r0
!	sys	seek; 0; 2
!	mov	2(sp),r0
!	cmp	-6(r0),$sys+read
!	bne	1f
!	sub	$6,2(sp)
!1:
!	inc	_intrp
!	mov	(sp)+,r0
!	rti
!
!
!	replaced by clock.c
!
!/	t = clock()
!
!.globl	_clock
!_clock:
!	sys	time
!	mov	r0,-(sp)
!	mov	r1,-(sp)
!	sub	t+2,r1
!	sbc	r0
!	sub	t,r0
!	mov	r1,r0
!	mov	(sp)+,t+2
!	mov	(sp)+,t
!	rts	pc
!
!.bss
!t:
!	.skip 4				! :	.=.+4
