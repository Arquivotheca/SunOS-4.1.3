/*
 *	@(#)module_ross_asm.s 1.1 92/07/30 SMI
 *	Copyright (c) 1990 by Sun Microsystems, Inc.
 *
 * assembly code support for modules based on the
 * Cypress CY7C604 or CY7C605 Cache Controller and
 * Memory Management Units.
 */

#define	ROSS_NOASI

#include <sys/param.h>
#include <machine/asm_linkage.h>
#include <machine/mmu.h>
#include <machine/pte.h>
#include <machine/cpu.h>
#include <machine/psl.h>
#include <machine/trap.h>
#include <machine/devaddr.h>
#include <machine/async.h>
#include <percpu_def.h>
#include "assym.s"

	!
	! This approach is based upon the latest errata 
	! (received 5/8/89) for "Sun-4M SYSTEM ARCHITECTURE"
	! which specifies that for Ross modules, in order to
	! fix a race condition, the AFSR must be read first,
	! and then the AFAR must only be read if the AFV bit 
	! of the AFSR is set.  Furthermore, the AFAR is
	! guaranteed not to go away until *it* is read.
	! Note that generic SRMMU behaves differently, in
	! that the AFAR must be read first, since reading
	! the AFSR unlocks these registers.
	ALTENTRY(ross_mmu_getasyncflt)
	set	RMMU_AFS_REG, %o1
	lda	[%o1]ASI_MOD, %o1
	st	%o1, [%o0]
	btst	AFSREG_AFV, %o1
	bz	1f
	set	RMMU_AFA_REG, %o1
	lda	[%o1]ASI_MOD, %o1
	st	%o1, [%o0+4]
1:
	set	-1, %o1
	retl
	st	%o1, [%o0+8]

/*
 * ross_pte_rmw: update a pte.
 * does no flushing.
 */
	ALTENTRY(ross_pte_rmw)		! (ppte, aval, oval)
	mov	%psr, %g2
	andn	%g2, PSR_ET, %g1
	mov	%g1, %psr		! disable traps
	nop; nop; nop			! psr delay
	ld	[%o0], %o5		! get old value
	andn	%o5, %o1, %o5		! turn some bits off
	or	%o5, %o2, %o5		! turn some bits on
	swap	[%o0], %o5		! update the PTE
	mov	%g2, %psr		! restore traps
	nop				! psr delay
	retl
	mov	%o5, %o0

#ifdef	VAC
/*
 * Virtual Address Cache routines.
 *
 *	Standard register allocation:
 *
 * %g1	scratchpad / jump vector / alt ctx reg ptr
 * %o0	virtual address			ctxflush puts context here
 * %o1	incoming size / loop counter
 * %o2	context number to borrow
 * %o3	saved context number
 * %o4	srmmu ctx reg ptr
 * %o5	saved psr
 * %o6	reserved (stack pointer)
 * %o7	reserved (return address)
 */

#define	CACHE_LINES	2048

#define	CACHE_LINESHFT	5
#define	CACHE_LINESZ	(1<<CACHE_LINESHFT)
#define	CACHE_LINEMASK	(CACHE_LINESZ-1)

#define	CACHE_BYTES	(CACHE_LINES<<CACHE_LINESHFT)

#define	MPTAG_OFFSET	0x40000

	ALTENTRY(ross_vac_init_asm)	! (clrbits, setbits)
	set	RMMU_CTL_REG, %o5
	lda	[%o5]ASI_MOD, %o4	! read control reg
	andn	%o4, %o0, %o4		! turn some bits off
	or	%o4, %o1, %o4		! turn some bits on
	sta	%o4, [%o5]ASI_MOD	! update control reg

	set	CACHE_BYTES, %o1	! cache bytes to init
	set	MPTAG_OFFSET, %o2	! offset for MPTAGs
1:
	deccc	CACHE_LINESZ, %o1
	sta	%g0, [%o1 + %o2]0xE	! clear MPTAG
	bne	1b
	sta	%g0, [%o1]0xE		! clear PVTAG

	retl
	nop


