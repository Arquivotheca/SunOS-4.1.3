/*	@(#)big_copy.s 1.1 92/07/30 SMI	*/

	.seg	"text"
	.align	4

/*
 *	Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <machine/asm_linkage.h>
#include <sys/errno.h>
#ifdef never
#include <sys/param.h>
#include <sun4/mmu.h>
#include "assym.s"

/*
 * Copy a block of storage, returning an error code if `from' or
 * `to' takes a kernel pagefault which cannot be resolved.
 * Returns EIO on pagefault error, 0 if all ok
 *
 * int
 * kcopy(from, to, count)
 *	caddr_t from, to;
 *	u_int count;
 */
	ENTRY(kcopy)
	sethi	%hi(copyerr), %o3	! copyerr is lofault value
	b	do_copy			! common code
	or	%o3, %lo(copyerr), %o3

/*
 * We got here because of a fault during kcopy.
 */
copyerr:
	st	%l6, [_u+U_LOFAULT]	! restore old u.u_lofault
	ret
	restore	%g0, EIO, %o0		! return (EIO)

/*
 * Copy a block of storage - must not overlap ( from + len <= to).
 * Registers: l6 - saved u.u_lofault
 *
 * bcopy(from, to, count)
 *	caddr_t from, to;
 *	u_int count;
 */
	ENTRY(bcopy)
	mov	0, %o3			! no lofault value
do_copy:
	save	%sp, -SA(MINFRAME), %sp	! get another window
	ld	[_u+U_LOFAULT], %l6	! save u.u_lofault
	cmp	%i2, 12			! for small counts
	bl	bytecp			!   just copy bytes
	st	%i3, [_u+U_LOFAULT]	! install new vector
	!
	! Probe to see if word access is allowed (i.e. not VME_D16)
	! This assumes that the source and destination will not
	! change to VME_D16 during the bcopy. This will get a data
	! fault with be_vmeserr set if unsuccessful. Trap will
	! then return to bcopy_vme16. This is gross but fast.
	!
	.global _bcopy_probe, _ebcopy_probe
_bcopy_probe:
	andn	%i0, 3, %l0		! align source
	ld	[%l0], %g0		! probe source
	andn	%i1, 3, %l0		! align dest
	ld	[%l0], %g0		! probe dest
_ebcopy_probe:
	!
	! use aligned transfers where possible
	!
	xor	%i0, %i1, %o4		! xor from and to address
	btst	7, %o4			! if lower three bits zero
	bz	aldoubcp		! can align on double boundary
	btst	3, %o4			! if lower two bits zero
	bz	alwordcp		! can align on word boundary
	btst	3, %i0			! delay slot, from address unaligned?
	!
	! use aligned reads and writes where possible
	! this differs from wordcp in that it copes
	! with odd alignment between source and destnation
	! using word reads and writes with the proper shifts
	! in between to align transfers to and from memory
	! i0 - src address, i1 - dest address, i2 - count
	! i3, i4 - tmps for used generating complete word
	! i5 (word to write)
	! l0 size in bits of upper part of source word (US)
	! l1 size in bits of lower part of source word (LS = 32 - US)
	! l2 size in bits of upper part of destination word (UD)
	! l3 size in bits of lower part of destination word (LD = 32 - UD)
	! l4 number of bytes leftover after aligned transfers complete
	! l5 the number 32
	!
	mov	32, %l5			! load an oft-needed constant
	bz	align_dst_only
	btst	3, %i1			! is destnation address aligned?
	clr	%i4			! clear registers used in either case
	bz	align_src_only
	clr	%l0
	!
	! both source and destination addresses are unaligned
	!
1:					! align source
	ldub	[%i0], %i3		! read a byte from source address
	add	%i0, 1, %i0		! increment source address
	or	%i4, %i3, %i4		! or in with previous bytes (if any)
	btst	3, %i0			! is source aligned?
	add	%l0, 8, %l0		! increment size of upper source (US)
	bnz,a	1b
	sll	%i4, 8, %i4		! make room for next byte

	sub	%l5, %l0, %l1		! generate shift left count (LS)
	sll	%i4, %l1, %i4		! prepare to get rest
	ld	[%i0], %i3		! read a word
	add	%i0, 4, %i0		! increment source address
	srl	%i3, %l0, %i5		! upper src bits into lower dst bits
	or	%i4, %i5, %i5		! merge
	mov	24, %l3			! align destination
