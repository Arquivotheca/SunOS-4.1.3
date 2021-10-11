/*	@(#)map.s 1.1 92/07/30	*/

!
!	Copyright (c) 1986 by Sun Microsystems, Inc.
!

/*
 * Additional memory mapping routines for use by standalone debugger,
 * setpgmap(), getpgmap() are taken from the boot code.
 */

#include "asm_linkage.h"
#include "assym.s"
#include <sys/param.h>
#include <sun4/mmu.h>
#include <sun4/pte.h>
#include <sun4/enable.h>
#include <sun4/cpu.h>
#include "../../debug/debug.h"

	.seg	"text"
	.align	4

/*
 * Read the page map entry for the given address v
 * and return it in a form suitable for software use.
 *
 * long
 * Getpgmap(v)
 * caddr_t v;
 */
	ENTRY(Getpgmap)
	andn	%o0, 0x3, %o1		! align to word boundary
	lda	[%o1]ASI_PM, %o0	! read page map entry
	retl
	nop

/*
 * Set the pme for address v using the software pte given.
 * Setpgmap() automatically turns on the ``no cache'' bit
 * for all mappings between DEBUGSTART and DEBUGEND.
 *
 * Setpgmap(v, pte)
 * caddr_t v;
 * long pte;
 */
	ENTRY(Setpgmap)
	set	DEBUGSTART, %o2		! skip ahead if before debugger
	cmp	%o0, %o2
	blu	1f
	.empty				! delay slot ok
	set	DEBUGEND, %o2		! skip ahead if after debugger
	cmp	%o0, %o2
	bgeu	1f
	.empty				! delay slot ok
	set	PG_NC, %o2		! set don't cache bit
	or	%o1, %o2, %o1
1:
	andn	%o0, 0x3, %o2		! align to word boundary
	retl
	sta	%o1, [%o2]ASI_PM	! write page map entry

/*
 * Get the segment map entry for ther given virtual address
 *
 * Getsegmap(v)
 * caddr_t vaddr;
 */
        .global _Getsegmap
_Getsegmap:
	andn    %o0, 0x1, %o0		! get relevant segment address bits
	lduha   [%o0]ASI_SM,%o0		! read segment entry
	set	_segmask, %o2		! need to mask bits due to bug in cobra
	ld	[%o2], %o2
	retl
	and	%o0, %o2, %o0

/*
 * Set the segment map entry for ther given virtual address
 *
 * Setsegmap(v, segm)
 * caddr_t vaddr;
 */
        .global _Setsegmap
_Setsegmap:
        andn    %o0, 0x1, %o0           ! get relevant segment address bits
        set     _segmask, %o2           ! need to mask bits due to bug in cobra
        ld      [%o2], %o2
        and     %o1, %o2, %o1
        retl
        stha    %o1, [%o0]ASI_SM

/*
 * Set the current context number to 0.
 *
 * setcontext()
 */
	ENTRY(setkcontext)
	set	CONTEXT_REG, %o0
	retl
	stba	%g0, [%o0]ASI_CTL	! write the context register

/*
 * get the machine type in the ID prom.
 *
 * u_char
 * getmachinetype()
 */
	ENTRY(getmachinetype)
	set	ID_PROM+1, %o0
	retl
	lduba	[%o0]ASI_CTL, %o0


/*
 * Flush a page from the cache.
 *
 * vac_pageflush(vaddr)
 * caddr_t vaddr;
 */
	ENTRY(vac_pageflush)
	srl	%o0, PGSHIFT, %o0	! mask off low bits
	sll	%o0, PGSHIFT, %o0
	set	_vac_linesize, %o1	! linesize
	ld	[%o1], %o1
	set	_vac_pglines, %o2	! number of lines in a page
	ld	[%o2], %o2
1:
	sta	%g0, [%o0]ASI_FCP	! flush a line, page match
	deccc	%o2			! decrement count
	bg	1b			! done yet?
	add	%o0, %o1, %o0		! generate next match address
2:
	retl
	nop

!
! Initialize the cache by invalidating all the cache tags.
! It DOESN'T turn on cache enable bit in the enable register.
!
! void
! vac_tagsinit()
!
	ENTRY(vac_tagsinit)
	set	_vac_linesize, %o0	! linesize
	ld	[%o0], %o2
	set	VAC_SIZE, %o1		! size of cache
	set	CACHE_TAGS, %o0		! address of cache tags in CTL space
1:
	subcc	%o1, %o2, %o1		! done yet?
	bgu	1b
	sta	%g0, [%o0 + %o1]ASI_CTL	! write tags to zero
	retl
	nop
