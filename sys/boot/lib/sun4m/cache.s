/*
 *	.seg	"data"
 *	.asciz	"@(#)cache.s	1.1 7/30/92 SMI"
 *	Copyright (c) 1988 by Sun Microsystems, Inc.
 */
	.seg	"text"
	.align	4

#include <machine/asm_linkage.h>
#include <machine/param.h>
#include <machine/pte.h>
#include <machine/mmu.h>
#include <machine/enable.h>
#define NSGVA	(1<<(32-PMGRPSHIFT))

/* XXX - hack for now */
#undef VAC

#ifdef VAC

/*
 * cache_prog(start, end)
 * change all of the text and data pages
 * that are used by prog to be cacheable
 */
	ENTRY(cache_prog)
	save	%sp, -WINDOWSIZE, %sp
	set	PG_NC, %l0		! the no cache bit
	set	NBPG, %l1		! pagesize increment
1:
	call	_getpgmap		! get pte
	mov	%i0, %o0		! address

	andn	%o0, %l0, %o1		! clear the nocache bit
	call	_setpgmap		! write the new pte
	mov	%i0, %o0		! address

	add	%i0, %l1, %i0		! add pagesize to get next pte
	cmp	%i0, %i1
	bleu	1b			! go do the next page
	nop

	ret
	restore

/*
 * turnon_cache
 *	write all the tags to zero
 *	turn on the cache via the system enable register
 */
	ENTRY(turnon_cache)
	set     CACHE_TAGS, %o0		! address of cache tags in CTL space
	set     _vac_size, %o1		! number of lines to initialize
	ld	[%o1], %o1
	set     _vac_linesize, %o2	! offsets for by 4 loop unroll
	ld	[%o2], %o2
	sll	%o2, 1, %o3		! VAC_LINESIZE * 2
	add	%o2, %o3, %o4		! VAC_LINESIZE * 3
	sll	%o2, 2, %o5		! VAC_LINESIZE * 4
1:
	sta     %g0, [%o0]ASI_CTL       ! write tags to zero
	sta     %g0, [%o0 + %o2]ASI_CTL ! offset 16
	sta     %g0, [%o0 + %o3]ASI_CTL ! offset 32
	sta     %g0, [%o0 + %o4]ASI_CTL ! offset 48
	subcc   %o1, 4, %o1             ! done yet?
	bg      1b
	add     %o0, %o5, %o0		! next cache tags address

	set     ENABLEREG, %o2          ! address of real version in hardware
	lduba   [%o2]ASI_CTL, %g1	! get current value
	bset	ENA_CACHE, %g1		! enable cache bit
	retl
	stba	%g1, [%o2]ASI_CTL	! write new value

/*
 * disable the cache, flush the cache completely (all of context 0)
 * We can't flush context 0 with a context flush, (a feature)
 * because we've been using context 0 in supervisor mode,
 * so we loop through segments.  Once flushed,  turn off the 
 * cache via the system enable register
 * NOTE: this assumes that standalones only use one context
 */
	ENTRY(turnoff_cache)
	save	%sp, -WINDOWSIZE, %sp
	mov	0, %l0			! address of first segment
	set	PMGRPSIZE, %l1		! number of bytes in a segment
	set	NPMGRPPERCTX/2, %l2	! number of segs to flush in 1st half
	set     _vac_linesize, %l5	! offsets for by 4 loop unroll
	ld	[%l5], %l5
	sll	%l5, 1, %l6		! VAC_LINESIZE * 2
	add	%l5, %l6, %l7		! VAC_LINESIZE * 3
	sll	%l5, 2, %l4		! VAC_LINESIZE * 4
	set	_npmgrps, %l3
	ld	[%l3], %l3
	dec	%l3			! PMGRP_INVALID
	set	_vac_size, %o2
	ld	[%o2], %o2
	mov	%l5, %o3		! compute NLINES=SIZE/LINESIZE...
0:
	srl	%o3, 1, %o3		! we "know" LINESIZE is a power of 2
	tst	%o3			!  so just shift until LINESIZE
	bnz,a	0b			!  goes to zero
	srl	%o2, 1, %o2
1:
	lduha	[%l0]ASI_SM, %g1	! get segment map
	cmp	%g1, %l3		! if invalid we don't have to flush
	be	3f
	.empty
	!
	! flush the segment
	!
	mov	%l0, %o0
	mov	%o2, %o1		! %o1 = VAC_SEGFLUSH_COUNT
2:	
	sta	%g0, [%o0]ASI_FCS
	sta	%g0, [%o0 + %l5]ASI_FCS
	sta	%g0, [%o0 + %l6]ASI_FCS
	sta	%g0, [%o0 + %l7]ASI_FCS
	deccc	4, %o1			! dec count
	bg	2b			! done?
	add	%o0, %l4, %o0		! next group of lines
3:
	deccc	%l2			! count down segments
	bnz	1b
	add	%l0, %l1, %l0		! next segment

	tst	%l0			! at end of 2nd half of addr space?
	set	(NSGVA - (NPMGRPPERCTX/2)) * PMGRPSIZE, %l0
					! start of 2nd half
	set	NPMGRPPERCTX/2, %l2	! number of segs to flush in 2nd half
	bnz	1b			! no, do 2nd half
	.empty

	set     ENABLEREG, %o0          ! address of enable reigster
	lduba   [%o0]ASI_CTL, %g1	! get current value
	bclr	ENA_CACHE, %g1		! clear enable cache bit
	stba	%g1, [%o0]ASI_CTL	! write new value
	ret				! cache is now off
	restore

#else VAC

	ENTRY(cache_prog)
	ALTENTRY(turnon_cache)
	ALTENTRY(turnoff_cache)
	retl
	nop

#endif
