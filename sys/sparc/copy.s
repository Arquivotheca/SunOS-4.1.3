/*	@(#)copy.s 1.1 92/07/30 SMI	*/

	.seg	"text"
	.align	4

/*
 *	Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include <sys/errno.h>
#include <machine/asm_linkage.h>
#include <machine/mmu.h>
#include <machine/pte.h>
#include "assym.s"

#ifdef	sun4m
#define	PTE_CACHEABLEMASK	0x80	/* XXX- move to pte.h */
#endif	sun4m

/*
 * limit use of the bcopy buffer to transfers of at least this size
 * if the tranfer isn't at least two cache lines in size, forget it
 */
#ifdef	sun4m
#define	BCOPY_BUF
#define	BCPY_BLKSZ	0x20
#define BCOPY_LIMIT	0x200	/* XXX- for now looks like best value */
#else
#define BCOPY_LIMIT	0x40
#endif	sun4m

/* 
 * sun4m uses a different define for probing the mmu.  Set the correct
 * define in the case of the sun4m.
 */
#ifdef sun4m
#undef	ASI_PM
#define	ASI_PM		ASI_FLPR

/*
 * These constants may not be used on a Sun-4M in the
 * way that this file uses them.
 */
#undef	VME_D16
#undef	VME_D32
#endif sun4m

/*
 * FAST_BLKCOPY enables a specially scheduled set of ldd/std
 * that should be optimal for all page copies.
 * Unfortunately, it uses %l6, which needs to be maintained.
 *
 * #define	FAST_BLKCOPY
 */

/*
 * Copy a block of storage, returning an error code if `from' or
 * `to' takes a kernel pagefault which cannot be resolved.
 * Returns errno value on pagefault error, 0 if all ok
 *
 * int
 * kcopy(from, to, count)
 *	caddr_t from, to;
 *	u_int count;
 */
	ENTRY(kcopy)
	set	copyerr, %o3		! copyerr is lofault value
do_copy:
	save	%sp, -SA(MINFRAME), %sp	! get another window
	sethi	%hi(_uunix), %g5
	ld	[%g5 + %lo(_uunix)], %g5
	ld	[%g5+U_LOFAULT], %l6	! save u.u_lofault
	b	bcopy_cmn		! common code
	st	%i3, [%g5+U_LOFAULT]	! install new vector

/*
 * We got here because of a fault during kcopy.
 * Errno value is in %g1.
 */
copyerr:
	sethi	%hi(_uunix), %g5
	ld	[%g5 + %lo(_uunix)], %g5
	st	%l6, [%g5+U_LOFAULT]	! restore old u.u_lofault
	ret
	restore	%g1, 0, %o0

/*
 * Copy a block of storage - must not overlap (from + len <= to).
 * Registers: l6 - saved u.u_lofault
 *
 * bcopy(from, to, count)
 *	caddr_t from, to;
 *	u_int count;
 */
	ENTRY(bcopy)
	save	%sp, -SA(MINFRAME), %sp	! get another window
	sethi	%hi(_uunix), %g5		! XXX - global u register?
	ld	[%g5 + %lo(_uunix)], %g5
	ld	[%g5+U_LOFAULT], %l6	! save u.u_lofault
bcopy_cmn:
	cmp	%i2, 12			! for small counts
	bl,a	dbytecp			! just copy bytes

	sub	%i0, %i1, %i0		! i0 gets difference of src and dst
#ifdef VME_D16
	!
	! Check src and dest page types for vme 16 or 32 bit access
	!
	andn	%i0, 3, %g1		! word align src
	ldub	[%i0],%g0		! make sure page is in memory
	lda	[%g1]ASI_PM, %g2	! get its pte

	andn	%i1, 3, %g1		! word align dest
	ldub	[%i1],%g0		! make sure page is in memory
	lda	[%g1]ASI_PM, %g1	! get its pte

	or	%g1, %g2, %g1		! if either
	srl	%g1, PGT_SHIFT, %g1	! page type is vme space
	andcc	%g1, VME_D16|VME_D32, %g1
	bz	bcopy_obmem		! no, do 'on board' memory
	cmp	%g1, VME_D16		! which vme?
	be	bcopy_vme16
	nop
	b	bcopy_vme32		! next instruction needed in delay slot 
	nop
#endif VME_D16
#ifdef SUN4M_690
	and	%i0, MMU_PAGEMASK, %i3
	or	%i3, FT_ALL<<8, %i3
	lda	[%i3]ASI_PM, %i4	! get source PTE

	and	%i4, 3, %i4
	cmp	%i4, MMU_ET_PTE
	bne,a	1f			! if not mapped in,
	ldub	[%i0], %g0		!   force source page into memory
1:
	lda	[%i3]ASI_PM, %i4	! get source PTE again

	srl	%i4, 28, %i4		! convert pte to space

	! if source is vme D16, limit transfer width
	! to 16 bits. (space=0xA or 0xC)
	cmp	%i4, 0xA		! from user vme-d16
	be	bcopy_vme16
	cmp	%i4, 0xC		! from supv vme-d16
	be	bcopy_vme16

	! if source is vme D32, limit transfer width
	! to 32 bits. (space=0xB or 0xD)
	cmp	%i4, 0xB		! from user vme-d32
	be	bcopy_vme32
	cmp	%i4, 0xD		! from supv vme-d32
	be	bcopy_vme32

	and	%i1, MMU_PAGEMASK, %i3
	or	%i3, FT_ALL<<8, %i3
	lda	[%i3]ASI_PM, %i4	! get destination PTE

	and	%i4, 3, %i4
	cmp	%i4, MMU_ET_PTE
	bne,a	1f			! if not mapped in,
	ldub	[%i1], %g0		!   force destination page into memory
1:
	lda	[%i3]ASI_PM, %i4	! get destination PTE again

	srl	%i4, 28, %i4		! convert pte to space

	! if destination is vme D16, limit transfer width
	! to 16 bits. (space=0xA or 0xC)
	cmp	%i4, 0xA		! to user vme-d16
	be	bcopy_vme16
	cmp	%i4, 0xC		! to supv vme-d16
	be	bcopy_vme16

	! if destination is vme D32, limit transfer width
	! to 32 bits. (space=0xB or 0xD)
	cmp	%i4, 0xB		! to user vme-d32
	be	bcopy_vme32
	cmp	%i4, 0xD		! to supv vme-d32
	be	bcopy_vme32
	nop
#endif
	!
	! use aligned transfers where possible
	!
bcopy_obmem:
	xor	%i0, %i1, %o4		! xor from and to address
	btst	7, %o4			! if lower three bits zero
	bz	aldoubcp		! can align on double boundary
	.empty	! assembler complaints about label