1:
	srl	%i5, %l3, %i4		! prepare to write a single byte
	stb	%i4, [%i1]		! write a byte
	add	%i1, 1, %i1		! increment destination address
	sub	%i2, 1, %i2		! decrement count
	btst	3, %i1			! is destination aligned?
	bnz,a	1b
	sub	%l3, 8, %l3		! delay slot, decrement shift count (LD)
	sub	%l5, %l3, %l2		! generate shift left count (UD)
	sll	%i5, %l2, %i5		! move leftover into upper bytes
	cmp	%l2, %l0		! cmp # req'd to fill dst w old src left
	bg	more_needed		! need more to fill than we have
	nop

	sll	%i3, %l1, %i3		! clear upper used byte(s)
	srl	%i3, %l1, %i3
	! get the odd bytes between alignments
	sub	%l0, %l2, %l0		! regenerate shift count
	sub	%l5, %l0, %l1		! generate new shift left count (LS)
	and	%i2, 3, %l4		! must do remaining bytes if count%4 > 0
	andn	%i2, 3, %i2		! # of aligned bytes that can be moved
	srl	%i3, %l0, %i4
	or	%i5, %i4, %i5
	st	%i5, [%i1]		! write a word
	subcc	%i2, 4, %i2		! decrement count
	bz	unalign_out
	add	%i1, 4, %i1		! increment destination address

	b	2f
	sll	%i3, %l1, %i5		! get leftover into upper bits
more_needed:
	sll	%i3, %l0, %i3		! save remaining byte(s)
	srl	%i3, %l0, %i3
	sub	%l2, %l0, %l1		! regenerate shift count
	sub	%l5, %l1, %l0		! generate new shift left count
	sll	%i3, %l1, %i4		! move to fill empty space
	b	3f
	or	%i5, %i4, %i5		! merge to complete word
	!
	! the source address is aligned and destination is not
	!
align_dst_only:
	ld	[%i0], %i4		! read a word
	add	%i0, 4, %i0		! increment source address
	mov	24, %l0			! initial shift alignment count
1:
	srl	%i4, %l0, %i3		! prepare to write a single byte
	stb	%i3, [%i1]		! write a byte
	add	%i1, 1, %i1		! increment destination address
	sub	%i2, 1, %i2		! decrement count
	btst	3, %i1			! is destination aligned?
	bnz,a	1b
	sub	%l0, 8, %l0		! delay slot, decrement shift count
xfer:
	sub	%l5, %l0, %l1		! generate shift left count
	sll	%i4, %l1, %i5		! get leftover
3:
	and	%i2, 3, %l4		! must do remaining bytes if count%4 > 0
	andn	%i2, 3, %i2		! # of aligned bytes that can be moved
2:
	ld	[%i0], %i3		! read a source word
	add	%i0, 4, %i0		! increment source address
	srl	%i3, %l0, %i4		! upper src bits into lower dst bits
	or	%i5, %i4, %i5		! merge with upper dest bits (leftover)
	st	%i5, [%i1]		! write a destination word
	subcc	%i2, 4, %i2		! decrement count
	bz	unalign_out		! check if done
	add	%i1, 4, %i1		! increment destination address
	b	2b			! loop
	sll	%i3, %l1, %i5		! get leftover
unalign_out:
	tst	%l4			! any bytes leftover?
	bz	cpdone
	.empty				! allow next instruction in delay slot
1:
	sub	%l0, 8, %l0		! decrement shift
	srl	%i3, %l0, %i4		! upper src byte into lower dst byte
	stb	%i4, [%i1]		! write a byte
	subcc	%l4, 1, %l4		! decrement count
	bz	cpdone			! done?
	add	%i1, 1, %i1		! increment destination
	tst	%l0			! any more previously read bytes
	bnz	1b			! we have leftover bytes
	mov	%l4, %i2		! delay slot, mv cnt where bytecp wants
	b,a	bytecp			! let bcopy do the rest
	!
	! the destination address is aligned and the source is not
	!
