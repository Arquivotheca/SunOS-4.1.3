/*
 *	.seg	"data"
 *	.asciz	"@(#)strncmp.s 1.1 92/07/30"
 *	Copyright (c) 1989 by Sun Microsystems, Inc.
 */

#include <sun4/asm_linkage.h>

	.seg	"text"
	.align	4

/*
 * strncmp(s1, s2, n)
 * Compare strings (at most n bytes):  s1>s2: >0  s1==s2: 0  s1<s2: <0
 */
	ENTRY(strncmp)
	save	%sp, -SA(WINDOWSIZE), %sp
	cmp	%i2, 8
	bl,a	cmp_bytes		! for small counts go do bytes
	sub	%i0, %i1, %i0		! delay slot, get diff from s1 - s2
	andcc	%i0, 3, %g0		! is s1 aligned
1:	bz	iss2			! if so go check s2
	andcc	%i1, 3, %i3		! is s2 aligned 

	deccc	%i2			! --n >= 0 ?
	bneg	doneq
	nop				! delay slot	

	ldsb	[%i0], %i4		! else cmp one byte
	ldsb	[%i1], %i5
	inc	%i0
	cmp	%i4, %i5
	bne	noteqb
	inc	%i1
	tst	%i4			! terminating zero
	bnz	1b
	andcc	%i0, 3, %g0
	b,a	doneq

iss2:	
	set     0x7efefeff, %l6
	set     0x81010100, %l7
	sethi	%hi(0xff000000), %l0	! masks to test for terminating null
	sethi	%hi(0x00ff0000), %l1
	srl	%l1, 8, %l2		! generate 0x0000ff00 mask

	bz	w4cmp			! if s2 word aligned, compare words
	cmp	%i3, 2			! check if s2 half aligned
	be	w2cmp
	cmp	%i3, 1			! check if aligned to 1 or 3 bytes	
w3cmp:	ldub	[%i1], %i5
	inc	1, %i1 
	be	w1cmp	
	sll	%i5, 24, %i5
	sub	%i0, %i1, %i0
2:
	deccc	4, %i2			! n >= 4 ?
	bge	3f
	ld	[%i1], %i3		! delay slot
	dec	%i1			! reset s2
	inc	%i0			! reset s1 diff
	b	cmp_bytes		! do a byte at a time if n < 4
	inc	4, %i2
3:
	ld	[%i0 + %i1], %i4
	inc	4, %i1
	srl	%i3, 8, %l4		! merge with the other half
	or	%l4, %i5, %i5
	cmp	%i4, %i5
	be	1f

	add	%i4, %l6, %l3
	b,a	noteq
1:	xor	%l3, %i4, %l3
	and	%l3, %l7, %l3
	cmp	%l3, %l7
	be,a	2b
	sll	%i3, 24, %i5
	
	andcc	%i4, %l0, %g0		! check if first byte was zero
	b,a	noteq
1:	bnz	1f
	andcc	%i4, %l1, %g0		! check if second byte was zero
	b,a	doneq
1:	bnz	1f
	andcc 	%i4, %l2, %g0		! check if third byte was zero
	b,a	doneq
1:	bnz	1f
	andcc	%i4, 0xff, %g0		! check if last byte is zero
	b,a	doneq
1:	bnz	2b
	sll	%i3, 24, %i5
	b,a	doneq

w1cmp:	clr	%l4
	lduh	[%i1], %l4
	inc	2, %i1
	sll	%l4, 8, %l4
	or	%i5, %l4, %i5

	sub	%i0, %i1, %i0
3:
	deccc	4, %i2			! n >= 4 ?
	bge	4f
	ld	[%i1], %i3		! delay slot
	dec	3, %i1			! reset s2
	inc	3, %i0			! reset s1 diff
	b	cmp_bytes		! do a byte at a time if n < 4
	inc	4, %i2
4:
	ld	[%i0 + %i1], %i4
	inc	4, %i1
	srl	%i3, 24, %l4		! merge with the other half
	or	%l4, %i5, %i5
	cmp	%i4, %i5
	be	1f

	add	%i4, %l6, %l3
	b,a	noteq
1:	xor	%l3, %i4, %l3
	and	%l3, %l7, %l3
	cmp	%l3, %l7
	be,a	3b
	sll	%i3, 8, %i5

	andcc	%i4, %l0, %g0		! check if first byte was zero
	b,a	noteq
1:	bnz	1f
	andcc	%i4, %l1, %g0		! check if second byte was zero
	b,a	doneq
1:	bnz	1f
	andcc 	%i4, %l2, %g0		! check if third byte was zero
	b,a	doneq