#if defined(VME_D16) || defined(SUN4M_690)
bcopy_vme32:
	xor	%i0, %i1, %o4		! xor from and to address
#endif /* VME_D16 || SUN4M_690 */
! (why is this label here?) bcopy_words:
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
	mov	%l4, %i2		! delay slot, mv cnt where dbytecp wants
	b	dbytecp			! let dbytecp do the rest
	sub	%i0, %i1, %i0		! i0 gets the difference of src and dst
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

#ifdef	BCOPY_BUF
bcp_bufchk:
	!
	! source and destination are now double-word aligned
	! test for bcopy buffer, on machines that don't have
	! a bcopy buffer the variable bcopy_res is initialized
	! to -1 which keeps anyone from using it
	!
	! the following commented out code is useable when different
	! machines have different sized bcopy buffers
	! it is overkill for now and not used, maybe later....
	!
	! sethi	%hi(_bcopy_res), %o2
	! ld	[%o2 + %lo(_bcopy_res)], %o3 ! is hardware available
	! tst	%o3
	! bnz	bcp_nobuf		! reserved or not enabled
	! sethi	_vac_linesize, %o4
	! ld	[%o4 + %lo(_vac_linesize)], %o4	! cache line size
	! sll	%o4, 1, %o5		! if > 2 lines try to use hardware
	! cmp	%o2, %o5		! see if it is worth while
	! blt	bcp_nobuf		! if not more than 2 lines, punt
	! sub	%o4, 1, %o5		! create alignment mask
	! andcc	%o1, %o5, %o1
	! xor	%i0, %i1, %o1		! check alignment of src and dest
	! bnz	bcp_nobuf		! not aligned, can't use hardware
	! andcc	%i0, %o5, %o1		! check src alignment
	! bz	bcp_buf			! if src is aligned, then so is dest
	! nop				! if not, copy doubles until aligned
	!1:
	! ldd	[%i0], %o0		! align to cache
	! std	%o0, [%i1]
	! sub	%i2, 8, %i2		! update count
	! add	%i0, 8, %i0		! update source address
	! andcc	%i0, %o5, %o3		! later....
	! bnz	1b
	! add	%i1, 8, %i1		! update dest address
	!bcp_buf:
	!
	sethi	%hi(_bcopy_res), %o5
	ld	[%o5 + %lo(_bcopy_res)], %o3 ! is hardware available
	mov	32, %o4			! interlock, set buffer size
	tst	%o3
	bnz	bcp_nobuf		! reserved or not enabled
	cmp	%i2, BCOPY_LIMIT	! see if it is worth while
	blt	bcp_nobuf		! if not more than 2 lines, punt

	xor	%i0, %i1, %o1		! check alignment of src and dest
	andcc	%o1, 0x1F, %o1
	bnz	bcp_nobuf		! not aligned, can't use hardware
	andcc	%i0, 0x1F, %o1		! check src alignment
	bz	bcp_buf			! if src is aligned, then so is dest
	nop				! if not, copy doubles until aligned
1:
	ldd	[%i0], %o0		! align to cache
	std	%o0, [%i1]
	sub	%i2, 8, %i2		! update count
	add	%i0, 8, %i0		! update source address
	andcc	%i0, 0x1F, %o3
	bnz	1b
	add	%i1, 8, %i1		! update dest address

	andn	%i2, 7, %i3
bcp_buf:
	ldstub	[%o5 + %lo(_bcopy_res)], %o3 ! try to grab hardware
	tst	%o3
	bnz	bcp_nobuf		! hardware in use, use software
	nop
!
! XXX--Put in something to efficiently use the bcopy HW when the
! src and dest addrs arent aligned, and the size of the copy is large.
!
! Should only get here on sun4m when have viking/mxcc
#ifdef sun4m
	xor	%i0, %i1, %i5		! check alignment of src and dest
	btst	MMU_PAGEOFFSET, %i5
	bz,a	1f			! src and dest aligned for page copies
	mov	0x10, %l2		! set cacheable bit in upper word
!
! Case where source and dest are not aligned, but neither crosses a page
! boundary.  Can still use HW in this case.
! Happens quite a bit, so its a big win.
!
	and	%i0, MMU_PAGEOFFSET, %i5
	add	%i5, %i2, %i5
	set	MMU_PAGESIZE, %i4
	cmp	%i5, %i4
	bg,a	bcp_nobuf		! Use SW, src would exceed page boundary
	st	%g0, [%o5 + %lo(_bcopy_res)]

	and	%i1, MMU_PAGEOFFSET, %i5
	add	%i5, %i2, %i5
	cmp	%i5, %i4		! %i4 still contains MMU_PAGESIZE
	ble	1f			! src and dest wont cross page
	mov	0x10, %l2		! set cacheable bit in upper word
					! Use SW, dst would exceed page boundary
	b	bcp_nobuf		! zero out HW lock word
	st	%g0, [%o5 + %lo(_bcopy_res)]
1:
	set	MXCC_STRM_SRC, %l5	! addr of stream source reg
	set	MXCC_STRM_DEST, %l4	! addr of stream destination reg

!
! spl here because dont want phys pages to be stolen from us until
! we have completed the copy.
!
	call	_splhigh		! Watch register usage on calls!
	ldub	[%i0], %o4		! Set R bit on src page
	mov	%o0, %l7		! must save if make any other calls
	stb	%o4, [%i1]		! Set RM bits in dest page
3:
	and	%i1, MMU_PAGEMASK, %l3	! destination virtual page
	or	%l3, FT_ALL<<8, %l3	! find destination phys addr
	lda	[%l3]ASI_PM, %l3	! get phys addr
	and	%l3, 3, %i3
	cmp	%i3, MMU_ET_PTE
	bne,a	3b			! if not mapped in,
	stb	%o4, [%i1]		! force target page into memory

	srl	%l3, 28, %i3		! check space
	tst	%i3
	bz	1f			! Obmem page
	btst	PTE_CACHEABLEMASK, %l3	! Double check, page marked cacheable?
	bz,a	2f			! Not obmem, and not cacheable
	mov	%i3, %l2		

	b	2f			! Not obmem, but cacheable
	or	%l2, %i3, %l2
1:
	bnz	2f			! Yes, obmem and cacheable, go ahead.
	nop
	mov	%g0, %l2		! No, clear cacheable bit.
2:
	srl	%l3, 8, %l3		! remove non-page bits from pte
	sll	%l3, MMU_PAGESHIFT, %l3	! phys page minus space bits
	and	%i1, MMU_PAGEOFFSET, %i3	! page offset via virt addr
	or	%l3, %i3, %l3		! destination phys addr complete
	mov	0x10, %l0		! set cacheable bit in upper word
