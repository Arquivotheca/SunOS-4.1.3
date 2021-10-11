/*
 *	.seg	"data"
 *	.asciz	"@(#)map.s 1.1 92/07/30 SMI"
 *	Copyright (c) 1988 by Sun Microsystems, Inc.
 */
	.seg	"text"
	.align	4
/*
 * Memory Mapping for Sun 4c
 *
 * The following subroutines accept any address in the mappable range
 * (256 megs).  They access the map for the current context register.  They
 * assume that currently we are running in supervisor state.
 */

#include <machine/asm_linkage.h>
#include <machine/mmu.h>

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
	retl
	lda	[%o1]ASI_PM,%o0		! read page map entry

/*
 * Set the segment map entry for segno to pm.
 * 
 * setsegmap(segno, pm)
 * u_int segno;
 * u_short pm;
 */
	ENTRY(setsegmap)
	set	SEGMENTADDRBITS, %o2
	and	%o0, %o2, %o0		! get relevant segment address bits
	retl
	stha	%o1,[%o0]ASI_SM		! write segment entry

/*
 * Get the segment map entry for the given virtual address
 * 
 * getsegmap(vaddr)
 * caddr_t vaddr;
 */
	ENTRY(getsegmap)
	retl
	lduba	[%o0]ASI_SM,%o0		! read segment entry

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
