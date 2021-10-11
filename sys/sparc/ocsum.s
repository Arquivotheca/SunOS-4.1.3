!	@(#)ocsum.s 1.1 92/07/30 SMI
!	Copyright (c) 1989 by Sun Microsystems, Inc.

	.seg	"text"
	.align	4


#include <machine/asm_linkage.h>

/*
 * ocsum(address, halfword_count)
 * Do a 16 bit one's complement sum of a given number of (16-bit)
 * halfwords. The halfword pointer must not be odd.
 *	%o0 address; %o1 count; %o3 accumulator; %o4 temp
 * 	%g2 and %g3 used in main loop
 */
	ENTRY(ocsum)
	clr	%o3		! clear accumulator
	cmp	%o1, 31		! less than 32 bytes?
	bl,a	dohw		!   just do halfwords
	tst	%o1		! delay slot, test count

	btst	31, %o0		! (delay slot)
	bz	2f		! if 32 byte aligned, skip
	nop

	!
	! Do first halfwords until 32-byte aligned
	!
1:
	lduh	[%o0], %g2	! read data
	add	%o0, 2, %o0	! increment address
	add	%o3, %g2, %o3	! add to accumulator, don't need carry yet
	btst	31, %o0		! 32 byte aligned?
	bnz	1b
	sub	%o1, 1, %o1	! decrement count
	!
	! loop to add in 32 byte chunks
	! The loads and adds are staggered to help avoid load/use
	! interlocks on highly pipelined implementations, and double
	! loads are used for 64-bit wide memory systems.
	!
2:
	sub	%o1, 16, %o1	! decrement count to aid testing
4:
	ldd	[%o0], %g2	! read data
	ldd	[%o0+8], %o4	! read more data
	addcc	%o3, %g2, %o3	! add to accumulator
	addxcc	%o3, %g3, %o3	! add to accumulator with carry
	ldd	[%o0+16], %g2	! read more data
	addxcc	%o3, %o4, %o3	! add to accumulator with carry
	addxcc	%o3, %o5, %o3	! add to accumulator with carry
	ldd	[%o0+24], %o4	! read more data
	addxcc	%o3, %g2, %o3	! add to accumulator with carry
	addxcc	%o3, %g3, %o3	! add to accumulator with carry
	addxcc	%o3, %o4, %o3	! add to accumulator
	addxcc	%o3, %o5, %o3	! add to accumulator with carry
	addxcc	%o3, 0, %o3	! if final carry, add it in
	subcc	%o1, 16, %o1	! decrement count (in halfwords)
	bge	4b
	add	%o0, 32, %o0	! delay slot, increment address
	
	add	%o1, 16, %o1	! add back in
	!
	! Do any remaining halfwords
	!
	b	dohw
	tst	%o1		! delay slot, for more to do

3:
	add	%o0, 2, %o0	! increment address
	addcc	%o3, %g2, %o3	! add to accumulator
	addxcc	%o3, 0, %o3	! if carry, add it in
	subcc	%o1, 1, %o1	! decrement count
dohw:
	bg,a	3b		! more to do?
	lduh	[%o0], %g2	! read data

	!
	! at this point the 32-bit accumulator
	! has the result that needs to be returned in 16-bits
	!
	sll	%o3, 16, %o4	! put low halfword in high halfword %o4
	addcc	%o4, %o3, %o3	! add the 2 halfwords in high %o3, set carry
	srl	%o3, 16, %o3	! shift to low halfword
	retl			! return
	addxcc	%o3, 0, %o0	! add in carry if any. result in %o0