3:
	and	%i0, MMU_PAGEMASK, %l1	! source virtual page
	or	%l1, FT_ALL<<8, %l1	! find source phys addr
	lda	[%l1]ASI_PM, %l1	! get phys addr
	and	%l1, 3, %i3
	cmp	%i3, MMU_ET_PTE
	bne,a	3b			! if not mapped in,
	ldub	[%i0], %g0		! force target page into memory
	
	srl	%l1, 28, %i3		! check space
	tst	%i3
	bz	1f			! Obmem page
	btst	PTE_CACHEABLEMASK, %l1	! Double check, page marked cacheable?
	bz,a	2f			! Not obmem, and not cacheable
	mov	%i3, %l0

	b	2f			! Not obmem, but cacheable
	or	%l0, %i3, %l0
1:
	bnz	2f			! Yes, obmem and cacheable, go ahead.
	nop
	mov	%g0, %l0		! No, clear cacheable bit.
2:
	srl	%l1, 8, %l1		! remove non-page bits from pte
	sll	%l1, MMU_PAGESHIFT, %l1	! phys page minus space bits
	and	%i0, MMU_PAGEOFFSET, %i3	! page offset via virt addr
	or	%l1, %i3, %l1		! source phys addr complete

	add	%i3, %i2, %i4		! will we cross page boundary?
	set	MMU_PAGESIZE, %o4	! use for pagesize boundary check
	cmp	%i4, %o4		! Yes if %i3+%i2 > PAGESIZE
	ble,a	hw_fastbcopy		! No.  Can copy all bytes and exit.
	mov	%i2, %o4		! %i2 has the requested transfer size
					! if yes, loop on page copies
	sub	%o4, %i3, %o4		! round transfer to PAGESIZE
	b	hw_pagecopy		! copy bytes in leading page
	sub	%i2, %o4, %i2		! decrease by what we will copy

hw_bcopyloop:
	mov	%l7, %o0		! %l7 holds results of splhigh
	call	_splx			! allow window for interrupts
	mov	0x10, %l0

	set	BCPY_BLKSZ, %i5		! Check if virt->phys translations
	cmp	%i2, %i5		!    will be necessary
	bl	9f			! Not necessary.  Exit, 
	nop				!     skipping the call to splx()
	
	call	_splhigh
	ldub	[%i0], %o4		! Set R bit in src page
	mov	%o0, %l7		! save return value of splhigh
3:
	mov	%i0, %l1
	or	%l1, FT_ALL<<8, %l1	! find new source phys addr
	lda	[%l1]ASI_PM, %l1	! get phys addr
	and	%l1, 3, %i3
	cmp	%i3, MMU_ET_PTE
	bne,a	3b			! if not mapped in,
	ldub	[%i0], %g0		! force target page into memory
	
	srl	%l1, 28, %i3		! check space
	tst	%i3
	bz	1f			! Obmem page
	btst	PTE_CACHEABLEMASK, %l1	! Double check, page marked cacheable?
	bz,a	2f			! Not obmem, and not cacheable
	mov	%i3, %l0

	b	2f			! Not obmem, but cacheable
	or	%l0, %i3, %l0
1:
	bnz	2f			! Yes, obmem and cacheable, go ahead.a
	nop
	mov	%g0, %l0		! No, clear cacheable bit.
2:
	srl	%l1, 8, %l1		! remove non-page bits from pte
	sll	%l1, MMU_PAGESHIFT, %l1	! phys page minus space bits
					!    We should be on page boundary
	mov	0x10, %l2
	stb	%o4, [%i1]		! Set RM bits in dest page
3:
	mov	%i1, %l3
	or	%l3, FT_ALL<<8, %l3	! find new dest phys addr
	lda	[%l3]ASI_PM, %l3	! get phys addr
	and	%l3, 3, %i3
	cmp	%i3, MMU_ET_PTE
	bne,a	3b			! if not mapped in,
	stb	%o4, [%i1]
	
	srl	%l3, 28, %i3		! check space
	tst	%i3
	bz	1f			! Obmem page
	btst	PTE_CACHEABLEMASK, %l3	! Double check, page marked cacheable?
	bz,a	2f			! Not obmem, and not cacheable
	mov	%i3, %l2

	b	2f			! Not obmem, but cacheable
	or	%l2, %i3, %l2		
1:
	bnz	2f			! Yes, obmem and cacheable, go ahead
	nop
	mov	%g0, %l2		! No, clear cacheable bit.
2:
	srl	%l3, 8, %l3		! remove non-page bits from pte
	sll	%l3, MMU_PAGESHIFT, %l3	! phys page minus space bits
					!    We should be on page boundary
	set	MMU_PAGESIZE, %o4
	cmp	%i2, %o4		! Do we have more than one page left?
	bg,a	hw_pagecopy		! Yes, call hw_pagecopy to loop
	sub	%i2, %o4, %i2		! decrease by what we will copy
					!    Always copy PAGESIZE here
	b	hw_fastbcopy		! its not
	mov	%i2, %o4

hw_fastbcopy:				! use for < PAGESIZE copies.
	stda	%l0, [%l5]ASI_MXCC
	stda	%l2, [%l4]ASI_MXCC
	dec	BCPY_BLKSZ, %o4		! decrement count
	inc	BCPY_BLKSZ, %i0		! increment source virt address
	inc	BCPY_BLKSZ, %i1		! increment dest virt address
	cmp	%o4, BCPY_BLKSZ		! still got more than BCPY_BLKSZ to go?
	bl,a	hw_bcopyexit		! No, done with HW
	mov	%o4, %i2		! what we need byteclr to finish up
	inc	BCPY_BLKSZ, %l1		! increment to next sublock
	b	hw_fastbcopy		! loop until finished
	inc	BCPY_BLKSZ, %l3		! increment to next sublock

hw_pagecopy:
	stda	%l0, [%l5]ASI_MXCC
	stda	%l2, [%l4]ASI_MXCC
	inc	BCPY_BLKSZ, %i0		! increment source virtual address
	deccc	BCPY_BLKSZ, %o4		! decrement count
	bz	hw_bcopyloop		! done
	inc	BCPY_BLKSZ, %i1		! increment dest virt address
	inc	BCPY_BLKSZ, %l1		! increment physaddr one sublock
	b	hw_pagecopy		! more to do
	inc	BCPY_BLKSZ, %l3		! increment to next sublock

hw_bcopyexit:
	mov	%l7, %o0		! %l7 holds results of splhigh call
	call	_splx
	.empty				! silence complaint about label
