/*
 *	.seg	"data"
 *	.asciz	"@(#)bcopy.s 1.1 92/07/30"
 *	Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <sun4/asm_linkage.h>

	.seg	"text"
	.align	4

/*
 * bcopy(s1, s2, len)
 * Copy s1 to s2, always copy n bytes.
 * For overlapped copies it does the right thing.
 */
	ENTRY(bcopy)
	cmp	%o0, %o1	! if from address is >= to use forward copy
	bg	2f		! else use backward if ...
	cmp	%o2, 9		! delay slot, for small counts copy bytes

	subcc	%o0, %o1, %o4	! get difference of two addresses
	bneg,a	1f
	neg	%o4		! if < 0, make it positive
1:	cmp	%o2, %o4	! compare size and abs(difference of addresses)
	bg	ovbc		! if size is bigger, have do overlapped copy

	cmp	%o2, 9		! delay slot, for small counts copy bytes
	!
	! normal, copy forwards
	!
2:	ble	dbytecp
	andcc	%o0, 3, %o5		! is src word aligned
	bz	aldst
	cmp	%o5, 2			! is src half-word aligned
	be	s2algn
	cmp	%o5, 3			! src is byte aligned
s1algn:	ldub	[%o0], %o3		! move 1 or 3 bytes to align it
	inc	1, %o0
	stb	%o3, [%o1]		! move a byte to align src
	inc	1, %o1
	bne	s2algn
	dec	%o2
	b	ald			! now go align dest
	andcc	%o1, 3, %o5

s2algn:	lduh	[%o0], %o3		! know src is 2 byte alinged
	inc	2, %o0
	srl	%o3, 8, %o4
	stb	%o4, [%o1]		! have to do bytes,
	stb	%o3, [%o1 + 1]		! don't know dst alingment
	inc	2, %o1
	dec	2, %o2

aldst:	andcc	%o1, 3, %o5		! align the destination address
ald:	bz	w4cp
	cmp	%o5, 2
	bz	w2cp
	cmp	%o5, 3
w3cp:	ld	[%o0], %o4
	inc	4, %o0
	srl	%o4, 24, %o5
	stb	%o5, [%o1]
	bne	w1cp
	inc	%o1
	dec	1, %o2
	andn	%o2, 3, %o3		! o3 is aligned word count
	sub	%o0, %o1, %o0		! o0 gets the difference

1:	sll	%o4, 8, %g1		! save residual bytes
	ld	[%o0+%o1], %o4
	deccc	4, %o3
	srl	%o4, 24, %o5		! merge with residual
	or	%o5, %g1, %g1
	st	%g1, [%o1]
	bnz	1b
	inc	4, %o1
	sub	%o0, 3, %o0		! used one byte of last word read
	b	7f
	and	%o2, 3, %o2

w1cp:	srl	%o4, 8, %o5
	sth	%o5, [%o1]
	inc	2, %o1
	dec	3, %o2
	andn	%o2, 3, %o3
	sub	%o0, %o1, %o0		! o0 gets the difference

2:	sll	%o4, 24, %g1		! save residual bytes
	ld	[%o0+%o1], %o4
	deccc	4, %o3
	srl	%o4, 8, %o5		! merge with residual
	or	%o5, %g1, %g1
	st	%g1, [%o1]
	bnz	2b
	inc	4, %o1
	sub	%o0, 1, %o0		! used three bytes of last word read
	b	7f
	and	%o2, 3, %o2

w2cp:	ld	[%o0], %o4
	inc	4, %o0
	srl	%o4, 16, %o5
	sth	%o5, [%o1]
	inc	2, %o1
	dec	2, %o2
	andn	%o2, 3, %o3		! o3 is aligned word count
	sub	%o0, %o1, %o0		! o0 gets the difference
	
3:	sll	%o4, 16, %g1		! save residual bytes
	ld	[%o0+%o1], %o4
	deccc	4, %o3
	srl	%o4, 16, %o5		! merge with residual
	or	%o5, %g1, %g1
	st	%g1, [%o1]
	bnz	3b
	inc	4, %o1
	sub	%o0, 2, %o0		! used two bytes of last word read
	b	7f
	and	%o2, 3, %o2

w4cp:	andn	%o2, 3, %o3		! o3 is aligned word count
	sub	%o0, %o1, %o0		! o0 gets the difference

1:	ld	[%o0+%o1], %o4		! read from address
	deccc	4, %o3			! decrement count
	st	%o4, [%o1]		! write at destination address
	bg	1b
	inc	4, %o1			! increment to address
	b	7f
	and	%o2, 3, %o2		! number of leftover bytes, if any

	!
	! differenced byte copy, works with any alignment
	!
dbytecp:
	b	7f
	sub	%o0, %o1, %o0		! o0 gets the difference

4:	stb	%o4, [%o1]		! write to address
	inc	%o1			! inc to address
7:	deccc	%o2			! decrement count
	bge,a	4b			! loop till done
	ldub	[%o0+%o1], %o4		! read from address
	retl
	mov	0, %o0			! return zero

	!
	! an overlapped copy that must be done "backwards"
	!
ovbc:	add	%o0, %o2, %o0		! get to end of source space
	add	%o1, %o2, %o1		! get to end of destination space
	sub	%o0, %o1, %o0		! o0 gets the difference

5:	dec	%o1			! decrement to address
	ldub	[%o0+%o1], %o3		! read a byte
	deccc	%o2			! decrement count
	bg	5b			! loop until done
	stb	%o3, [%o1]		! write byte
	retl
	mov	0, %o0			! return zero