align_src_only:
	ldub	[%i0], %i3		! read a byte from source address
	add	%i0, 1, %i0		! increment source address
	or	%i4, %i3, %i4		! or in with previous bytes (if any)
	btst	3, %i0			! is source aligned?
	add	%l0, 8, %l0		! increment shift count (US)
	bnz,a	align_src_only
	sll	%i4, 8, %i4		! make room for next byte
	b,a	xfer
	!
	! if from address unaligned for double-word moves,
	! move bytes till it is, if count is < 56 it could take
	! longer to align the thing than to do the transfer
	! in word size chunks right away
	!
aldoubcp:
	cmp	%i2, 56			! if count < 56, use wordcp, it takes
	bl,a	alwordcp		! longer to align doubles than words
	mov	3, %o0			! mask for word alignment
	call	alignit			! copy bytes until aligned
	mov	7, %o0			! mask for double alignment
	!
	! source and destination are now double-word aligned
	! see if transfer is large enough to gain by loop unrolling
	!
	cmp	%i2, 512		! if less than 512 bytes
	bge,a	blkcopy			! just copy double-words (overwrite i3)
	mov	0x100, %i3		! blk copy chunk size for unrolled loop
	!
	! i3 has aligned count returned by alignit
	!
	and	%i2, 7, %i2		! unaligned leftover count
5:
	ldd	[%i0], %o4		! read from address
	add	%i0, 8, %i0		! inc from address
	std	%o4, [%i1]		! write at destination address
	subcc	%i3, 8, %i3		! dec count
	bg	5b
	add	%i1, 8, %i1		! delay slot, inc to address
wcpchk:
	cmp	%i2, 4			! see if we can copy a word
	bl	bytecp			! if 3 or less bytes use bytecp
	!
	! for leftover bytes we fall into wordcp, if needed
	!
wordcp:
	and	%i2, 3, %i2		! unaligned leftover count
5:
	ld	[%i0], %o4		! read from address
	add	%i0, 4, %i0		! inc from address
	st	%o4, [%i1]		! write at destination address
	subcc	%i3, 4, %i3		! dec count
	bg	5b
	add	%i1, 4, %i1		! delay slot, inc to address
	b,a	bytecp

	! we come here to align copies on word boundaries
alwordcp:
	call	alignit			! go word-align it
	mov	3, %o0			! bits that must be zero to be aligned
	b,a	wordcp

	!
	! byte copy, works with any alignment
	!
1:
	add	%i0, 1, %i0		! inc from address
	stb	%o4, [%i1]		! write to address
	add	%i1, 1, %i1		! inc to address
bytecp:
	subcc	%i2, 1, %i2		! dec count
	bge,a	1b			! loop till done
	ldub	[%i0], %o4		! read from address
cpdone:
	st	%l6, [_u+U_LOFAULT]	! restore old u.u_lofault
	ret
	restore %g0, 0, %o0		! return (0)

/*
 * Common code used to align transfers on word and doubleword
 * boudaries.  Aligns source and destination and returns a count
 * of aligned bytes to transfer in %i3
 */
1:
	inc	%i0			! inc from
	stb	%o4, [%i1]		! write a byte
	inc	%i1			! inc to
	dec	%i2			! dec count
alignit:
	btst	%o0, %i0		! %o0 is bit mask to check for alignment
	bnz,a	1b
	ldub	[%i0], %o4		! read next byte

	retl
	andn	%i2, %o0, %i3		! return size of aligned bytes

/*
 * Copy a page of memory.
 * Assumes double word alignment and a count >= 256.
 *
 * pgcopy(from, to, count)
 *	caddr_t from, to;
 *	u_int count;
 */
	ENTRY(pgcopy)
	save	%sp, -SA(MINFRAME), %sp	! get another window
	mov	0x100, %i3
	!
	! loops have been unrolled so that 64 instructions(16 cache-lines)
	! are used; 256 bytes are moved each time through the loop
	! i0 - from; i1 - to; i2 - count; i3 - chunksize; o4,o5 -tmp
	!
	! We read a whole cache line and then we write it to
	! minimize thrashing.
	!