!
! Check Error Register to see if AE bit set, indicating problem with
! stream operation we just did.  Fatal, so we go down if a problem
! is found.
!
9:
	sethi	%hi(_bcopy_cnt), %o3	! count # times used buffer

	set	MXCC_ERROR, %l7		! Load addr of MXCC err reg
	ldda	[%l7]ASI_MXCC, %l4	! %l5 hold bits 0-31 of paddr
	set	MXCC_ERR_AE, %o4		
	btst	%o4, %l4		! %l4 hold the status bits
	bz	1f			! If bit not set continue exit
	ld	[%o3 + %lo(_bcopy_cnt)], %o4

	set	MXCC_ERR_EV, %i4	! If error bit set...
	btst	%i4, %l4		!    check if valid bit set
	bnz	6f
	.empty				! Silence complaint about label
1:
	st	%g0, [%o5 + %lo(_bcopy_res)] ! interlock, unlock hardware
	inc	%o4
	b	bytecp			! do remaining bytes, if any
	st	%o4, [%o3 + %lo(_bcopy_cnt)]
6:
	set	0f, %o0
	call	_panic
	nop
0:
	.asciz	"bcopy stream operation failed"
	.align	4

#else	sun4m
!
! This is where we go if we are using HW bcopy
! but it isnt a viking/mxcc
!
1:	lda	[%i0]ASI_BC, %g0	! fill buffer
	sub	%i2, %o4, %i2		! update count
	add	%i0, %o4, %i0		! update source address
	cmp	%i2, %o4		! check if finished
	sta	%g0, [%i1]ASI_BC	! write buffer
	bge	1b			! loop until done
	add	%i1, %o4, %i1		! update dest address

	sethi	%hi(_bcopy_cnt), %o3	! count # times used buffer
	ld	[%o3 + %lo(_bcopy_cnt)], %o4
	st	%g0, [%o5 + %lo(_bcopy_res)] ! interlock, unlock hardware
	inc	%o4
	b	bytecp			! do remaining bytes, if any
	st	%o4, [%o3 + %lo(_bcopy_cnt)]

#endif	sun4m

bcp_nobuf:
#endif	BCOPY_BUF

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
	sub	%i0, %i1, %i0		! i0 gets the difference of src and dst
5:
	ldd	[%i0+%i1], %o4		! read from address
	std	%o4, [%i1]		! write at destination address
	subcc	%i3, 8, %i3		! dec count
	bg	5b
	add	%i1, 8, %i1		! delay slot, inc to address
wcpchk:
	cmp	%i2, 4			! see if we can copy a word
	bl	dbytecp			! if 3 or less bytes use bytecp
	.empty
	!
	! for leftover bytes we fall into wordcp, if needed
	!
wordcp:
	and	%i2, 3, %i2		! unaligned leftover count
5:
	ld	[%i0+%i1], %o4		! read from address
	st	%o4, [%i1]		! write at destination address
	subcc	%i3, 4, %i3		! dec count
	bg	5b
	add	%i1, 4, %i1		! delay slot, inc to address
	b,a	dbytecp

	! we come here to align copies on word boundaries
alwordcp:
	call	alignit			! go word-align it
	mov	3, %o0			! bits that must be zero to be aligned
	b	wordcp
	sub	%i0, %i1, %i0		! i0 gets the difference of src and dst

	!
	! byte copy, works with any alignment
	!
bytecp:	b	dbytecp
	sub	%i0, %i1, %i0		! i0 gets difference of src and dst

	!
	! differenced byte copy, works with any alignment
	! assumes dest in %i1 and (source - dest) in %i0
	!
1:
	stb	%o4, [%i1]		! write to address
	inc	%i1			! inc to address
dbytecp:
	deccc	%i2			! dec count
	bge,a	1b			! loop till done
	ldub	[%i0+%i1], %o4		! read from address
cpdone:
	sethi	%hi(_uunix), %g5
	ld	[%g5 + %lo(_uunix)], %g5
	st	%l6, [%g5+U_LOFAULT]	! restore old u.u_lofault
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
#ifdef	FAST_BLKCOPY
!
! This schedule is optimal for 4/60 and 4/2xx, and is
! nearly optimal for 4/3xx. Measurements should be
! taken also on 4/1xx and 4/4xx products, and
! eventually on all MBUS modules.

	ldd	[%i0+0xf8], %l0
	 ldd	[%i0+0xf0], %l2	;	std	%l0, [%i1+0xf8]
	  ldd	[%i0+0xe8], %l4	;	 std	%l2, [%i1+0xf0]
	   ldd	[%i0+0xe0], %l6	;	  std	%l4, [%i1+0xe8]
	ldd	[%i0+0xd8], %l0	;	   std	%l6, [%i1+0xe0]
	 ldd	[%i0+0xd0], %l2	;	std	%l0, [%i1+0xd8]
	  ldd	[%i0+0xc8], %l4	;	 std	%l2, [%i1+0xd0]
	   ldd	[%i0+0xc0], %l6	;	  std	%l4, [%i1+0xc8]
	ldd	[%i0+0xb8], %l0	;	   std	%l6, [%i1+0xc0]
	 ldd	[%i0+0xb0], %l2	;	std	%l0, [%i1+0xb8]
	  ldd	[%i0+0xa8], %l4	;	 std	%l2, [%i1+0xb0]
	   ldd	[%i0+0xa0], %l6	;	  std	%l4, [%i1+0xa8]
	ldd	[%i0+0x98], %l0	;	   std	%l6, [%i1+0xa0]
	 ldd	[%i0+0x90], %l2	;	std	%l0, [%i1+0x98]
	  ldd	[%i0+0x88], %l4	;	 std	%l2, [%i1+0x90]
	   ldd	[%i0+0x80], %l6	;	  std	%l4, [%i1+0x88]
	ldd	[%i0+0x78], %l0	;	   std	%l6, [%i1+0x80]
	 ldd	[%i0+0x70], %l2	;	std	%l0, [%i1+0x78]
	  ldd	[%i0+0x68], %l4	;	 std	%l2, [%i1+0x70]
	   ldd	[%i0+0x60], %l6	;	  std	%l4, [%i1+0x68]
	ldd	[%i0+0x58], %l0	;	   std	%l6, [%i1+0x60]
	 ldd	[%i0+0x50], %l2	;	std	%l0, [%i1+0x58]
	  ldd	[%i0+0x48], %l4	;	 std	%l2, [%i1+0x50]
	   ldd	[%i0+0x40], %l6	;	  std	%l4, [%i1+0x48]
	ldd	[%i0+0x38], %l0	;	   std	%l6, [%i1+0x40]
	 ldd	[%i0+0x30], %l2	;	std	%l0, [%i1+0x38]
	  ldd	[%i0+0x28], %l4	;	 std	%l2, [%i1+0x30]
	   ldd	[%i0+0x20], %l6	;	  std	%l4, [%i1+0x28]
	ldd	[%i0+0x18], %l0	;	   std	%l6, [%i1+0x20]
	 ldd	[%i0+0x10], %l2	;	std	%l0, [%i1+0x18]
	  ldd	[%i0+0x08], %l4	;	 std	%l2, [%i1+0x10]
	   ldd	[%i0+0x00], %l6	;	  std	%l4, [%i1+0x08]
	b	instr		;	   std	%l6, [%i1+0x00]

