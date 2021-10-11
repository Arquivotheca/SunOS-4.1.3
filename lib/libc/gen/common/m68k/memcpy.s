	.data
/*	.asciz	"@(#)memcpy.s 1.1 92/07/30 SMI"	*/
	.text

#include "DEFS.h"

| Copy a block of storage - must check for overlap (len < abs(from-to))
| Usage: memcpy(to, from, count)

ENTRY(memcpy)
	movl	PARAM,a1	| to
	movl	PARAM2,a0	| from
	movl	PARAM3,d0	| get count
	jle	ret		| return if not positive
| Check for overlap
	movl	a1,d1		| get abs(from-to)
	subl	a0,d1
	bge	1$
	negl	d1
1$:	cmpl	d0,d1		| check for overlap
	blt	ovbcopy
| If from address odd, move one byte to 
| try to make things even
ok:	movw	a0,d1		| from
	btst	#0,d1		| test for odd bit in from
	jeq	1$		| even, skip
	movb	a0@+,a1@+	| move a byte
	subql	#1,d0		| adjust count
| If to address is odd now, we have to do byte moves
1$:	movl	a1,d1		| low bit one if mutual oddness
	btst	#0,d1		| test low bit
	jne	bytes		| do it slow and easy
| The addresses are now both even 
| Now move longs
	movl	d0,d1		| save count
	lsrl	#2,d0		| get # longs
	jra	3$		| enter long move loop
| The following loop runs in loop mode on 68010
2$:	movl	a0@+,a1@+	| move a long
3$:	dbra	d0,2$		| decr, br if >= 0
	clrw	d0
	subql	#1,d0
	bccs	2$
| Now up to 3 bytes remain to be moved
	movl	d1,d0		| restore count
	andl	#3,d0		| mod sizeof long
	jra	bytes		| go do bytes

| Here if we have to move byte-by-byte because
| the pointers didn't line up.  68010 loop mode is used.
bloop:	movb	a0@+,a1@+	| loop mode byte moves
bytes:	dbra	d0,bloop
	clrw	d0
	subql	#1,d0
	bccs	bloop
ret:	movl	PARAM,d0	| return "to" value
	RET

| Block copy with possibly overlapped operands
ovbcopy:
	cmpl	a0,a1		| check direction of copy
	jgt	bwd		| do it backwards
| Here if from > to - copy bytes forward
	jra	2$
| Loop mode byte copy
1$:	movb	a0@+,a1@+
2$:	dbra	d0,1$
	clrw	d0
	subql	#1,d0
	bccs	1$
	movl	PARAM,d0	| return "to" value
	RET
| Here if from < to - copy bytes backwards
bwd:	addl	d0,a0		| get end of from area
	addl	d0,a1		| get end of to area
	jra	2$		| enter loop
| Loop mode byte copy
1$:	movb	a0@-,a1@-
2$:	dbra	d0,1$
	clrw	d0
	subql	#1,d0
	bccs	1$
	movl	PARAM,d0	| return "to" value
	RET