blkcopy:
	ldd	[%i0+0xf8], %l0		! 0xfc
	ldd	[%i0+0xf0], %l2
	std	%l0, [%i1+0xf8]
	std	%l2, [%i1+0xf0]

	ldd	[%i0+0xe8], %l0		! 0xec
	ldd	[%i0+0xe0], %l2
	std	%l0, [%i1+0xe8]
	std	%l2, [%i1+0xe0]

	ldd	[%i0+0xd8], %l0		! 0xdc
	ldd	[%i0+0xd0], %l2
	std	%l0, [%i1+0xd8]
	std	%l2, [%i1+0xd0]

	ldd	[%i0+0xc8], %l0		! 0xcc
	ldd	[%i0+0xc0], %l2
	std	%l0, [%i1+0xc8]
	std	%l2, [%i1+0xc0]

	ldd	[%i0+0xb8], %l0		! 0xbc
	ldd	[%i0+0xb0], %l2
	std	%l0, [%i1+0xb8]
	std	%l2, [%i1+0xb0]

	ldd	[%i0+0xa8], %l0		! 0xac
	ldd	[%i0+0xa0], %l2
	std	%l0, [%i1+0xa8]
	std	%l2, [%i1+0xa0]

	ldd	[%i0+0x98], %l0		! 0x9c
	ldd	[%i0+0x90], %l2
	std	%l0, [%i1+0x98]
	std	%l2, [%i1+0x90]

	ldd	[%i0+0x88], %l0		! 0x8c
	ldd	[%i0+0x80], %l2
	std	%l0, [%i1+0x88]
	std	%l2, [%i1+0x80]

	ldd	[%i0+0x78], %l0		! 0x7c
	ldd	[%i0+0x70], %l2
	std	%l0, [%i1+0x78]
	std	%l2, [%i1+0x70]

	ldd	[%i0+0x68], %l0		! 0x6c
	ldd	[%i0+0x60], %l2
	std	%l0, [%i1+0x68]
	std	%l2, [%i1+0x60]

	ldd	[%i0+0x58], %l0		! 0x5c
	ldd	[%i0+0x50], %l2
	std	%l0, [%i1+0x58]
	std	%l2, [%i1+0x50]

	ldd	[%i0+0x48], %l0		! 0x4c
	ldd	[%i0+0x40], %l2
	std	%l0, [%i1+0x48]
	std	%l2, [%i1+0x40]

	ldd	[%i0+0x38], %l0		! 0x3c
	ldd	[%i0+0x30], %l2
	std	%l0, [%i1+0x38]
	std	%l2, [%i1+0x30]

	ldd	[%i0+0x28], %l0		! 0x2c
	ldd	[%i0+0x20], %l2
	std	%l0, [%i1+0x28]
	std	%l2, [%i1+0x20]

	ldd	[%i0+0x18], %l0		! 0x1c
	ldd	[%i0+0x10], %l2
	std	%l0, [%i1+0x18]
	std	%l2, [%i1+0x10]

	ldd	[%i0+0x8], %l0		! 0x0c
	ldd	[%i0], %l2
	std	%l0, [%i1+0x8]
	std	%l2, [%i1]

instr:
	sub	%i2, %i3, %i2		! decrement count
	add	%i0, %i3, %i0		! increment from address
	cmp	%i2, 0x100		! enough to do another block?
	bge	blkcopy			! yes, do another chunk
	add	%i1, %i3, %i1		! increment to address
	tst	%i2			! all done yet?
	ble	cpdone			! yes, return
	cmp	%i2, 15			! can we do more cache lines
	bg,a	1f
	andn	%i2, 15, %i3		! %i3 bytes left, aligned (to 16 bytes)
	b	wcpchk
	andn	%i2, 3, %i3		! %i3 bytes left, aligned to 4 bytes
1:
	set	instr, %o5		! address of copy instructions
	sub	%o5, %i3, %o5		! jmp address relative to instr
	jmp	%o5
	nop

#endif never
/*
 * Block copy with possibly overlapped operands.
 *
 * ovbcopy(from, to, count)
 *	caddr_t from, to;
 *	u_int count;
 */
	ENTRY(ovbcopy)
	tst	%o2			! check count
	bg,a	1f			! nothing to do or bad arguments
	subcc	%o0, %o1, %o3		! difference of from and to address

	retl				! return
	nop
