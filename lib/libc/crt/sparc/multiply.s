/*
 *	.seg	"data"
 *	.asciz	"@(#)multiply.s 1.1 92/07/30 Copyr 1987 Sun Micro"
 *	.align	4
 */
	.seg	"text"

!	Copyright (c) 1987 by Sun Microsystems, Inc.


#include <machine/asm_linkage.h>

/*
 * procedure to perform a 32 by 32 signed integer multiply.
 * pass the multiplier into %o0, and the multiplicand into %o1
 * the least significant 32 bits of the result will be returned in %o0,
 * and the most significant in %o1
 *
 * Most signed integer multiplies involve small positive numbers, so it is
 * worthwhile to optimize for short multiplies at the expense of long 
 * multiplies.  This code checks the size of the multiplier, and has
 * special cases for the following:
 *
 *	4 or fewer bit multipliers:	18 or 19 instruction cycles
 *	8 or fewer bit multipliers:	25 or 26 instruction cycles
 *	12 or fewer bit multipliers:	33 or 34 instruction cycles
 *	16 or fewer bit multipliers:	41 or 42 instruction cycles
 *
 * Long multipliers require 60 to 64 instruction cycles:
 *
 *	+x * +y		60 instruction cycles
 *	+x * -y		64 instruction cycles
 *	-x * +y		62 instruction cycles
 *	-x * -y		62 instruction cycles
 *
 * This code indicates that overflow has occured, by leaving the Z condition
 * code clear. The following call sequence would be used if you wish to
 * deal with overflow:
 *
 *	 	call	.mul
 *		nop		( or set up last parameter here )
 *		bnz	overflow_code	(or tnz to overflow handler)
 */
	RTENTRY(.mul)
	wr	%o0, %y			! multiplier to Y register

	andncc	%o0, 0xf, %o4		! mask out lower 4 bits; if branch
					! taken, %o4, N and V have been cleared 

	be	mul_4bit		! 4-bit multiplier
	sethi	%hi(0xffff0000), %o5	! mask for 16-bit case; have to
					! wait 3 instructions after wd
					! before %y has stabilized anyway

	andncc	%o0, 0xff, %o4
	be,a	mul_8bit		! 8-bit multiplier
	mulscc	%o4, %o1, %o4		! first iteration of 9

	andncc	%o0, 0xfff, %o4
	be,a	mul_12bit		! 12-bit multiplier
	mulscc	%o4, %o1, %o4		! first iteration of 13

	andcc	%o0, %o5, %o4
	be,a	mul_16bit		! 16-bit multiplier
	mulscc	%o4, %o1, %o4		! first iteration of 17

	andcc	%g0, %g0, %o4		! zero the partial product
					! and clear N and V conditions
	!
	! long multiply
	!
	mulscc	%o4, %o1, %o4		! first iteration of 33
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4		! 32nd iteration
	mulscc	%o4, %g0, %o4		! last iteration only shifts
	!
	! For unsigned multiplies, a pure shifty-add approach yields the
	! correct result.  Signed multiplies introduce complications.
	!
	! With 32-bit twos-complement numbers, -x can be represented as
	!
	!	((2 - (x/(2**32)) mod 2) * 2**32.
	!
	! To simplify the equations, the radix point can be moved to just
	! to the left of the sign bit.  So:
	!
	! 	 x *  y	= (xy) mod 2
	!	-x *  y	= (2 - x) mod 2 * y = (2y - xy) mod 2
	!	 x * -y	= x * (2 - y) mod 2 = (2x - xy) mod 2
	!	-x * -y = (2 - x) * (2 - y) = (4 - 2x - 2y + xy) mod 2
	!
	! Because of the way the shift into the partial product is calculated
	! (N xor V), the extra term is automagically removed for negative
	! multiplicands, so no adjustment is necessary.
	!
	! if the multiplier is negative, the result is:
	! 
	!	-xy + x * (2**32)
	!
	! we fix that here
	!
	tst	%o0
	rd	%y, %o0
	bge	1f
	tst	%o0			! for overflow check

	sub	%o4, %o1, %o4		! subtract (2**32) * %o1; bits 63-32
					! of the product are in %o4
	!
	! The multiply hasn't overflowed if:
	!	low-order bits are positive and high-order bits are 0
	!	low-order bits are negative and high-order bits are -1
	!
	! if you are not interested in detecting overflow,
	! replace the following code with:
	!
	!	1:	retl	
	!		mov	%o4, %o1
	!
1:
	bge	2f			! if low-order bits were positive.
	mov	%o4, %o1		! return most sig. bits of prod

	retl				! leaf routine return
	subcc	%o1, -1, %g0		! set Z if high order bits are -1 (for
					! negative product)
2:
	retl				! leaf routine return
	addcc	%o1, %g0, %g0		! set Z if high order bits are 0 (for
					! positive product)
	!
	! 4-bit multiply
	!
mul_4bit:
	mulscc	%o4, %o1, %o4		! first iteration of 5
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4		! 4th iteration
	mulscc	%o4, %g0, %o4		! last iteration only shifts

	rd	%y, %o5
	sll	%o4, 4, %o0		! left shift middle bits by 4 bits
	srl	%o5, 28, %o5		! right shift low bits by 28 bits
	!
	! The multiply hasn't overflowed if:
	!	low-order bits are positive and high-order bits are 0
	!	low-order bits are negative and high-order bits are -1
	!
	! if you are not interested in detecting overflow,
	! replace the following code with:
	!
	!		or	%o5, %o0, %o0
	!		retl	
	!		sra	%o4, 28, %o1
	!
	orcc	%o5, %o0, %o0		! merge for true product
	bge	3f			! if low-order bits were positive.
	sra	%o4, 28, %o1		! right shift high bits by 28 bits
					! and put into  %o1

	retl				! leaf routine return
	subcc	%o1, -1, %g0		! set Z if high order bits are -1 (for
					! negative product)
3:
	retl				! leaf routine return
	addcc	%o1, %g0, %g0		! set Z if high order bits are 0
	!
	! 8-bit multiply
	!
mul_8bit:
	mulscc	%o4, %o1, %o4		! second iteration of 9
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4		! 8th iteration
	mulscc	%o4, %g0, %o4		! last iteration only shifts

	rd	%y, %o5
	sll	%o4, 8, %o0		! left shift middle bits by 8 bits
	srl	%o5, 24, %o5		! right shift low bits by 24 bits
	!
	! The multiply hasn't overflowed if:
	!	low-order bits are positive and high-order bits are 0
	!	low-order bits are negative and high-order bits are -1
	!
	! if you are not interested in detecting overflow,
	! replace the following code with:
	!
	!		or	%o5, %o0, %o0
	!		retl	
	!		sra	%o4, 24, %o1
	!
	orcc	%o5, %o0, %o0		! merge for true product
	bge	4f			! if low-order bits were positive.
	sra	%o4, 24, %o1		! right shift high bits by 24 bits
					! and put into  %o1

	retl				! leaf routine return
	subcc	%o1, -1, %g0		! set Z if high order bits are -1 (for
					! negative product)
4:
	retl				! leaf routine return
	addcc	%o1, %g0, %g0		! set Z if high order bits are 0
	!
	! 12-bit multiply
	!
mul_12bit:
	mulscc	%o4, %o1, %o4		! second iteration of 13
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4		! 12th iteration
	mulscc	%o4, %g0, %o4		! last iteration only shifts

	rd	%y, %o5
	sll	%o4, 12, %o0		! left shift middle bits by 12 bits
	srl	%o5, 20, %o5		! right shift low bits by 20 bits
	!
	! The multiply hasn't overflowed if:
	!	low-order bits are positive and high-order bits are 0
	!	low-order bits are negative and high-order bits are -1
	!
	! if you are not interested in detecting overflow,
	! replace the following code with:
	!
	!		or	%o5, %o0, %o0
	!		retl	
	!		sra	%o4, 20, %o1
	!
	orcc	%o5, %o0, %o0		! merge for true product
	bge	5f			! if low-order bits were positive.
	sra	%o4, 20, %o1		! right shift high bits by 20 bits
					! and put into  %o1

	retl				! leaf routine return
	subcc	%o1, -1, %g0		! set Z if high order bits are -1 (for
					! negative product)
5:
	retl				! leaf routine return
	addcc	%o1, %g0, %g0		! set Z if high order bits are 0
	!
	! 16-bit multiply
	!
mul_16bit:
	mulscc	%o4, %o1, %o4		! second iteration of 17
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4
	mulscc	%o4, %o1, %o4		! 16th iteration
	mulscc	%o4, %g0, %o4		! last iteration only shifts

	rd	%y, %o5
	sll	%o4, 16, %o0		! left shift middle bits by 16 bits
	srl	%o5, 16, %o5		! right shift low bits by 16 bits
	!
	! The multiply hasn't overflowed if:
	!	low-order bits are positive and high-order bits are 0
	!	low-order bits are negative and high-order bits are -1
	!
	! if you are not interested in detecting overflow,
	! replace the following code with:
	!
	!		or	%o5, %o0, %o0
	!		retl	
	!		sra	%o4, 16, %o1
	!
	orcc	%o5, %o0, %o0		! merge for true product
	bge	6f			! if low-order bits were positive.
	sra	%o4, 16, %o1		! right shift high bits by 16 bits
					! and put into  %o1

	retl				! leaf routine return
	subcc	%o1, -1, %g0		! set Z if high order bits are -1 (for
					! negative product)
6:
	retl				! leaf routine return
	addcc	%o1, %g0, %g0		! set Z if high order bits are 0