! The easiest ASI-based flush that gets rid of
! all the supervisor data takes one pass for
! each region (yecch). So, don`t use ASI based
! flushes for this one.

! This routine is used to flush data out to memory,
! but it is OK for the data to be left in the cache,
! so we only have to flush dirty lines. And, for
! those lines, we leave a hunk of kernel text in
! the cache in their place. No choice ...

#define	TAG_OFFSET	0x40000		/* Cy7C605 MPtag offset */
#define	VM_BITS		0x60		/* C77C605 MPtag M and V bits */

	ALTENTRY(ross_vac_flushall)
	set	CACHE_BYTES, %o1		! look at whole cache
	set	TAG_OFFSET, %o2			! MPtag ofset
	set	KERNELBASE+CACHE_BYTES, %o5	! base of safe area to read
	b	2f				! enter loop at bottom test
	nop
1:				! top of loop: compare, flush
	andcc	%o4, VM_BITS, %o3		! test valid and modified
	cmp	%o3, VM_BITS			! are both on?
	beq,a	2f				! if we matched,
	ld	[%o5+%o1], %g0			!   force cache line replace
2:				! end of loop: again?
	deccc	CACHE_LINESZ, %o1		! any more to do?
	bcc,a	1b				! if so, loop back
	lda	[%o2+%o1]ASI_DCT, %o4		!   read corresponding MPtag

	retl
	nop


! ross_vac_usrflush: flush all user data from the cache

	ALTENTRY(ross_vac_usrflush)

	set	CACHE_BYTES/8, %g1
	mov	%g0, %o0
	add	%o0, %g1, %o1
	add	%o1, %g1, %o2
	add	%o2, %g1, %o3
	add	%o3, %g1, %o4
	add	%o4, %g1, %o5
	add	%o5, %g1, %g2
	add	%g2, %g1, %g3

1:	deccc	CACHE_LINESZ, %g1
	sta	%g0, [%o0 + %g1]ASI_FCU
	sta	%g0, [%o1 + %g1]ASI_FCU
	sta	%g0, [%o2 + %g1]ASI_FCU
	sta	%g0, [%o3 + %g1]ASI_FCU
	sta	%g0, [%o4 + %g1]ASI_FCU
	sta	%g0, [%o5 + %g1]ASI_FCU
	sta	%g0, [%g2 + %g1]ASI_FCU
	bne	1b
	sta	%g0, [%g3 + %g1]ASI_FCU

	retl
	nop

/*
 * BORROW_CONTEXT: temporarily set the context number
 * to that given in %o2. See above for register assignments.
 * NOTE: all interrupts below level 15 are disabled until
 * the next RESTORE_CONTEXT. Do we want to disable traps
 * entirely, to prevent L15s from being serviced in a borrowed
 * context number? (trying this out now)
 */

#define	BORROW_CONTEXT			\
	mov	%psr, %g7;		\
	andn	%g7, PSR_ET, %g6;	\
	mov	%g6, %psr;		\
	nop ;	nop;			\
\
	set	RMMU_CTP_REG, %g6;	\
	lda	[%g6]ASI_MOD, %g6;	\
	sll	%o2, 2, %g5;		\
	sll	%g6, 4, %g6;		\
	add	%g6, %g5, %g6;		\
	lda	[%g6]ASI_MEM, %g6;	\
	and	%g6, 3, %g6;		\
	subcc	%g6, MMU_ET_PTP, %g0;	\
\
	set	RMMU_CTX_REG, %g6;	\
	bne	1f;			\
	lda	[%g6]ASI_MOD, %g5;	\
	sta	%o2, [%g6]ASI_MOD;	\
1:
/*
 * RESTORE_CONTEXT: back out from whatever BORROW_CONTEXT did.
 * NOTE: asssumes two cycles of PSR DELAY follow the macro;
 * currently, all uses are followed by "retl ; nop".
 */
#define	RESTORE_CONTEXT			\
	sta	%g5, [%g6]ASI_MOD;	\
	mov	%g7, %psr;		\
	nop

	ALTENTRY(ross_vac_ctxflush)

	BORROW_CONTEXT

	set	CACHE_BYTES/8, %g1
	mov	%g0, %o0
	add	%o0, %g1, %o1
	add	%o1, %g1, %o2
	add	%o2, %g1, %o3
	add	%o3, %g1, %o4
	add	%o4, %g1, %o5
	add	%o5, %g1, %g2
	add	%g2, %g1, %g3

1:	deccc	CACHE_LINESZ, %g1
	sta	%g0, [%o0 + %g1]ASI_FCC
	sta	%g0, [%o1 + %g1]ASI_FCC
	sta	%g0, [%o2 + %g1]ASI_FCC
	sta	%g0, [%o3 + %g1]ASI_FCC
	sta	%g0, [%o4 + %g1]ASI_FCC
	sta	%g0, [%o5 + %g1]ASI_FCC
	sta	%g0, [%g2 + %g1]ASI_FCC
	bne	1b
	sta	%g0, [%g3 + %g1]ASI_FCC

	RESTORE_CONTEXT

	retl
	nop

! void ross_vac_rgnflush(va)		flush data in this region from the cache.
#ifdef	MULTIPROCESSOR
!					context number in %o2
#endif	MULTIPROCESSOR

	ALTENTRY(ross_vac_rgnflush)

#ifdef	MULTIPROCESSOR
	BORROW_CONTEXT
#endif	MULTIPROCESSOR

	set	CACHE_BYTES/8, %g1
	mov	%g0, %o0
	add	%o0, %g1, %o1
	add	%o1, %g1, %o2
	add	%o2, %g1, %o3
	add	%o3, %g1, %o4
	add	%o4, %g1, %o5
	add	%o5, %g1, %g2
	add	%g2, %g1, %g3

1:	deccc	CACHE_LINESZ, %g1
	sta	%g0, [%o0 + %g1]ASI_FCR
	sta	%g0, [%o1 + %g1]ASI_FCR
	sta	%g0, [%o2 + %g1]ASI_FCR
	sta	%g0, [%o3 + %g1]ASI_FCR
	sta	%g0, [%o4 + %g1]ASI_FCR
	sta	%g0, [%o5 + %g1]ASI_FCR
	sta	%g0, [%g2 + %g1]ASI_FCR
	bne	1b
	sta	%g0, [%g3 + %g1]ASI_FCR

#ifdef	MULTIPROCESSOR
	RESTORE_CONTEXT
#endif	MULTIPROCESSOR

	retl
	nop

! void ross_vac_segflush(va)		flush data in this segment from the cache
#ifdef	MULTIPROCESSOR
!					context number in %o2
#endif	MULTIPROCESSOR

	ALTENTRY(ross_vac_segflush)
#ifdef	MULTIPROCESSOR
	BORROW_CONTEXT
#endif	MULTIPROCESSOR

	set	CACHE_BYTES/8, %g1
	mov	%g0, %o0
	add	%o0, %g1, %o1
	add	%o1, %g1, %o2
	add	%o2, %g1, %o3
	add	%o3, %g1, %o4
	add	%o4, %g1, %o5
	add	%o5, %g1, %g2
	add	%g2, %g1, %g3

1:	deccc	CACHE_LINESZ, %g1
	sta	%g0, [%o0 + %g1]ASI_FCS
	sta	%g0, [%o1 + %g1]ASI_FCS
	sta	%g0, [%o2 + %g1]ASI_FCS
	sta	%g0, [%o3 + %g1]ASI_FCS
	sta	%g0, [%o4 + %g1]ASI_FCS
	sta	%g0, [%o5 + %g1]ASI_FCS
	sta	%g0, [%g2 + %g1]ASI_FCS
	bne	1b
	sta	%g0, [%g3 + %g1]ASI_FCS

#ifdef	MULTIPROCESSOR
	RESTORE_CONTEXT
#endif	MULTIPROCESSOR
	retl
	nop

! void ross_vac_pageflush(va)		flush data in this page from the cache.
#ifdef	MULTIPROCESSOR
!					context number in %o2
#endif

	ALTENTRY(ross_vac_pageflush)

#ifdef	MULTIPROCESSOR
	BORROW_CONTEXT
#endif	MULTIPROCESSOR

	set	PAGESIZE/8, %g1
	add	%o0, %g1, %o1
	add	%o1, %g1, %o2
	add	%o2, %g1, %o3
	add	%o3, %g1, %o4
	add	%o4, %g1, %o5
	add	%o5, %g1, %g2
	add	%g2, %g1, %g3

1:	deccc	CACHE_LINESZ, %g1
	sta	%g0, [%o0 + %g1]ASI_FCP
	sta	%g0, [%o1 + %g1]ASI_FCP
	sta	%g0, [%o2 + %g1]ASI_FCP
	sta	%g0, [%o3 + %g1]ASI_FCP
	sta	%g0, [%o4 + %g1]ASI_FCP
	sta	%g0, [%o5 + %g1]ASI_FCP
	sta	%g0, [%g2 + %g1]ASI_FCP
	bne	1b
	sta	%g0, [%g3 + %g1]ASI_FCP

#ifdef	MULTIPROCESSOR
	RESTORE_CONTEXT
#endif	MULTIPROCESSOR

	retl
	nop

! void ross_vac_pagectxflush(va, ctx)	flush data in this page and ctx from the cache.

	ALTENTRY(ross_vac_pagectxflush)
	set	PAGESIZE, %o1

	BORROW_CONTEXT

	set	PAGESIZE/8, %g1
	add	%o0, %g1, %o1
	add	%o1, %g1, %o2
	add	%o2, %g1, %o3
	add	%o3, %g1, %o4
	add	%o4, %g1, %o5
	add	%o5, %g1, %g2
	add	%g2, %g1, %g3

1:	deccc	CACHE_LINESZ, %g1
	sta	%g0, [%o0 + %g1]ASI_FCP
	sta	%g0, [%o1 + %g1]ASI_FCP
	sta	%g0, [%o2 + %g1]ASI_FCP
	sta	%g0, [%o3 + %g1]ASI_FCP
	sta	%g0, [%o4 + %g1]ASI_FCP
	sta	%g0, [%o5 + %g1]ASI_FCP
	sta	%g0, [%g2 + %g1]ASI_FCP
	bne	1b
	sta	%g0, [%g3 + %g1]ASI_FCP

	RESTORE_CONTEXT

	retl
	nop

! void ross_vac_flush(va, sz)		flush data in this range from the cache
#ifdef	MULTIPROCESSOR
!					context number in %o2
#endif	MULTIPROCESSOR

	ALTENTRY(ross_vac_flush)
	and	%o0, CACHE_LINEMASK, %g1		! figure align error on start
	sub	%o0, %g1, %o0			! push start back that much
	add	%o1, %g1, %o1			! add it to size

#ifdef	MULTIPROCESSOR
	BORROW_CONTEXT
#endif	MULTIPROCESSOR

1:	deccc	CACHE_LINESZ, %o1
	sta	%g0, [%o0]ASI_FCP
	bcc	1b
	inc	CACHE_LINESZ, %o0

#ifdef	MULTIPROCESSOR
	RESTORE_CONTEXT
#endif	MULTIPROCESSOR

	retl
	nop

	ALTENTRY(ross_cache_on)
	set	0x100, %o2			! cache enable
	set	RMMU_CTL_REG, %o0
	lda	[%o0]ASI_MOD, %o1
	or	%o1, %o2, %o1
	sta	%o1, [%o0]ASI_MOD
	retl
	nop

	ALTENTRY(check_cache)
	set	RMMU_CTL_REG, %o0
	retl
	lda	[%o0]ASI_MOD, %o0

#ifdef	ROSS_NOASI
/************************************************************************
 *	ross_noasi routines
 *
 *	flush the cache by replacing the specified lines
 *	with data from elsewhere in memory.
 */

! void ross_noasi_vac_usrflush(va)		flush user data from the cache.

	ALTENTRY(ross_noasi_vac_usrflush)
	set	8, %o3				! match if VALID & !SUPV
	set	10, %o2				! check SUPV and VALID bits
	set	CACHE_BYTES, %o1		! look at whole cache
	set	0, %o0				! starting at zero offset
	set	0, %g2				! no secondary match
	set	1, %g3				! no secondary match
	b	ross_noasi_vac_flushcommon
	nop

! void ross_noasi_vac_ctxflush(ctx)		flush data in this context from the cache.

	ALTENTRY(ross_noasi_vac_ctxflush)
	sll	%o0, 4, %o3			! ctx as specified
	or	%o3, 8, %o3			! valid must be set
	set	0x0000FFF8, %o2			! check CTX, VALID
	set	CACHE_BYTES, %o1		! look at whole cache
	set	0, %o0				! starting at zero offset
	set	10, %g2				! also check VALID, SUPV
	set	10, %g3				! match if VALID && SUPV
	b	ross_noasi_vac_flushcommon
	nop

! void ross_noasi_vac_rgnflush(va)		flush data in this region from the cache.

	ALTENTRY(ross_noasi_vac_rgnflush)
	set	0xFF000000, %o5
	and	%o5, %o0, %o5			! extract virtual tag
#ifndef	MULTIPROCESSOR
	set	RMMU_CTX_REG, %o2
	lda	[%o2]ASI_MOD, %o2		! get context number
#endif	MULTIPROCESSOR
	sll	%o2, 4, %o3			! CTX from CTXREG, or passed
	or	%o3, 8, %o3			! valid must be set
	or	%o3, %o5, %o3			! RGN as specified
	set	0xFF00FFF8, %o2			! chk REGION, CTX, VALID
	set	CACHE_BYTES, %o1		! look at whole cache
	set	0, %o0				! starting at zero offset
	set	10, %g2				! also check VALID, SUPV
	set	10, %g3				! match if VALID && SUPV
	b	ross_noasi_vac_flushcommon
	nop


! void ross_noasi_vac_segflush(va)		flush data in this segment from the cache

	ALTENTRY(ross_noasi_vac_segflush)
	set	0xFFFC0000, %o5
	and	%o5, %o0, %o5			! extract virtual tag
	srl	%o0, 18, %o0			! round address to
	sll	%o0, 18, %o0			! base of segment
#ifndef	MULTIPROCESSOR
	set	RMMU_CTX_REG, %o2
	lda	[%o2]ASI_MOD, %o2		! get context number
#endif	MULTIPROCESSOR
	sll	%o2, 4, %o3			! CTX from CTXREG, or passed
	or	%o3, 8, %o3			! VALID must be set
	or	%o3, %o0, %o3			! and segment matches
	set	0xFFFCFFF8, %o2			! chk CTX, VALID, SEG
	set	CACHE_BYTES, %o1		! look at whole cache
	set	0, %o0				! starting at zero offset
	set	10, %g2				! also check VALID, SUPV
	set	10, %g3				! match if VALID && SUPV
	b	ross_noasi_vac_flushcommon
	nop

! void ross_noasi_vac_pageflush(va)		flush data in this page from the cache.

	ALTENTRY(ross_noasi_vac_pageflush)
#ifndef	MULTIPROCESSOR
	set	RMMU_CTX_REG, %o1
	lda	[%o1]ASI_MOD, %o1		! get context number
#else	MULTIPROCESSOR
	mov	%o2, %o1			! move context number into place
#endif	MULTIPROCESSOR
	b	_ross_noasi_vac_pagectxflush
	nop

! void ross_noasi_vac_pagectxflush(va, ctx)	flush data in this page and ctx from the cache.

	ALTENTRY(ross_noasi_vac_pagectxflush)
	set	0xFFFF0000, %o5
	and	%o5, %o0, %o5			! extract virtual tag
	sll	%o1, 4, %o3			! CTX as specified
	or	%o3, 8, %o3			! VALID must be set
	or	%o3, %o5, %o3			! page must match
	set	0xFFFFFFF8, %o2			! check CTX, VALID, VTAG
	set	PAGESIZE, %o1			! look at one page
	set	0x0000F000, %g1
	and	%o0, %g1, %o0			! extract virtual index
	set	10, %g2				! also check VALID, SUPV
	set	10, %g3				! match if VALID && SUPV
	b	ross_noasi_vac_flushcommon
	nop


! void ross_noasi_vac_flush(va, sz)		flush data in this range from the cache

	ALTENTRY(ross_noasi_vac_flush)
	set	0xFFFF0000, %o5
	and	%o5, %o0, %o5			! extract virtual tag
#ifndef	MULTIPROCESSOR
	set	RMMU_CTX_REG, %o2
	lda	[%o2]ASI_MOD, %o2		! get context number
#endif	MULTIPROCESSOR
	sll	%o2, 4, %o3			! CTX as specified
	or	%o3, 8, %o3			! VALID must be set
	or	%o3, %o5, %o3			! virtual tag matches
	set	0xFFFFFFF8, %o2			! check tag, CTX, valid
	set	0xFFFF, %g1
	and	%o0, %g1, %o0			! extract virtual index
	and	%o0, CACHE_LINEMASK, %g1	! figure align error on start
	sub	%o0, %g1, %o0			! push start back that much
	add	%o1, %g1, %o1			! add it to size
	add	%o1, CACHE_LINEMASK, %o1	! round xfer size up
	andn	%o1, CACHE_LINEMASK, %o1	! to multiple of line size
	set	10, %g2				! also check VALID, SUPV
	set	10, %g3				! match if VALID && SUPV
	b	ross_noasi_vac_flushcommon
	nop
	
ross_noasi_vac_flushcommon:
	set	KERNELBASE+CACHE_BYTES, %o5	! base of safe area to read
	b	2f				! enter loop at bottom test
	add	%o5, %o0, %o5			!   offset to our line

1:				! top of loop: compare, flush
	and	%o4, %o2, %g1			! extract primary check bits
	cmp	%g1, %o3			! check vs primary check
	beq,a	2f				! if we matched,
	ld	[%o5+%o1], %g0			!   force cache line replace

	and	%o4, %g2, %g1			! extract secondary check bits
	cmp	%g1, %g3			! check vs secondary check
	beq,a	2f				! if we matched,
	ld	[%o5+%o1], %g0			!   force cache line replace
2:				! end of loop: again?
	deccc	CACHE_LINESZ, %o1		! any more to do?
	bcc,a	1b				! if so, loop back
	lda	[%o0+%o1]ASI_DCT, %o4		!   read corresponding PVtag

	retl
	nop
#endif	ROSS_NOASI

#endif	VAC