#endif FAST_BLKCOPY
/*
 * This code generated from the sun4m I/O perf work.
 */
#ifdef	sun4m
	ldd	[%i0+0xf8], %l0		! 0xfc
	ldd	[%i0+0xf0], %l2
	ldd	[%i0+0xe8], %o0		
	ldd	[%i0+0xe0], %o2
	std	%l0, [%i1+0xf8]
	std	%l2, [%i1+0xf0]
	std	%o0, [%i1+0xe8]
	std	%o2, [%i1+0xe0]

	ldd	[%i0+0xd8], %l0		! 0xdc
	ldd	[%i0+0xd0], %l2
	ldd	[%i0+0xc8], %o0		
	ldd	[%i0+0xc0], %o2
	std	%l0, [%i1+0xd8]
	std	%l2, [%i1+0xd0]
	std	%o0, [%i1+0xc8]
	std	%o2, [%i1+0xc0]

	ldd	[%i0+0xb8], %l0		! 0xbc
	ldd	[%i0+0xb0], %l2
	ldd	[%i0+0xa8], %o0		
	ldd	[%i0+0xa0], %o2
	std	%l0, [%i1+0xb8]
	std	%l2, [%i1+0xb0]
	std	%o0, [%i1+0xa8]
	std	%o2, [%i1+0xa0]

	ldd	[%i0+0x98], %l0		! 0x9c
	ldd	[%i0+0x90], %l2
	ldd	[%i0+0x88], %o0		
	ldd	[%i0+0x80], %o2
	std	%l0, [%i1+0x98]
	std	%l2, [%i1+0x90]
	std	%o0, [%i1+0x88]
	std	%o2, [%i1+0x80]

	ldd	[%i0+0x78], %l0		! 0x7c
	ldd	[%i0+0x70], %l2
	ldd	[%i0+0x68], %o0	
	ldd	[%i0+0x60], %o2
	std	%l0, [%i1+0x78]
	std	%l2, [%i1+0x70]
	std	%o0, [%i1+0x68]
	std	%o2, [%i1+0x60]

	ldd	[%i0+0x58], %l0		! 0x5c
	ldd	[%i0+0x50], %l2
	ldd	[%i0+0x48], %o0
	ldd	[%i0+0x40], %o2
	std	%l0, [%i1+0x58]
	std	%l2, [%i1+0x50]
	std	%o0, [%i1+0x48]
	std	%o2, [%i1+0x40]

	ldd	[%i0+0x38], %l0		! 0x3c
	ldd	[%i0+0x30], %l2
	ldd	[%i0+0x28], %o0	
	ldd	[%i0+0x20], %o2
	std	%l0, [%i1+0x38]
	std	%l2, [%i1+0x30]
	std	%o0, [%i1+0x28]
	std	%o2, [%i1+0x20]

	ldd	[%i0+0x18], %l0		! 0x1c
	ldd	[%i0+0x10], %l2
	ldd	[%i0+0x8], %o0		
	ldd	[%i0], %o2
	std	%l0, [%i1+0x18]
	std	%l2, [%i1+0x10]
	std	%o0, [%i1+0x8]
	std	%o2, [%i1]
instr:

	sub	%i2, %i3, %i2		! decrement count
	add	%i0, %i3, %i0		! increment from address
	cmp	%i2, 0x100		! enough to do another block?
	bge	blkcopy			! yes, do another chunk
	add	%i1, %i3, %i1		! increment to address
	tst	%i2			! all done yet?
	ble	cpdone			! yes, return
	cmp	%i2, 31			! can we do more cache lines
	bg,a	1f
	andn	%i2, 31, %i3		! %i3 bytes left, aligned (to 32 bytes)
	andn	%i2, 3, %i3		! %i3 bytes left, aligned to 4 bytes
	b	wcpchk
	sub	%i0, %i1, %i0		! create diff of src and dest addr
1:
	set	instr, %o5		! address of copy instructions
	sub	%o5, %i3, %o5		! jmp address relative to instr
	jmp	%o5
	nop

#else sun4m

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
	andn	%i2, 3, %i3		! %i3 bytes left, aligned to 4 bytes
	b	wcpchk
	sub	%i0, %i1, %i0		! create diff of src and dest addr
1:
	set	instr, %o5		! address of copy instructions
	sub	%o5, %i3, %o5		! jmp address relative to instr
	jmp	%o5
	nop
#endif	sun4m
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

/*
 * Zero a block of storage, returning an error code if we
 * take a kernel pagefault which cannot be resolved.
 * Returns errno value on pagefault error, 0 if all ok
 *
 * int
 * kzero(addr, count)
 *	caddr_t addr;
 *	u_int count;
 */
	ENTRY(kzero)
	save	%sp, -SA(MINFRAME), %sp	! get another window
	sethi	%hi(zeroerr), %i2
	b	do_zero
	or	%i2, %lo(zeroerr), %i2

/*
 * We got here because of a fault during kzero.
 * Errno value is in %g1.
 */
zeroerr:
	sethi	%hi(_uunix), %g5
	ld	[%g5 + %lo(_uunix)], %g5
	st	%i5, [%g5+U_LOFAULT]	! restore old u.u_lofault
	ret
	restore	%g1, 0, %o0

/*
 * Zero a block of storage.
 *
 * bzero(addr, length)
 *	caddr_t addr;
 *	u_int count;
 */
	ENTRY2(bzero,blkclr)

	save	%sp, -SA(MINFRAME), %sp	! get another window
	mov	0, %i2
do_zero:
	sethi	%hi(_uunix), %g5
	ld	[%g5 + %lo(_uunix)], %g5
	ld	[%g5+U_LOFAULT], %i5	! save u.u_lofault
	cmp	%i1, 15			! check for small counts
	bl	byteclr			! just clear bytes
	st	%i2, [%g5+U_LOFAULT]	! install new vector

	!
	! Check for word alignment.
	!
	btst	3, %i0
	bz	bzero_probe
	mov	0x100, %i3		! constant size of main loop
	!
	!
	! clear bytes until word aligned
	!
1:	clrb	[%i0]
	add	%i0, 1, %i0
	btst	3, %i0
	bnz	1b
	sub	%i1, 1, %i1
	!
	! Word aligned.