1:
	bneg,a	2f
	neg	%o3			! if < 0, make it positive
2:	cmp	%o2, %o3		! cmp size and abs(from - to)
	ble	_bcopy			! if size <= abs(diff): use bcopy,
	.empty				!   no overlap
	cmp	%o0, %o1		! compare from and to addresses
	blu	ov_bkwd			! if from < to, copy backwards
	nop
	!
	! Copy forwards.
	!
ov_fwd:
	ldub	[%o0], %o3		! read from address
	inc	%o0			! inc from address
	stb	%o3, [%o1]		! write to address
	deccc	%o2			! dec count
	bg	ov_fwd			! loop till done
	inc	%o1			! inc to address

	retl				! return
	nop
	!
	! Copy backwards.
	!
ov_bkwd:
	deccc	%o2			! dec count
	ldub	[%o0 + %o2], %o3	! get byte at end of src
	bg	ov_bkwd			! loop till done
	stb	%o3, [%o1 + %o2]	! delay slot, store at end of dst

	retl				! return
	nop

#ifdef never
/*
 * Zero a block of storage, returning an error code if we
 * take a kernel pagefault which cannot be resolved.
 * Returns EIO on pagefault error, 0 if all ok
 *
 * int
 * kzero(addr, count)
 *	caddr_t addr;
 *	u_int count;
 */
	ENTRY(kzero)
	sethi	%hi(zeroerr), %o2
	b	do_zero
	or	%o2, %lo(zeroerr), %o2

/*
 * We got here because of a fault during kzero.
 */
zeroerr:
	st	%o5, [_u+U_LOFAULT]	! restore old u.u_lofault
	retl
	restore	%g0, EIO, %o0		! return (EIO)

/*
 * Zero a block of storage.
 *
 * bzero(addr, length)
 *	caddr_t addr;
 *	u_int count;
 */
	ENTRY2(bzero,blkclr)
	mov	0, %o2
do_zero:
	ld	[_u+U_LOFAULT], %o5	! save u.u_lofault
	cmp	%o1, 15			! check for small counts
	bl	byteclr			! just clear bytes
	st	%o2, [_u+U_LOFAULT]	! install new vector

	!
	! Check for word alignment.
	!
	btst	3, %o0
	bz	_bzero_probe
	mov	0x100, %o3		! constant size of main loop
	!
	!
	! clear bytes until word aligned
	!
1:
	clrb	[%o0]
	add	%o0, 1, %o0
	btst	3, %o0
	bnz	1b
	sub	%o1, 1, %o1
	!
	! Word aligned.
	! Probe to see if word access is allowed (i.e. not VME_D16)
	! This assumes that the source will not change to VME_D16
	! during the bzero. This will get a data fault with
	! be_vmeserr set if unsuccessful. Trap will
	! then return to bzero_vme16. This is gross but fast.
	!
	.global _bzero_probe, _ebzero_probe
_bzero_probe:
	clr	[%o0]			! clear word to probe
_ebzero_probe:
	!
	! If needed move a word to become double-word aligned.
	!
	btst	7, %o0			! align to double-word boundary
	bz	2f
	clr	%g1			! clr g1 for second half of double %g0
	sub	%o1, 4, %o1
	b	2f
	add	%o0, 4, %o0

	!std	%g0, [%o0+0xf8]
zeroblock:
	std	%g0, [%o0+0xf0]
	std	%g0, [%o0+0xe8]
	std	%g0, [%o0+0xe0]
	std	%g0, [%o0+0xd8]
	std	%g0, [%o0+0xd0]
	std	%g0, [%o0+0xc8]
	std	%g0, [%o0+0xc0]
	std	%g0, [%o0+0xb8]
	std	%g0, [%o0+0xb0]
	std	%g0, [%o0+0xa8]
	std	%g0, [%o0+0xa0]
	std	%g0, [%o0+0x98]
	std	%g0, [%o0+0x90]
	std	%g0, [%o0+0x88]
	std	%g0, [%o0+0x80]
	std	%g0, [%o0+0x78]
	std	%g0, [%o0+0x70]
	std	%g0, [%o0+0x68]
	std	%g0, [%o0+0x60]
	std	%g0, [%o0+0x58]
	std	%g0, [%o0+0x50]
	std	%g0, [%o0+0x48]
	std	%g0, [%o0+0x40]
	std	%g0, [%o0+0x38]
	std	%g0, [%o0+0x30]
	std	%g0, [%o0+0x28]
	std	%g0, [%o0+0x20]
	std	%g0, [%o0+0x18]
	std	%g0, [%o0+0x10]
	std	%g0, [%o0+0x08]
	std	%g0, [%o0]