1:	bnz	1f
	andcc	%i4, 0xff, %g0		! check if last byte is zero
	b,a	doneq
1:	bnz	3b
	sll	%i3, 8, %i5
	b,a	doneq
	
w2cmp:	
	lduh	[%i1], %i5		! read a halfword to align s2
	inc	2, %i1	
	sll	%i5, 16, %i5

	sub	%i0, %i1, %i0
4:
	deccc	4, %i2			! n >= 4 ?
	bge	5f
	ld	[%i1], %i3		! delay slot
	dec	2, %i1			! reset s2
	inc	2, %i0			! reset s1 diff
	b	cmp_bytes		! do a byte at a time if n < 4
	inc	4, %i2			! delay slot
5:
	ld	[%i1 + %i0], %i4	! read a word from s2
	inc	4, %i1
	srl	%i3, 16, %l4		! merge with the other half
	or	%l4, %i5, %i5
	cmp	%i4, %i5
	be	1f

	add	%i4, %l6, %l3
	b,a	noteq
1:	xor	%l3, %i4, %l3		! are any bytes 0?
	and	%l3, %l7, %l3
	cmp	%l3, %l7
	be,a	4b
	sll	%i3, 16, %i5

	andcc	%i4, %l0, %g0		! check if first byte was zero
	b,a	noteq
1:	bnz	1f
	andcc	%i4, %l1, %g0		! check if second byte was zero
	b,a	doneq
1:	bnz	1f
	andcc 	%i4, %l2, %g0		! check if third byte was zero
	b,a	doneq
1:	bnz	1f
	andcc	%i4, 0xff, %g0		! check if last byte is zero
	b,a	doneq
1:	bnz	4b
	sll	%i3, 16, %i5
	b,a	doneq

w4cmp:	sub	%i0, %i1, %i0
	ld	[%i1], %i5		! read a word from s1
5:
	deccc	4, %i2			! n >= 4 ?
	bneg,a	cmp_bytes		! do a byte at a time if n < 4
	inc	4, %i2

	ld	[%i1 + %i0], %i4	! read a word from s2
	cmp	%i4, %i5
	inc	4, %i1
	be	1f

	add	%i4, %l6, %l3
	b,a	noteq
1:	xor	%l3, %i4, %l3
	and	%l3, %l7, %l3
	cmp	%l3, %l7
	be,a	5b
	ld	[%i1], %i5

	andcc	%i4, %l0, %g0		! check if first byte was zero
	b,a	noteq
	! if word is equal and any bytes are zero, strings are equal
1:	bnz	1f
	andcc	%i4, %l1, %g0		! check if second byte was zero
	b,a	doneq
1:	bnz	1f
	andcc 	%i4, %l2, %g0		! check if third byte was zero
	b,a	doneq
1:	bnz	1f
	andcc	%i4, 0xff, %g0		! check if last byte is zero
	b,a	doneq
1:	bnz,a	5b
	ld	[%i1], %i5
doneq:	ret
	restore	%g0, %g0, %o0		! equal return zero

noteq:	sra	%i4, 24, %l4
	sra	%i5, 24, %l5
	subcc	%l4, %l5, %i0
	bne	6f
	andcc	%l4, 0xff, %g0
	bz	doneq
	sll	%i4, 8, %l4
	sll	%i5, 8, %l5
	sra	%l4, 24, %l4
	sra	%l5, 24, %l5
	subcc	%l4, %l5, %i0
	bne	6f
	andcc	%l4, 0xff, %g0
	bz	doneq
	sll	%i4, 16, %l4
	sll	%i5, 16, %l5
	sra	%l4, 24, %l4
	sra	%l5, 24, %l5
	subcc	%l4, %l5, %i0
	bne	6f
	andcc	%l4, 0xff, %g0
	bz	doneq
	nop
noteqb:
	sll	%i4, 24, %l4
	sll	%i5, 24, %l5
	sra	%l4, 24, %l4
	sra	%l5, 24, %l5
	subcc	%l4, %l5, %i0
6:	ret
	restore	%i0, %g0, %o0

	! Do a byte by byte comparison, disregarding alignments
cmp_bytes:
	deccc	%i2			! --n >= 0 ?
1:
	bneg	doneq
	nop				! delay slot 
	ldsb	[%i1 + %i0], %i4	! read a word from s1
	ldsb	[%i1], %i5		! read a word from s2

	inc	%i1
	cmp	%i4, %i5
	bne	noteqb
	tst	%i4			! terminating zero
	bnz	1b
	deccc	%i2			! --n >= 0
	b,a	doneq	