bzero_probe:
#ifdef VME_D16
	! Check page type to see if word/double access is 
	! allowed (i.e. not VME_D16|VME_D32)
	! This assumes that the destination will not change to 
	! VME_DXX during the bzero. This will get a data fault with
	! be_vmeserr set if unsuccessful. 
	!
	ldub	[%i0],%g0		! make sure page is in memory
	lda	[%i0]ASI_PM, %g1	! get src's pte
	srl	%g1, PGT_SHIFT, %g1	! shift and mask for vme
	andcc	%g1, VME_D32|VME_D16, %g1
	bz	bzero_obmem		! not vme, then do obio
	!
	! check which vme space and jump accordingly
	!
	cmp	%g1, VME_D16
	be	bzero_vme16
	nop
	b,a	bzero_vme32
#endif VME_D16
#ifdef SUN4M_690
	and	%i0, MMU_PAGEMASK, %g1
	or	%g1, FT_ALL<<8, %g1
	lda	[%g1]ASI_PM, %i2	! get src's pte
	and	%i2, 3, %i2
	cmp	%i2, MMU_ET_PTE
	bne,a	1f			! if not mapped in,
	ldub	[%i0], %g0		!   force target page into memory
1:
	lda	[%g1]ASI_PM, %i2	! get target PTE again.
	srl	%i2, 28, %i2		! convert pte to space

	! if target is vme D16, limit transfer width
	! to 16 bits. (space=0xA or 0xC)
	cmp	%i2, 0xA		! from user vme-d16
	be	bzero_vme16
	cmp	%i2, 0xC		! from supv vme-d16
	be	bzero_vme16

	! if target is vme D32, limit transfer width
	! to 32 bits. (space=0xB or 0xD)
	cmp	%i2, 0xB		! from user vme-d32
	be	bzero_vme32
	cmp	%i2, 0xD		! from supv vme-d32
	be	bzero_vme32
	nop
#endif

#ifdef	BCOPY_BUF
	!
	! HW version of routine.
	! obmem, if needed move a word to become double-word aligned.
	!
bzero_obmem:
	btst	7, %i0
	bz	bzero_bufchk		! is double aligned?
	clr	%g1			! clr g1 for second half of double %g0
	clr	[%i0]			! clr to double boundry
	sub	%i1, 4, %i1		! decrement count
	add	%i0, 4, %i0		! increment address
	/*
	 * The source and destination are now double-word aligned,
	 * see if the bcopy buffer is available.
	 */
bzero_bufchk:
	sethi	%hi(_bcopy_res), %l2
	ld	[%l2 + %lo(_bcopy_res)], %i2 ! is the bcopy buffer available
	tst	%i2
	bnz	bzero_nobuf
	cmp	%i1, BCOPY_LIMIT	! for small counts use software
	bl	bzero_nobuf
	!
	! set	_vac_linesize, %i4
	! ld	[%i4 + %lo(_vac_linesize)], %i4
	! sub	%i4, 1, %i3		! create line mask
	! andcc	%i0, %i3, %g0
	!
	mov 	32, %i4
	andcc	%i0, 0x1F, %g0		! check if cache-line aligned
1:	bz,a	bzero_buf
	ldstub	[%l2 + %lo(_bcopy_res)], %i2 ! try to grab bcopy buffer
	std	%g0, [%i0]
	add	%i0, 8, %i0
	sub	%i1, 8, %i1		! decrement count
	b	1b
	andcc	%i0, 0x1F, %g0		! check if aligned

bzero_buf:
	tst	%i2
	bnz	bzero_nobuf		! give up, hardware in use 
	.empty				! the following set is ok in delay slot
	set	_zeros, %g2		
!
! Should only get here on sun4m when have viking/mxcc
! XXX--can play it safe and put in stuff to assure _zeros is page aligned
!
#ifdef	sun4m
!
! spl here because dont want phys pages to be stolen from us until
! we have completed the copy.
!
	call	_splhigh		! Watch register usage on calls!
	ldub	[%g2], %g0		! Set R bit in src page
	mov	%o0, %l7		! must save if make any other calls
3:
	or	%g2, FT_ALL<<8, %l1	! _zeros should be page aligned
	lda	[%l1]ASI_PM, %l1	! get phys addr
	and	%l1, 3, %i3
	cmp	%i3, MMU_ET_PTE
	bne,a	3b			! if not mapped in,
	ldub	[%g2], %g0		! force target page into memory
					! know that space is obmem here
	set	MXCC_STRM_SRC, %l5	! addr of stream source reg
	mov	0x10, %l0		! set cacheable bit in upper word
	srl	%l1, 8, %l1		! get rid of non-page bits
	sll	%l1, MMU_PAGESHIFT, %l1	! now have page aligned phys addr
	stda	%l0, [%l5]ASI_MXCC	! have finished stream read of zeros
	set	MXCC_STRM_DEST, %l5	! addr of stream destination reg

	stb	%g0, [%i0]		! Set RM bits in dest page
3:
	and	%i0, MMU_PAGEMASK, %l1
	or	%l1, FT_ALL<<8, %l1
	lda	[%l1]ASI_PM, %l1	! get phys addr
	and	%l1, 3, %i3
	cmp	%i3, MMU_ET_PTE
	bne,a	3b			! if not mapped in,
	stb	%g0, [%i0]		! force target page into memory

	srl	%l1, 28, %i3		! check space
	tst	%i3
	bz	1f			! Obmem page
	btst	PTE_CACHEABLEMASK, %l1	! Double check, page marked cacheable?
	bz,a	2f			! Not obmem, and not cacheable
	mov	%i3, %l0

	b	2f			! Not obmem, but cacheable
	or	%l0, %i3, %l0
1:
	bnz	2f			! Yes, obmem and cacheable, go ahead.
	nop
	mov	%g0, %l0		! No, clear cacheable bit.
2:
	srl	%l1, 8, %l1		! remove non-page bits from pte
	sll	%l1, MMU_PAGESHIFT, %l1	! phys page minus space bits
	and	%i0, MMU_PAGEOFFSET, %i3	! page offset via virt addr
	or	%l1, %i3, %l1		! phys addr complete

	add	%i3, %i1, %i2		! will we cross page boundary?
	set	MMU_PAGESIZE, %i4	! use for pagesize boundary check
	cmp	%i2, %i4		! Yes, if %i3+%i1 > PAGESIZE
	ble,a	hw_fastbzero		! No.  Can copy all bytes and exit.
	mov	%i1, %i4		! %i1 has the requested transfer size

					! if yes, loop on page copies
	sub	%i4, %i3, %i4		! round transfer to PAGESIZE
	b	hw_pagezero		! copy bytes in leading page
	sub	%i1, %i4, %i1		! decrease by what we will copy