zinst:
	add	%o0, %o3, %o0		! increment source address
	sub	%o1, %o3, %o1		! decrement count
2:	cmp	%o1, 0x100		! can we do whole chunk?
	bge,a	zeroblock
	std	%g0, [%o0+0xf8]		! do first double of chunk

	cmp	%o1, 7			! can we zero any more double words
	ble	byteclr			! too small go zero bytes

	andn	%o1, 7, %o3		! %o3 bytes left, double-word aligned
	srl	%o3, 1, %o2		! using doubles, need 1 instr / 2 words
	set	zinst, %o4		! address of clr instructions
	sub	%o4, %o2, %o4		! jmp address relative to instr
	jmp	%o4
	nop
	!
	! do leftover bytes
	!
3:
	add	%o0, 1, %o0		! increment address
byteclr:
	subcc	%o1, 1, %o1		! decrement count
	bge,a	3b
	clrb	[%o0]			! zero a byte

	st	%o5, [_u+U_LOFAULT]	! restore old u.u_lofault
	retl
	mov	0, %o0			! return (0)

/*
 * bcopy_vme16(from, to, count)
 * Block copy to/from a 16 bit vme device.
 * There is little optimization because the VME is so slow.
 * We are only entered after a VME size error trap from within bcopy.
 * We get here with more than 3 bytes to copy.
 */
	ENTRY(bcopy_vme16)
	xor	%i0, %i1, %i4		! test for mutually half word aligned
	btst	1, %i4
	bnz	bytecp			! misaligned, copy bytes
	.empty
	btst	1, %i0			! test for initial byte
	bz,a	1f
	andn	%i2, 1, %i3		! count of aligned bytes to clear

	ldub	[%i0], %i4		! copy initial byte
	add	%i0, 1, %i0
	stb	%i4, [%i1]
	add	%i1, 1, %i1
	sub	%i2, 1, %i2
	andn	%i2, 1, %i3		! count of aligned bytes to clear
1:
	and	%i2, 1, %i2		! unaligned leftover count
2:
	lduh	[%i0], %i4		! read from address
	add	%i0, 2, %i0		! inc from address
	sth	%i4, [%i1]		! write at destination address
	subcc	%i3, 2, %i3		! dec count
	bg	2b
	add	%i1, 2, %i1		! delay slot, inc to address

	b,a	bytecp			! copy remaining bytes, if any

/*
 * bzero_vme16(addr, length)
 * Zero a block of VME_D16 memory.
 * There is little optimization because the VME is so slow.
 * We get here with more than 3 bytes to clear.
 */
	ENTRY(bzero_vme16)
	!
	! clear memory until halfword aligned
	!
1:
	btst	1, %o0			! test for half word aligned
	bz,a	2f
	andn	%o1, 1, %o3		! count of aligned bytes to clear

	clrb	[%o0]			! clear 1 byte
	sub	%o1, 1, %o1
	add	%o0, 1, %o0
	andn	%o1, 1, %o3		! count of aligned bytes to clear
	!
	! now half word aligned
	!
2:
	and	%o1, 1, %o1		! unaligned leftover count
3:
	clrh	[%o0]			! zero word
	subcc	%o3, 2, %o3		! decrement count
	bg	3b
	add	%o0, 2, %o0		! increment address

	b,a	byteclr			! zero remaining bytes, if any
#endif never

/*
 * Copy a null terminated string from one point to another in
 * the kernel address space.
 * NOTE - don't use %o5 in this routine as copy{in,out}str uses it.
 *
 * copystr(from, to, maxlength, lencopied)
 *	caddr_t from, to;
 *	u_int maxlength, *lencopied;
 */
	ENTRY(copystr)
	tst	%o2
	bg	1f
	mov	%o2, %o4		! save original count

	!
	! maxlength <= 0
	!
	bz	cs_out			! maxlength = 0
	mov	ENAMETOOLONG, %o0

	retl				! maxlength < 0
	mov	EFAULT, %o0		! return (EFAULT)

	!
	! Do a byte by byte loop.
	! We do this instead of a word by word copy because most strings
	! are small and this takes a small number of cache lines.
	!
