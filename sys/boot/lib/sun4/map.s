/*
 *	.seg	"data"
 *	.asciz	"@(#)map.s 1.1 92/07/30"
 *	Copyright (c) 1986 by Sun Microsystems, Inc.
 */
 	.seg	"text"
 	.align	4
/*
 * Memory Mapping for Sun 4
 *
 * The following subroutines accept any address in the mappable range
 * (256 megs).  They access the map for the current context register.  They
 * assume that currently we are running in supervisor state.
 */

#include <sun4/asm_linkage.h>
#include <sun4/mmu.h>
#define	CONTEXTBASE	0x30000000	/* context reg */
#define	SEGMENTADDRBITS 0xFFFC0000	/* segment map virtual address mask */


/*
 * Set the pme for address v using the software pte given.
 * 
 * setpgmap(v, pte)
 * caddr_t v;
 * long pte;
 */ 
	ENTRY(setpgmap)
	andn	%o0,0x3,%o2		! align to word boundary
	retl
	sta	%o1,[%o2]ASI_PM		! write page map entry

/*
 * Read the page map entry for the given address v
 * and return it in a form suitable for software use.
 *
 * long
 * getpgmap(v)
 * caddr_t v;
 */
	ENTRY(getpgmap)
	andn	%o0,0x3,%o1		! align to word boundary
	lda	[%o1]ASI_PM,%o0		! read page map entry
	retl
nop			! nop hack needed for FAB 1 XXXX

#ifndef BOOTBLOCK
/*
 * Set the segment map entry for segno to pm.
 * 
 * setsegmap(v, pm)
 * u_int segno;
 * u_short pm;
 */
	ENTRY(setsegmap)
	set	SEGMENTADDRBITS, %o2
	and	%o0, %o2, %o0		! get relevant segment address bits
#ifdef P1
or	%o0, 0x2, %o0	! proto1 hack !!!! XXXXX
#endif P1
	retl
	stha	%o1,[%o0]ASI_SM		! write segment entry

/*
 * Get the segment map entry for ther given virtual address
 * 
 * getsegmap(v)
 * caddr_t vaddr;
 */
	ENTRY(getsegmap)
	andn    %o0, 0x1, %o0           ! align to halfword boundary
	lduha	[%o0]ASI_SM,%o0		! read segment entry
	set     _segmask, %o2           ! need to mask bits due to bug in cobra
	ld      [%o2], %o2
	retl
	and     %o0, %o2, %o0          ! A HACK BECAUSE COBRA IS BROKEN

/*
 * Set the current [user] context number to uc.
 *
 * setcontext(uc)
 * int uc;
 */
        ENTRY(setcontext)
        set     CONTEXTBASE, %o1
        retl
        stba    %o0, [%o1]ASI_CTL       ! write the context register

#endif !BOOTBLOCK