hw_bzeroloop:
	mov	%l7, %o0
	call	_splx
	mov	0x10, %l0

	set	BCPY_BLKSZ, %i4
	cmp	%i1, %i4
	bl	9f
	nop

	call	_splhigh
	stb	%g0, [%i0]		! set RM bits on dest page
	mov	%o0, %l7		! save return value of splhigh
3:
	mov	%i0, %l1
	or	%l1, FT_ALL<<8, %l1
	lda	[%l1]ASI_PM, %l1	! get phys addr
	and	%l1, 3, %i3
	cmp	%i3, MMU_ET_PTE
	bne,a	3b			! if not mapped in,
	stb	%g0, [%i0]		! force target page into memory
	
	srl	%l1, 28, %i3		! check space
	tst	%i3
	bz	1f			! Obmem page
	btst	PTE_CACHEABLEMASK, %l1	! Double check, page marked cacheable?
	bz,a	2f			! Not obmem, and not cacheable
	mov	%i3, %l0

	b	2f			! Not obmem, but cacheable
	or	%l0, %i3, %l0
1:
	bnz	2f			! Yes, obmem and cacheable, go ahead.
	nop
	mov	%g0, %l0		! No, clear cacheable bit.
2:
	srl	%l1, 8, %l1		! remove non-page bits from pte
	sll	%l1, MMU_PAGESHIFT, %l1	! phys page minus space bits
					!    We should be on page boundary
	set	MMU_PAGESIZE, %i4
	cmp	%i1, %i4		! Do we have more than one page left?
	bg,a	hw_pagezero		! Yes, call hw_pagezero to loop
	sub	%i1, %i4, %i1		! decrease by what we will copy
					!    Always copy PAGESIZE here
	b	hw_fastbzero		! No, can call hw_fastbzero to exit
	mov	%i1, %i4		! End of transfer <= PAGESIZE

hw_fastbzero:				! use for < PAGESIZE copies.
	stda	%l0, [%l5]ASI_MXCC
	dec	BCPY_BLKSZ, %i4		! decrement count
	inc	BCPY_BLKSZ, %i0		! increment address
	cmp	%i4, BCPY_BLKSZ		! still got more than BCPY_BLKSZ to go?
	bl,a	hw_bzeroexit		! No, done with HW
	mov	%i4, %i1		! what we need byteclr to finish up
	b	hw_fastbzero		! loop until finished
	inc	BCPY_BLKSZ, %l1		! increment to next sublock

hw_pagezero:
	stda	%l0, [%l5]ASI_MXCC
	deccc	BCPY_BLKSZ, %i4		! decrement count
	bz	hw_bzeroloop		! done
	inc	BCPY_BLKSZ, %i0		! increment virtual address
	b	hw_pagezero		! more to do
	inc	BCPY_BLKSZ, %l1		! increment physaddr one sublock

hw_bzeroexit:
	mov	%l7, %o0
	call	_splx
	.empty				! Silence complaint about label
!
! Check Error Register to see if AE bit set, indicating problem with
! stream operation we just did.  Fatal, so we go down if a problem
! is found.
!
9:
	sethi	%hi(_bzero_cnt), %i2	! count # times used buffer

	set	MXCC_ERROR, %l7		! Load addr of MXCC err reg
	ldda	[%l7]ASI_MXCC, %l4	! %l5 hold bits 0-31 of paddr
	set	MXCC_ERR_AE, %i4
	btst	%i4, %l4		! %l4 hold the status bits
	bz	1f			! if not set continue exiting
	ld	[%i2 + %lo(_bzero_cnt)], %i4

	set	MXCC_ERR_EV, %o4	! If error bit set....
	btst	%o4, %l4		!    check if valid bit set
	bnz	6f
	.empty				! Silence complaint about label
1:
	stb	%g0, [%l2 + %lo(_bcopy_res)]	! interlock, unlock bcopy buffer
	inc	%i4
	b	byteclr
	st	%i4, [%i2 + %lo(_bzero_cnt)]
6:
	set	0f, %o0
	call	_panic
	nop
0:
	.asciz	"bzero stream operation failed"
	.align	4

#else	sun4m

	lda	[%g2]ASI_BC, %g0	! load bcopy buffer with zeros
3:	sta	%g0, [%i0]ASI_BC	! zero a line
	sub	%i1, %i4, %i1		! decrement count
	cmp	%i1, %i4		! check if done
	bge	3b			! loop until done
	add	%i0, %i4, %i0		! increment address

	sethi	%hi(_bzero_cnt), %g2	! count # times used buffer
	ld	[%g2 + %lo(_bzero_cnt)], %i4
	stb	%g0, [%l2 + %lo(_bcopy_res)]	! interlock, unlock bcopy buffer
	inc	%i4
	b	byteclr
	st	%i4, [%g2 + %lo(_bzero_cnt)]

#endif	sun4m
	!
	! Software version.
	! obmem, if needed move a word to become double-word aligned.
	!
bzero_swobmem:
	! We do the btst in the delay slot on branch here.
#else	BCOPY_BUF

bzero_obmem:
	btst	7, %i0			! is double aligned?
#endif	BCOPY_BUF
	bz	bzero_nobuf
	clr	%g1			! clr g1 for second half of double %g0
	clr	[%i0]			! clr to double boundry
	sub	%i1, 4, %i1		! decrement count
	b	bzero_nobuf
	add	%i0, 4, %i0		! increment address

!!!	std	%g0, [%i0+0xf8]		! done below in delay slot

bzero_blk:

	std	%g0, [%i0+0xf0]
	std	%g0, [%i0+0xe8]
	std	%g0, [%i0+0xe0]
	std	%g0, [%i0+0xd8]
	std	%g0, [%i0+0xd0]
	std	%g0, [%i0+0xc8]
	std	%g0, [%i0+0xc0]
	std	%g0, [%i0+0xb8]
	std	%g0, [%i0+0xb0]
	std	%g0, [%i0+0xa8]
	std	%g0, [%i0+0xa0]
	std	%g0, [%i0+0x98]
	std	%g0, [%i0+0x90]
	std	%g0, [%i0+0x88]
	std	%g0, [%i0+0x80]
	std	%g0, [%i0+0x78]
	std	%g0, [%i0+0x70]
	std	%g0, [%i0+0x68]
	std	%g0, [%i0+0x60]
	std	%g0, [%i0+0x58]
	std	%g0, [%i0+0x50]
	std	%g0, [%i0+0x48]
	std	%g0, [%i0+0x40]
	std	%g0, [%i0+0x38]
	std	%g0, [%i0+0x30]
	std	%g0, [%i0+0x28]
	std	%g0, [%i0+0x20]
	std	%g0, [%i0+0x18]
	std	%g0, [%i0+0x10]
	std	%g0, [%i0+0x08]
	std	%g0, [%i0+0x00]

