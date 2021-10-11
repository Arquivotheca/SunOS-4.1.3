/*
 *	.seg	"data"
 *	.asciz	"@(#)strlen.s 1.1 92/07/30"
 *	Copyright (c) 1987 by Sun Microsystems, Inc.
 */
#include <sun4/asm_linkage.h>

	.seg	"text"
	.align	4
/*
 * Returns the number of
 * non-NULL bytes in string argument.
 */
	ENTRY(strlen)
	mov	%o0, %o1
	andcc	%o1, 3, %o3		! is src word aligned
	bz	nowalgnd
	clr	%o0			! length of non-zero bytes
	cmp	%o3, 2			! is src half-word aligned
	be	s2algn
	cmp	%o3, 3			! src is byte aligned
	ldub	[%o1], %o3		! move 1 or 3 bytes to align it
	inc	1, %o1			! in either case, safe to do a byte
	be	s3algn
	tst	%o3
s1algn:	bnz,a	s2algn			! now go align dest
	inc	1, %o0
	b,a	done

s2algn:	lduh	[%o1], %o3		! know src is half-byte aligned
	inc	2, %o1
	srl	%o3, 8, %o4
	tst	%o4			! is the first byte zero
	bnz,a	1f
	inc	%o0
	b,a	done
1:	andcc	%o3, 0xff, %o3		! is the second byte zero
	bnz,a	nowalgnd
	inc	%o0
	b,a	done
s3algn:	bnz,a	nowalgnd
	inc	1, %o0
	b,a	done

nowalgnd:
	! use trick to check if any read bytes of a word are zero
	! the following two constants will generate "byte carries"
	! and check if any bit in a byte is set, if all characters
	! are 7bits (unsigned) this allways works, otherwise
	! there is a specil case that rarely happens, see below

	set	0x7efefeff, %o3
	set	0x81010100, %o4

3:	ld	[%o1], %o2		! main loop
	inc	4, %o1
	add	%o2, %o3, %o5		! generate byte-carries
	xor	%o5, %o2, %o5		! see if orignal bits set
	and	%o5, %o4, %o5
	cmp	%o5, %o4		! if ==,  no zero bytes
	be,a	3b
	inc	4, %o0

	! check for the zero byte and increment the count appropriately
	! some information (the carry bit) is lost if bit 31
	! was set (very rare), if this is the rare condition,
	! return to the main loop again

	sethi	%hi(0xff000000), %o5	! mask used to test for terminator
	andcc	%o2, %o5, %g0		! check if first byte was zero
	bnz	1f
	srl	%o5, 8, %o5
done:	retl
	nop
1:	andcc	%o2, %o5, %g0		! check if second byte was zero
	bnz	1f
	srl	%o5, 8, %o5
done1:	retl
	inc	%o0
1:	andcc 	%o2, %o5, %g0		! check if third byte was zero
	bnz	1f
	andcc	%o2, 0xff, %g0		! check if last byte is zero
done2:	retl
	inc	2, %o0
1:	bnz,a	3b
	inc	4, %o0			! count of bytes
done3:	retl
	inc	3, %o0
