/*
 *	.seg	"data"
 *	.asciz	"@(#)samul.s 1.1 92/07/30"
 *	Copyright (c) 1986 by Sun Microsystems, Inc.
 */
 	.seg	"text"
 	.align	4

#include <machine/asm_linkage.h>
#include <machine/trap.h>

	RTENTRY(.mul)
	mov	%o1, %y			! multiplicand to Y register
	andcc	%g0, %g0, %o4		! zero the partial product
					! and clear N and V conditions
	set	xx, %o5			! jump address after 31 interations
mul_longway:
	mulscc	%o4, %o0, %o4		! first iteration of 33
	mulscc	%o4, %o0, %o4
	mulscc	%o4, %o0, %o4
	mulscc	%o4, %o0, %o4
	mulscc	%o4, %o0, %o4
	mulscc	%o4, %o0, %o4
	mulscc	%o4, %o0, %o4
	mulscc	%o4, %o0, %o4
	mulscc	%o4, %o0, %o4
	mulscc	%o4, %o0, %o4
	mulscc	%o4, %o0, %o4
	mulscc	%o4, %o0, %o4
	mulscc	%o4, %o0, %o4
	mulscc	%o4, %o0, %o4
	mulscc	%o4, %o0, %o4
	mulscc	%o4, %o0, %o4
	mulscc	%o4, %o0, %o4
	mulscc	%o4, %o0, %o4
	mulscc	%o4, %o0, %o4
	mulscc	%o4, %o0, %o4
	mulscc	%o4, %o0, %o4
	mulscc	%o4, %o0, %o4
	mulscc	%o4, %o0, %o4
	mulscc	%o4, %o0, %o4
	mulscc	%o4, %o0, %o4
	mulscc	%o4, %o0, %o4
	mulscc	%o4, %o0, %o4
	mulscc	%o4, %o0, %o4
	mulscc	%o4, %o0, %o4
	mulscc	%o4, %o0, %o4
	jmp	%o5
	mulscc	%o4, %o0, %o4		! 31st iteration

xx:
	sub	%g0, %o0, %o5		! negate the multiplicand
	mulscc	%o4, %o5, %o4		! 32nd iteration
	mulscc	%o4, %g0, %o4		! last iteration only shifts
	!
	! if you are not interested in detecting overflow,
	! remove the following branch (KEEP the rd!) and subcc
	!
	bge	1f			! skip negative overflow check
	rd	%y, %o0			! return least sig. bits of prod
	subcc	%o4, -1, %g0		! set Z if high order bits
					! are -1.
1:
	retl
	mov	%o4, %o1		! delay slot

/*
 * procedure to perform a 32 by 32 unsigned multiply.
 * pass the multiplicand into %o0, and the multiplier into %o1
 * the least significant 32 bits of the result will be returned in %o0,
 * and the most significant in %o1
 *
 * THE FOLLOWING IS NOT TRUE (yet):
 * This code indicates that overflow has occured, by leaving the Z condition
 * code clear. The following call sequence would be used if you wish to
 * deal with overflow:
 *
 *	 	call	.umul
 *		nop		( or set up last parameter here )
 *		bnz	overflow_code	(or tnz to overflow handler)
 */
	RTENTRY(.umul)
	mov	%o1, %y			! multiplicand to Y register
	andcc	%g0, %g0, %o4		! zero the partial product
					! and clear N and V conditions
					! bits of prod.
	!
	! long multiply
	!
umul_longway:
	! must detect and mask high-order bit of multiplicand,
	! since it will be (mis-)interpreted by the mulscc instruction
	! as a sign bit.
	addcc	%o0, %g0, %o2		! i.e. move, set cc
	bge	1f
	sethi	%hi(0x80000000), %o3
	andncc	%o0, %o3, %o0		! clear the bit, clear cc
1:
	set	uxx, %o5		! jump address after 31 interations
	b,a	mul_longway
uxx:
	mulscc	%o4, %o0, %o4		! 32nd iteration
	! hign-order correction
	! first, collect the outgoing carry
unsigned_correction:
	bl	1f
	mov	%o3, %o5		! (delay)
	mov	0, %o5			! %o5 has carry-out in high-order bit
1:	! now decide if we need to correct
	! consult saved copy of multiplicand, in %o2, to see if sign is set
	tst	%o2
	bpos	2f
	tst	%o5	! (delay) set SIGNED cc on carry-out we've collected
	! apply correction factor
	addcc	%o4, %o1, %o4
	! or-in carry-out
	bcc	1f
	tst	%o5	! (delay) set SIGNED cc on carry-out we've collected
	orcc	%o5, %o3, %o5 ! re-set SIGNED likewise
1:
2:
	mulscc	%o4, %g0, %o4		! last iteration collects carry-out
	!
	! should test for significant bits in the high-order word here
	!
	! beq	1f			! nothing here
	rd	%y, %o0			! return least sig. bits of prod
!1:
	retl
	mov	%o4, %o1		! delay slot