zinst:
	add	%i0, %i3, %i0		! increment source address
	sub	%i1, %i3, %i1		! decrement count
bzero_nobuf:
	cmp	%i1, 0x100		! can we do whole chunk?
	bge,a	bzero_blk
	std	%g0, [%i0+0xf8]		! do first double of chunk

	cmp	%i1, 7			! can we zero any more double words
	ble	byteclr			! too small, go zero bytes
	andn	%i1, 7, %i3		! %i3 bytes left, double-word aligned
	srl	%i3, 1, %i2		! using doubles, need 1 instr / 2 words
	set	zinst, %i4		! address of clr instructions
	sub	%i4, %i2, %i4		! jmp address relative to instr
	jmp	%i4
	nop
	!
	! do leftover bytes
	!
3:
	add	%i0, 1, %i0		! increment address
byteclr:
	subcc	%i1, 1, %i1		! decrement count
	bge,a	3b
	clrb	[%i0]			! zero a byte

	sethi	%hi(_uunix), %g5	! XXX - global u register?
	ld	[%g5 + %lo(_uunix)], %g5
	st	%i5, [%g5+U_LOFAULT]	! restore old u.u_lofault
	ret
	restore	%g1, 0, %o0

#if defined(VME_D16) || defined(SUN4M_690)
/*
 * bcopy_vme16(from, to, count)
 * Block copy to/from a 16 bit vme device.
 * There is little optimization because the VME is so slow.
 */
bcopy_vme16:
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
	sub	%i0, %i1, %i0		! i0 gets the difference
2:
	lduh	[%i0+%i1], %i4		! read from address
	sth	%i4, [%i1]		! write at destination address
	subcc	%i3, 2, %i3		! dec count
	bg	2b
	add	%i1, 2, %i1		! delay slot, inc to address

	b,a	dbytecp			! copy remaining bytes, if any

/*
 * bzero_vme16(addr, length)
 * Zero a block of VME_D16 memory.
 * There is little optimization because the VME is so slow.
 * We come here word aligned
 */
bzero_vme16:
        andn    %i1, 1, %i3             ! count of aligned shorts to clear
	and	%i1, 1, %i1		! unaligned leftover count
	mov	%i3, %i4
1:	subcc	%i3, 2, %i3		! decrement count
	bg	1b
	clrh	[%i0+%i3]		! zero short

	b	byteclr			! zero remaining bytes, if any
	add	%i0, %i4, %i0		! increment address

/*
 * bzero_vme32(addr, length)
 * Zero a block of VME_D32 memory.
 * There is little optimization because the VME is so slow.
 * We come here word aligned
 */
bzero_vme32:
        andn    %i1, 3, %i3             ! count of aligned words to clear
        and     %i1, 3, %i1             ! unaligned leftover byte count
	mov	%i3, %i4
1:
        subcc   %i3, 4, %i3             ! decrement count
        bg      1b
	clr	[%i0+%i3]		! zero word
 
        b	byteclr			! zero remaining bytes, if any
        add	%i0, %i4, %i0		! increment address
#endif VME_D16 || defined(SUN4M_690)

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
	mov	%o2, %o4		! save original count
	tst	%o2
	bg,a	1f
	sub	%o0, %o1, %o0		! o0 gets the difference of src and dst

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
	stb	%g1, [%o1]		! store byte
	tst	%g1			! null byte?
	bnz	1f
	add	%o1, 1, %o1		! incr dst addr

	b	cs_out			! last byte in string
	mov	0, %o0			! ret code = 0
1:
	subcc	%o2, 1, %o2		! test count
	bge,a	0b
	ldub	[%o0+%o1], %g1		! delay slot, get source byte

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
 * Errno value is in %g1.
 */
copyioerr:
	sethi	%hi(_uunix), %g5
	ld	[%g5 + %lo(_uunix)], %g5
	st	%l6, [%g5+U_LOFAULT]	! restore old u.u_lofault
	ret
	restore	%g1, 0, %o0

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
	sethi	%hi(_uunix), %g5
	ld	[%g5 + %lo(_uunix)], %g5
	call	_copystr
	st	%g1, [%g5+U_LOFAULT]	! catch faults

	jmp	%o5 + 8			! return (results of copystr)
	clr	[%g5+U_LOFAULT]		! clear fault catcher

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
	sethi	%hi(_uunix), %g5	! XXX - global u register?
	ld	[%g5 + %lo(_uunix)], %g5
	jmp	%o5 + 8			! return (EFAULT)
	clr	[%g5+U_LOFAULT]

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
	sethi	%hi(_uunix), %g5
	ld	[%g5 + %lo(_uunix)], %g5
	st	%o3, [%g5+U_LOFAULT]
	ld	[%o0], %o0		! get the word
	retl
	clr	[%g5+U_LOFAULT]		! clear u.u_lofault

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
	sethi	%hi(_uunix), %g5
	ld	[%g5 + %lo(_uunix)], %g5
	st	%o3, [%g5+U_LOFAULT]
	ldub	[%o0], %o0		! get the byte
	retl
	clr	[%g5+U_LOFAULT]		! clear u.u_lofault

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
	sethi	%hi(_uunix), %g5
	ld	[%g5 + %lo(_uunix)], %g5
	st	%o3, [%g5+U_LOFAULT]
	b	suret
	st	%o1, [%o0]		! set the word

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
	sethi	%hi(_uunix), %g5
	ld	[%g5 + %lo(_uunix)], %g5
	st	%o3, [%g5+U_LOFAULT]
	stb	%o1, [%o0]		! set the byte
suret:
	mov	0, %o0			! indicate success
	retl
	clr	[%g5+U_LOFAULT]		! clear u.u_lofault

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
	sethi	%hi(_uunix), %g5
	ld	[%g5 + %lo(_uunix)], %g5
	st	%o3, [%g5+U_LOFAULT]
	lduh	[%o0], %o0		! get the half word
	retl
	clr	[%g5+U_LOFAULT]		! clear u.u_lofault

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
	sethi	%hi(_uunix), %g5
	ld	[%g5 + %lo(_uunix)], %g5
	st	%o3, [%g5+U_LOFAULT]
	b	suret
	sth	%o1, [%o0]		! set the half word

fsuerr:
	mov	-1, %o0			! return error
	sethi	%hi(_uunix), %g5
	ld	[%g5 + %lo(_uunix)], %g5
	retl
	clr	[%g5+U_LOFAULT]		! clear u.u_lofault