0:
	add	%o0, 1, %o0		! interlock slot, inc src addr
	stb	%g1, [%o1]		! store byte
	tst	%g1			! null byte?
	bnz	1f
	add	%o1, 1, %o1		! incr dst addr

	b	cs_out			! last byte in string
	mov	0, %o0			! ret code = 0
1:
	subcc	%o2, 1, %o2		! test count
	bge,a	0b
	ldub	[%o0], %g1		! delay slot, get source byte

	mov	0, %o2			! max number of bytes moved
	mov	ENAMETOOLONG, %o0	! ret code = ENAMETOOLONG
cs_out:
	tst	%o3			! want length?
	bz	2f
	.empty
	sub	%o4, %o2, %o4		! compute length and store it
	st	%o4, [%o3]
2:
	retl
	nop				! return (ret code)

#ifdef never
/*
 * Transfer data to and from user space -
 * Note that these routines can cause faults
 * It is assumed that the kernel has nothing at
 * less than KERNELBASE in the virtual address space.
 */

/*
 * Copy kernel data to user space.
 *
 * int
 * copyout(kaddr, uaddr, count)
 *	caddr_t kaddr, uaddr;
 *	u_int count;
 */
	ENTRY(copyout)
	sethi	%hi(KERNELBASE), %g1	! test uaddr < KERNELBASE
	cmp	%o1, %g1
	sethi	%hi(copyioerr), %o3	! copyioerr is lofault value
	bleu	do_copy			! common code
	or	%o3, %lo(copyioerr), %o3

	retl				! return (EFAULT)
	mov	EFAULT, %o0

/*
 * Copy user data to kernel space.
 *
 * int
 * copyin(uaddr, kaddr, count)
 *	caddr_t uaddr, kaddr;
 *	u_int count;
 */
	ENTRY(copyin)
	sethi	%hi(KERNELBASE), %g1	! test uaddr < KERNELBASE
	cmp	%o0, %g1
	sethi	%hi(copyioerr), %o3	! copyioerr is lofault value
	bleu	do_copy			! common code
	or	%o3, %lo(copyioerr), %o3

	retl				! return (EFAULT)
	mov	EFAULT, %o0

/*
 * We got here because of a fault during copy{in,out}.
 */
copyioerr:
	st	%l6, [_u+U_LOFAULT]	! restore old u.u_lofault
	ret
	restore	%g0, EFAULT, %o0	! return (EFAULT)

/*
 * Copy a null terminated string from the user address space into
 * the kernel address space.
 *
 * copyinstr(uaddr, kaddr, maxlength, lencopied)
 *	caddr_t uaddr, kaddr;
 *	u_int maxlength, *lencopied;
 */
	ENTRY(copyinstr)
	sethi	%hi(KERNELBASE), %g1	! test uaddr < KERNELBASE
	cmp	%o0, %g1
	bgeu	copystrerr
	mov	%o7, %o5		! save return address
cs_common:
	set	copystrerr, %g1
	call	_copystr
	st	%g1, [_u+U_LOFAULT]	! catch faults

	jmp	%o5 + 8			! return (results of copystr)
	clr	[_u+U_LOFAULT]		! clear fault catcher

/*
 * Copy a null terminated string from the kernel
 * address space to the user address space.
 *
 * copyoutstr(kaddr, uaddr, maxlength, lencopied)
 *	caddr_t kaddr, uaddr;
 *	u_int maxlength, *lencopied;
 */
	ENTRY(copyoutstr)
	sethi	%hi(KERNELBASE), %g1	! test uaddr < KERNELBASE
	cmp	%o1, %g1
	blu	cs_common
	mov	%o7, %o5		! save return address
	! fall through

/*
 * Fault while trying to move from or to user space.
 * Set and return error code.
 */
copystrerr:
	mov	EFAULT, %o0
	jmp	%o5 + 8			! return (EFAULT)
	clr	[_u+U_LOFAULT]

