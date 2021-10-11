/*	@(#)map.s 1.1 92/07/30 SMI	*/

/*
 *	Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Sun-4 MMU and Virtual Address Cache routines.
 */

#include <sys/param.h>
#include <machine/asm_linkage.h>
#include <machine/mmu.h>
#include <machine/pte.h>
#include <machine/enable.h>
#include <machine/cpu.h>
#include "assym.s"

	.seg	"text"
	.align	4

!
! Read the page map entry for the given address v
! and return it in a form suitable for software use.
!
! u_int
! map_getpgmap(v)
!	addr_t v;
!
	ENTRY(map_getpgmap)
	andn	%o0, 0x3, %o1		! align to word boundary
	retl
	lda	[%o1]ASI_PM, %o0	! read page map entry

!
! Set the pme for address v using the software pte given.
!
! map_setpgmap(v, pte)
!	addr_t v;
!	u_int pte;
!
	ENTRY(map_setpgmap)
	andn	%o0, 0x3, %o2		! align to word boundary
	retl
	sta	%o1, [%o2]ASI_PM	! write page map entry

!
! Return the 16 bit segment map entry for the given segment number.
!
! u_int
! map_getsgmap(v)
!	addr_t v;
!
	ENTRY(map_getsgmap)
	andn	%o0, 0x3, %o0		! align to word boundary
	lduha	[%o0]ASI_SM, %o0	! read segment number
	sethi	%hi(_segmask), %o2	! need to mask bits due to bug in cobra
	ld      [%o2 + %lo(_segmask)], %o2
	retl
	and     %o0, %o2, %o0          ! A HACK BECAUSE COBRA IS BROKEN

!
! Set the segment map entry for segno to pm.
!
! map_setsgmap(v, pm)
!	addr_t v;
!	u_int pm;
!
	ENTRY(map_setsgmap)
	andn	%o0, 0x3, %o0		! align to word boundary
	retl
	stha	%o1, [%o0]ASI_SM	! write segment entry

#ifdef MMU_3LEVEL
!
! Return the 16 bit region map entry for the given region number.
!
! u_int
! map_getrgnmap(v)
!	addr_t v;
!
	ENTRY(map_getrgnmap)
	andn	%o0, 0x1, %o0		! align to halfword boundary
	or	%o0, 0x2, %o0
	lduha	[%o0]ASI_RM, %o0	! read region number
	retl
	srl	%o0, 0x8, %o0
!
! Set the segment map entry for segno to pm.
!
! map_setrgnmap(v, pm)
!	addr_t v;
!	u_int pm;
!	 
	ENTRY(map_setrgnmap)
	andn	%o0, 0x1, %o0		! align to halfword boundary
	or	%o0, 0x2, %o0
	sll	%o1, 0x8, %o1
	retl
	stha	%o1, [%o0]ASI_RM	! write region entry
#endif /* MMU_3LEVEL */

!
! Return the current context number.
!
! u_int
! map_getctx()
!
	ENTRY(map_getctx)
	set	CONTEXT_REG, %o1
	retl
	lduba	[%o1]ASI_CTL, %o0	! read the context register
!
! Set the current context number.
!
! map_setctx(c)
!	u_int c;
!
	ENTRY(map_setctx)
	set	CONTEXT_REG, %o1
	retl
	stba	%o0, [%o1]ASI_CTL	! write the context register

#ifdef VAC
!
! Function to mark a page as noncachable
!
! void
! vac_dontcache(p)
!
! used by: (keep this up to date, so we can ditch this function later)
!
	ENTRY(vac_dontcache)
	andn    %o0, 0x3, %g1           ! align to word boundary
	lda     [%g1]ASI_PM, %o0        ! read old page map entry
	set     PG_NC, %g2
	or      %o0, %g2, %o0           ! turn on NOCACHE bit
	retl
	sta     %o0, [%g1]ASI_PM        ! and write it back out

!
! Initialize the cache by invalidating all the cache tags.
! It DOESN'T turn on cache enable bit in the enable register.
!
! void
! vac_tagsinit()
!
	ENTRY(vac_tagsinit)
	sethi	%hi(_vac_linesize), %o0	! linesize
	ld	[%o0 + %lo(_vac_linesize)], %o2
	sethi	%hi(_vac_size), %o1	! size of cache
	ld	[%o1 + %lo(_vac_size)], %o1
	set	CACHE_TAGS, %o0		! address of cache tags in CTL space