/*
 * Fetch user (long) word.
 *
 * int
 * fuword(addr)
 *	caddr_t addr;
 */
	ENTRY2(fuword,fuiword)
	sethi	%hi(KERNELBASE), %o3	! compare access addr to KERNELBASE
	cmp	%o0, %o3		! if (KERNELBASE >= addr) error
	bgeu	fsuerr
	btst	0x3, %o0		! test alignment
	bne	fsuerr
	.empty
	set	fsuerr, %o3		! set u.u_lofault to catch any fault
	st	%o3, [_u+U_LOFAULT]
	ld	[%o0], %o0		! get the word
	retl
	clr	[_u+U_LOFAULT]		! clear u.u_lofault

/*
 * Fetch user byte.
 *
 * int
 * fubyte(addr)
 *	caddr_t addr;
 */
	ENTRY2(fubyte,fuibyte)
	sethi	%hi(KERNELBASE), %o3	! compare access addr to KERNELBASE
	cmp	%o0, %o3		! if (KERNELBASE >= addr) error
	bgeu	fsuerr
	.empty
	set	fsuerr, %o3		! set u.u_lofault to catch any fault
	st	%o3, [_u+U_LOFAULT]
	ldub	[%o0], %o0		! get the byte
	retl
	clr	[_u+U_LOFAULT]		! clear u.u_lofault

/*
 * Set user (long) word.
 *
 * int
 * suword(addr, value)
 *	caddr_t addr;
 *	int value;
 */
	ENTRY2(suword,suiword)
	sethi	%hi(KERNELBASE), %o3	! compare access addr to KERNELBASE
	cmp	%o0, %o3		! if (KERNELBASE >= addr) error
	bgeu	fsuerr
	btst	0x3, %o0		! test alignment
	bne	fsuerr
	.empty
	set	fsuerr, %o3		! set u.u_lofault to catch any fault
	st	%o3, [_u+U_LOFAULT]
	st	%o1, [%o0]		! set the word
	b,a	suret

/*
 * Set user byte.
 *
 * int
 * subyte(addr, value)
 *	caddr_t addr;
 *	int value;
 */
	ENTRY2(subyte,suibyte)
	sethi	%hi(KERNELBASE), %o3	! compare access addr to KERNELBASE
	cmp	%o0, %o3		! if (KERNELBASE >= addr) error
	bgeu	fsuerr
	.empty
	set	fsuerr, %o3		! set u.u_lofault to catch any fault
	st	%o3, [_u+U_LOFAULT]
	stb	%o1, [%o0]		! set the byte
suret:
	mov	0, %o0			! indicate success
	retl
	clr	[_u+U_LOFAULT]		! clear u.u_lofault

/*
 * Fetch user short (half) word.
 *
 * int
 * fusword(addr)
 *	caddr_t addr;
 */
	ENTRY(fusword)
	sethi	%hi(KERNELBASE), %o3	! compare access addr to KERNELBASE
	cmp	%o0, %o3		! if (KERNELBASE >= addr) error
	bgeu	fsuerr
	btst	0x1, %o0		! test alignment
	bne	fsuerr
	.empty
	set	fsuerr, %o3		! set u.u_lofault to catch any fault
	st	%o3, [_u+U_LOFAULT]
	lduh	[%o0], %o0		! get the half word
	retl
	clr	[_u+U_LOFAULT]		! clear u.u_lofault

/*
 * Set user short word.
 *
 * int
 * susword(addr, value)
 *	caddr_t addr;
 *	int value;
 */
	ENTRY(susword)
	sethi	%hi(KERNELBASE), %o3	! compare access addr to KERNELBASE
	cmp	%o0, %o3		! if (KERNELBASE >= addr) error
	bgeu	fsuerr
	btst	0x1, %o0		! test alignment
	bne	fsuerr
	.empty
	set	fsuerr, %o3		! set u.u_lofault to catch any fault
	st	%o3, [_u+U_LOFAULT]
	sth	%o1, [%o0]		! set the half word
	b,a	suret

fsuerr:
	mov	-1, %o0			! return error
	retl
	clr	[_u+U_LOFAULT]		! clear u.u_lofault

#endif never