1:
	subcc	%o1, %o2, %o1		! done yet?
	bgu	1b
	sta	%g0, [%o0 + %o1]ASI_CTL	! write tags to zero
	retl
	.empty				! next instruction ok in delay slot
!
! Flush a context from the cache.
! To flush a context we must cycle through all lines of the
! cache issuing a store into alternate space command for each
! line whilst the context register remains constant.
! We'll start at the end and work backwards to use only
! one variable for the loop and test.
!
! void
! vac_ctxflush()
!
	ENTRY(vac_ctxflush)
	sethi	%hi(_vac), %g1		! check if cache is turned on
	ld	[%g1 + %lo(_vac)], %g1
	tst	%g1
	bnz,a	1f			! if cache not enabled, return
	save	%sp, -SA(MINFRAME), %sp	! get a new window

	retl
	nop
1:
	sethi	%hi(_vac_size), %l0	! cachesize / number of steps in loop
	ld	[%l0 + %lo(_vac_size)], %l0
	srl	%l0, 4, %l0
	sethi	%hi(_vac_linesize), %i5
	ld	[%i5 + %lo(_vac_linesize)], %i5
	sethi	%hi(_flush_cnt+FM_CTX), %i2 ! increment flush count
	ld	[%i2 + %lo(_flush_cnt+FM_CTX)], %i3
	inc	%i3
	call	_flush_user_windows	! make sure no windows are hanging out
	st	%i3, [%i2 + %lo(_flush_cnt+FM_CTX)]

	!
	! preload a bunch of offsets
	! Avoid going through the cache sequentially by flushing
	! 16 lines spread evenly through the cache.
	!
	sub	%l0, %i5, %i0		! cachesize/16 - linesize
	add	%l0, %l0, %l1		! cachesize/16*2
	add	%l1, %l0, %l2		! cachesize/16*3
	add	%l2, %l0, %l3		! ...
	add	%l3, %l0, %l4
	add	%l4, %l0, %l5
	add	%l5, %l0, %l6
	add	%l6, %l0, %l7
	add	%l7, %l0, %o0
	add	%o0, %l0, %o1
	add	%o1, %l0, %o2
	add	%o2, %l0, %o3
	add	%o3, %l0, %o4
	add	%o4, %l0, %o5
	add	%o5, %l0, %i4

	sta	%g0, [%i0]ASI_FCC
2:
	sta	%g0, [%i0 + %l0]ASI_FCC
	sta	%g0, [%i0 + %l1]ASI_FCC
	sta	%g0, [%i0 + %l2]ASI_FCC
	sta	%g0, [%i0 + %l3]ASI_FCC
	sta	%g0, [%i0 + %l4]ASI_FCC
	sta	%g0, [%i0 + %l5]ASI_FCC
	sta	%g0, [%i0 + %l6]ASI_FCC
	sta	%g0, [%i0 + %l7]ASI_FCC
	sta	%g0, [%i0 + %o0]ASI_FCC
	sta	%g0, [%i0 + %o1]ASI_FCC
	sta	%g0, [%i0 + %o2]ASI_FCC
	sta	%g0, [%i0 + %o3]ASI_FCC
	sta	%g0, [%i0 + %o4]ASI_FCC
	sta	%g0, [%i0 + %o5]ASI_FCC
	sta	%g0, [%i0 + %i4]ASI_FCC
	subcc	%i0, %i5, %i0			! generate next address
	bge,a	2b				! are we done yet?
	sta	%g0, [%i0]ASI_FCC

	ret
	restore

!
! Flush all non-supervisor lines from the cache.
! Cycle through all lines of the cache issuing a store into
! alternate space command for each line.
! We'll start at the end and work backwards to use only
! one variable for the loop and test.
!
! void
! vac_usrflush()
!
	ENTRY(vac_usrflush)
	sethi	%hi(_vac), %g1		! check if cache is turned on
	ld	[%g1 + %lo(_vac)], %g1
	tst	%g1
	bnz,a	1f			! if cache not enabled, return
	save	%sp, -SA(MINFRAME), %sp ! get a new window

	retl
	nop
1:
	sethi	%hi(_vac_size), %l0
	ld	[%l0 + %lo(_vac_size)], %l0
	sethi	%hi(_vac_linesize), %i5
	ld	[%i5 + %lo(_vac_linesize)], %i5
	sethi	%hi(_flush_cnt+FM_USR), %i2 ! increment flush count
	ld	[%i2 + %lo(_flush_cnt+FM_USR)], %i3
	srl	%l0, 4, %l0		! cachesize / 16 (# steps in loop)
	inc	%i3
	call	_flush_user_windows	! make sure no windows are hanging out
	st	%i3, [%i2 + %lo(_flush_cnt+FM_USR)]

	!
	! 
	!
	sethi	%hi(_vac_hashwusrflush), %g1
	ld	[%g1 + %lo(_vac_hashwusrflush)], %g1
	tst	%g1
	bz,a	vac_fake_usrflush
	mov	%i5, %i2

	!
	! preload a bunch of offsets
	! Avoid going through the cache sequentially by flushing
	! 16 lines spread evenly through the cache.
	!
	sub	%l0, %i5, %i0		! cachesize/16 - 16
	add	%l0, %l0, %l1		! cahcesize/16*2
	add	%l1, %l0, %l2		! cahcesize/16*3
	add	%l2, %l0, %l3		! ...
	add	%l3, %l0, %l4
	add	%l4, %l0, %l5
	add	%l5, %l0, %l6
	add	%l6, %l0, %l7
	add	%l7, %l0, %o0
	add	%o0, %l0, %o1
	add	%o1, %l0, %o2
	add	%o2, %l0, %o3
	add	%o3, %l0, %o4
	add	%o4, %l0, %o5
	add	%o5, %l0, %i4

	sta	%g0, [%i0]ASI_FCU
2:
	sta	%g0, [%i0 + %l0]ASI_FCU
	sta	%g0, [%i0 + %l1]ASI_FCU
	sta	%g0, [%i0 + %l2]ASI_FCU
	sta	%g0, [%i0 + %l3]ASI_FCU
	sta	%g0, [%i0 + %l4]ASI_FCU
	sta	%g0, [%i0 + %l5]ASI_FCU
	sta	%g0, [%i0 + %l6]ASI_FCU
	sta	%g0, [%i0 + %l7]ASI_FCU
	sta	%g0, [%i0 + %o0]ASI_FCU
	sta	%g0, [%i0 + %o1]ASI_FCU
	sta	%g0, [%i0 + %o2]ASI_FCU
	sta	%g0, [%i0 + %o3]ASI_FCU
	sta	%g0, [%i0 + %o4]ASI_FCU
	sta	%g0, [%i0 + %o5]ASI_FCU
	sta	%g0, [%i0 + %i4]ASI_FCU
	subcc	%i0, %i5, %i0			! generate next address
	bge,a	2b				! are we done yet?
	sta	%g0, [%i0]ASI_FCU
	ret
	restore

#ifdef MMU_3LEVEL
!
! Flush a region from the cache.
! To flush the argument region from the cache we hold the bits that
! specify the region in the address constant and issue a store into
! alternate space command for each line of the cache by incrementing
! the lower bits of the address by VAC_LINESIZE (cache line size).
!
! vac_rgnflush(v)
!	addr_t v;
!
	ENTRY(vac_rgnflush)
	sethi	%hi(_vac), %g1		! check if cache is turned on
	ld	[%g1 + %lo(_vac)], %g1
	tst	%g1
	bnz,a	1f			! if cache not enabled, return
	save	%sp, -SA(MINFRAME), %sp

	retl
	nop
1:
	sethi	%hi(_vac_size), %l0
	ld	[%l0 + %lo(_vac_size)], %l0
	sethi	%hi(_vac_linesize), %i5
	ld	[%i5 + %lo(_vac_linesize)], %i5
	sethi	%hi(_vac_nlines), %i1	! nlines to flush / # steps in loop
	ld	[%i1 + %lo(_vac_nlines)], %i1
	sethi	%hi(_flush_cnt+FM_REGION), %g1 ! increment flush count
	ld	[%g1 + %lo(_flush_cnt+FM_REGION)], %g2
	srl	%l0, 4, %l0		! cachesize / 16 (# of steps in loop)
	inc	%g2
	call	_flush_user_windows	! make sure no windows are hanging out
	st	%g2, [%g1 + %lo(_flush_cnt+FM_SEGMENT)]
	srl	%i0, SMGRPSHIFT, %i0	! mask off lo bits
	sll	%i0, SMGRPSHIFT, %i0

	!
	! preload a bunch of offsets
	! Avoid going through sequentially by flushing
	! 16 lines spread evenly through the cache.
	!
	add	%i0, %l0, %i0
	sub	%i0, %i5, %i0		! base address + cachesize - linesize
	add	%l0, %l0, %l1		! cachesize*2
	add	%l1, %l0, %l2		! cachesize*3
	add	%l2, %l0, %l3		! ...
	add	%l3, %l0, %l4
	add	%l4, %l0, %l5
	add	%l5, %l0, %l6
	add	%l6, %l0, %l7
	add	%l7, %l0, %o0
	add	%o0, %l0, %o1
	add	%o1, %l0, %o2
	add	%o2, %l0, %o3
	add	%o3, %l0, %o4
	add	%o4, %l0, %o5
	add	%o5, %l0, %i4
2:
	sta	%g0, [%i0]ASI_FCR
	sta	%g0, [%i0 + %l0]ASI_FCR
	sta	%g0, [%i0 + %l1]ASI_FCR
	sta	%g0, [%i0 + %l2]ASI_FCR
	sta	%g0, [%i0 + %l3]ASI_FCR
	sta	%g0, [%i0 + %l4]ASI_FCR
	sta	%g0, [%i0 + %l5]ASI_FCR
	sta	%g0, [%i0 + %l6]ASI_FCR
	sta	%g0, [%i0 + %l7]ASI_FCR
	sta	%g0, [%i0 + %o0]ASI_FCR
	sta	%g0, [%i0 + %o1]ASI_FCR
	sta	%g0, [%i0 + %o2]ASI_FCR
	sta	%g0, [%i0 + %o3]ASI_FCR
	sta	%g0, [%i0 + %o4]ASI_FCR
	sta	%g0, [%i0 + %o5]ASI_FCR
	sta	%g0, [%i0 + %i4]ASI_FCR
	subcc	%i1, 16, %i1		! decrement loop count
	bg	2b			! are we done yet?
	sub	%i0, %i5, %i0		! generate next address
	ret
	restore

#endif /* MMU_3LEVEL */

!
! Flush a segment from the cache.
! To flush the argument segment from the cache we hold the bits that
! specify the segment in the address constant and issue a store into
! alternate space command for each line of the cache by incrementing
! the lower bits of the address by VAC_LINESIZE (cache line size - 16).
!
! vac_segflush(v)
!	addr_t v;
!
	ENTRY(vac_segflush)
	sethi	%hi(_vac), %g1		! check if cache is turned on
	ld	[%g1 + %lo(_vac)], %g1
	tst	%g1
	bnz,a	1f			! if cache not enabled, return
	save	%sp, -SA(MINFRAME), %sp

	retl
	nop
1:
	sethi	%hi(_vac_size), %l0
	ld	[%l0 + %lo(_vac_size)], %l0
	srl	%l0, 4, %l0		! cachesize / number of steps in loop
	sethi	%hi(_vac_linesize), %i5
	ld	[%i5 + %lo(_vac_linesize)], %i5
	sethi	%hi(_vac_nlines), %i1	! nlines to flush / # steps in loop
	ld	[%i1 + %lo(_vac_nlines)], %i1
	sethi	%hi(_flush_cnt+FM_SEGMENT), %g1 ! increment flush count
	ld	[%g1 + %lo(_flush_cnt+FM_SEGMENT)], %g2
	srl	%i0, PMGRPSHIFT, %i0	! mask off lo bits
	sll	%i0, PMGRPSHIFT, %i0
	inc	%g2
	call	_flush_user_windows	! make sure no windows are hanging out
	st	%g2, [%g1 + %lo(_flush_cnt+FM_SEGMENT)]

	!
	! preload a bunch of offsets
	! Avoid going through sequentially by flushing
	! 16 lines spread evenly through the cache.
	!
	add	%i0, %l0, %i0
	sub	%i0, %i5, %i0		! base address + cachesize - linesize
	add	%l0, %l0, %l1		! cachesize/16*2
	add	%l1, %l0, %l2		! cachesize/16*3
	add	%l2, %l0, %l3		! ...
	add	%l3, %l0, %l4
	add	%l4, %l0, %l5
	add	%l5, %l0, %l6
	add	%l6, %l0, %l7
	add	%l7, %l0, %o0
	add	%o0, %l0, %o1
	add	%o1, %l0, %o2
	add	%o2, %l0, %o3
	add	%o3, %l0, %o4
	add	%o4, %l0, %o5
	add	%o5, %l0, %i4
2:
	sta	%g0, [%i0]ASI_FCS
	sta	%g0, [%i0 + %l0]ASI_FCS
	sta	%g0, [%i0 + %l1]ASI_FCS
	sta	%g0, [%i0 + %l2]ASI_FCS
	sta	%g0, [%i0 + %l3]ASI_FCS
	sta	%g0, [%i0 + %l4]ASI_FCS
	sta	%g0, [%i0 + %l5]ASI_FCS
	sta	%g0, [%i0 + %l6]ASI_FCS
	sta	%g0, [%i0 + %l7]ASI_FCS
	sta	%g0, [%i0 + %o0]ASI_FCS
	sta	%g0, [%i0 + %o1]ASI_FCS
	sta	%g0, [%i0 + %o2]ASI_FCS
	sta	%g0, [%i0 + %o3]ASI_FCS
	sta	%g0, [%i0 + %o4]ASI_FCS
	sta	%g0, [%i0 + %o5]ASI_FCS
	sta	%g0, [%i0 + %i4]ASI_FCS
	subcc	%i1, 16, %i1		! decrement loop count
	bg	2b			! are we done yet?
	sub	%i0, %i5, %i0		! generate next address
	ret
	restore

!
! Flush a page from the cache.
! To flush the page containing the argument virtual address from
! the cache we hold the bits that specify the page constant and
! issue a store into alternate space command for each line of
! the cache. Since the match part of the address is larger we
! have less lines to cycle through. Once again, we increment
! the lower bits of the address by VAC_LINESIZE to cycle through
! lines of the cache.
!
! vac_pageflush(v)
!	addr_t v;
!
	ENTRY(vac_pageflush)
	sethi	%hi(_vac), %g1		! check if cache is turned on
	ld	[%g1 + %lo(_vac)], %g1
	tst	%g1
	bnz,a	1f			! if cache not enabled, return
	save	%sp, -SA(MINFRAME), %sp

	retl
	nop
1:
	sethi	%hi(_flush_cnt+FM_PAGE), %g1 ! increment flush count
	ld	[%g1 + %lo(_flush_cnt+FM_PAGE)], %g2
	sethi	%hi(_vac_linesize), %i5
	inc	%g2
	st	%g2, [%g1 + %lo(_flush_cnt+FM_PAGE)]
#ifdef HISTORY
	set	EVT_VAC_FLUSH_PAGE, %o0
	mov	%i0, %o1
	call	_evt_log		! evt_log(EVT_VAC_FLUSH_PAGE, v, 0)
	clr	%o2
#endif HISTORY
	ld	[%i5 + %lo(_vac_linesize)], %i5
	sethi	%hi(_vac_pglines), %i1
	ld	[%i1 + %lo(_vac_pglines)], %i1
	srl	%i0, PGSHIFT, %i0	! mask off lo bits
	sll	%i0, PGSHIFT, %i0
	set	KERNELBASE, %l0
	cmp	%i0, %l0		! if user addr, flush windows
	bgeu	0f
	nop

	call	_flush_user_windows	! make sure no windows are hanging out
	nop

	!
	! A flush that causes a writeback will happen in parallel
	! with other instructions.  Back to back flushes which cause
	! writebacks cause the processor to wait until the first writeback
	! is finished and the second is initiated before proceeding.
	! Avoid going through a page sequentially by flushing
	! 16 lines spread evenly through the page.
	!
0:
	mov	NBPG/16, %l0		! 512
	add	%l0, NBPG/16, %l1	! 1024
	add	%l1, NBPG/16, %l2	! ...
	add	%l2, NBPG/16, %l3
	add	%l3, NBPG/16, %l4
	add	%l4, NBPG/16, %l5
	add	%l5, NBPG/16, %l6
	add	%l6, NBPG/16, %l7
	add	%l7, NBPG/16, %o0
	add	%o0, NBPG/16, %o1
	add	%o1, NBPG/16, %o2
	add	%o2, NBPG/16, %o3
	add	%o3, NBPG/16, %o4
	add	%o4, NBPG/16, %o5
	add	%o5, NBPG/16, %i4
	sub	%l0, %i5, %i3		! NBPG/16 - linesize
	add	%i0, %i3, %i0 		! page addr + 512 - 16
2:
	sta	%g0, [%i0]ASI_FCP
	sta	%g0, [%i0 + %l0]ASI_FCP
	sta	%g0, [%i0 + %l1]ASI_FCP
	sta	%g0, [%i0 + %l2]ASI_FCP
	sta	%g0, [%i0 + %l3]ASI_FCP
	sta	%g0, [%i0 + %l4]ASI_FCP
	sta	%g0, [%i0 + %l5]ASI_FCP
	sta	%g0, [%i0 + %l6]ASI_FCP
	sta	%g0, [%i0 + %l7]ASI_FCP
	sta	%g0, [%i0 + %o0]ASI_FCP
	sta	%g0, [%i0 + %o1]ASI_FCP
	sta	%g0, [%i0 + %o2]ASI_FCP
	sta	%g0, [%i0 + %o3]ASI_FCP
	sta	%g0, [%i0 + %o4]ASI_FCP
	sta	%g0, [%i0 + %o5]ASI_FCP
	sta	%g0, [%i0 + %i4]ASI_FCP
	subcc	%i1, 16, %i1		! decrement loop count
	bg	2b			! are we done yet?
	sub	%i0, %i5, %i0		! generate next match address
	ret
	restore

!
! Flush a range of addresses.
!
! vac_flush(v, nbytes)
!	addr_t vaddr;
!	int size;
!
	ENTRY(vac_flush)
	sethi	%hi(_vac), %g1		! check if cache is turned on
	ld	[%g1 + %lo(_vac)], %g1
	tst	%g1
	bnz,a	1f			! if cache not enabled, return
	save	%sp, -SA(MINFRAME), %sp

	retl
	nop
1:
	sethi	%hi(_vac_linesize), %i5
	ld	[%i5 + %lo(_vac_linesize)], %i5

	sethi	%hi(_flush_cnt+FM_PARTIAL), %g1 ! increment flush count
	ld	[%g1 + %lo(_flush_cnt+FM_PARTIAL)], %g2
	sub	%i5, 1, %l0		! build linesize mask
	andn	%i0, %l0, %i0		! mask off line offset bits
	inc	%g2
	st	%g2, [%g1 + %lo(_flush_cnt+FM_PARTIAL)]

	mov	%i5, %l1		! preload a bunch of offsets
	add	%i5, %l1, %l2		! linesize*2
	add	%i5, %l2, %l3		! linesize*3
	add	%i5, %l3, %l4		! linesize*4
	add	%i5, %l4, %l5		! linesize*5
	add	%i5, %l5, %l6		! linesize*6
	add	%i5, %l6, %l7		! linesize*7
	sll	%i5, 3, %i5		! linesize * 8 (loop flush amount)
	add	%i1, %l1, %i1		! round up size
2:
	sta	%g0, [%i0]ASI_FCP	
	sta	%g0, [%i0 + %l1]ASI_FCP
	sta	%g0, [%i0 + %l2]ASI_FCP
	sta	%g0, [%i0 + %l3]ASI_FCP
	sta	%g0, [%i0 + %l4]ASI_FCP
	sta	%g0, [%i0 + %l5]ASI_FCP
	sta	%g0, [%i0 + %l6]ASI_FCP
	sta	%g0, [%i0 + %l7]ASI_FCP
	subcc	%i1, %i5, %i1			! decrement count
	bg	2b				! are we done yet?
	add	%i0, %i5, %i0			! generate next chunk address

	ret
	restore

!
! Flush a single cache line
!
! vac_flushone(v)
!       addr_t vaddr;
!
	ENTRY(vac_flushone)
	set     _vac_linesize, %o1
	ld      [%o1], %o1
	sub     %o1, 1, %o1
	andn    %o0, %o1, %o0
	retl
	sta     %g0, [%o0]ASI_FCP

#endif VAC

! read sun4_330 conf register, Memory configuration.
!
	ENTRY(read_confreg)

	mov     0x2, %o0 
	retl
	lduha[%o0]ASI_SM, %o0

!
! Flush all user lines from the cache.  We do this by reading a portion
! of the kernel text starting at sys_trap. The size of the portion is
! equal to the VAC size. We read a word from each line. sys_trap was chosen
! as the start address because it is the start of the locore code 
! that we assume will be very likely executed in near future.
!
! We branch here from vac_usrflush if VAC does not support 'user flush' in HW.
! We have a new register window.
!
vac_fake_usrflush:
	!
	! Here we have:
	!  	i2	linesize
	!	l0	cachesize / 16
	! 
	
	!
	! Due to a bug in HW, some processors must map the trap vectors
	! non cacheable. Software (locore.s) must guarantee that the
	! code that follows the trap vectors starts in the next page.
	! We are paranoid about it and check that sys_trap is actually
	! in a cacheable page. We panic otherwise.
	!
	cmp	%i3, 1
	set	sys_trap, %i0		! start reading text seg. from sys_trap
	bne	2f
	mov	%l0, %i3		! loop counter: vac_size / 16

	! check pte only the first time vac_fake_usrflush is called
	lda	[%i0]ASI_PM, %l2	! read page map entry
	set	PG_NC, %l1
	btst	%l1, %l2
	bz	2f
	nop

	sethi	%hi(6f), %o0
	call	_panic		
	or	%o0, %lo(6f), %o0

	.seg	"data"
6:	.asciz	"vac_usrflush: sys_trap is not in cacheable page"
	.seg	"text"

2:
	!
	! A flush that causes a writeback will happen in parallel
	! with other instructions.  Back to back flushes which cause
	! writebacks cause the processor to wait until the first writeback
	! is finished and the second is initiated before proceeding.
	! Avoid going through the cache sequentially by flushing
	! 16 lines spread evenly through the cache.
	!
	!  i0 	start address 
	!  i2 	linesize
	!  i3   vac_size / 16
	!  l0	vac_size / 16
	!
	add	%l0, %l0, %l1		! 2 * (vac_size / 16)
	add	%l1, %l0, %l2	! ...
	add	%l2, %l0, %l3
	add	%l3, %l0, %l4
	add	%l4, %l0, %l5
	add	%l5, %l0, %l6
	add	%l6, %l0, %l7
	add	%l7, %l0, %o0
	add	%o0, %l0, %o1
	add	%o1, %l0, %o2
	add	%o2, %l0, %o3
	add	%o3, %l0, %o4
	add	%o4, %l0, %o5
	add	%o5, %l0, %i4		! 15 * (vac_size / 16)
	

3:
	ld	[%i0      ], %i1
	ld	[%i0 + %l0], %i1
	ld	[%i0 + %l1], %i1
	ld	[%i0 + %l2], %i1
	ld	[%i0 + %l3], %i1
	ld	[%i0 + %l4], %i1
	ld	[%i0 + %l5], %i1
	ld	[%i0 + %l6], %i1
	ld	[%i0 + %l7], %i1
	ld	[%i0 + %o0], %i1
	ld	[%i0 + %o1], %i1
	ld	[%i0 + %o2], %i1
	ld	[%i0 + %o3], %i1
	ld	[%i0 + %o4], %i1
	ld	[%i0 + %o5], %i1
	ld	[%i0 + %i4], %i1

	subcc	%i3, %i2, %i3		! decrement loop count
	bg	3b			! are we done yet?
	add	%i0, %i2, %i0		! generate next addr
	ret
	restore
