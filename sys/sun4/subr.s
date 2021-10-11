/*	@(#)subr.s 1.1 92/07/30 SMI	*/

/*
 *	Copyright (c) 1989 by Sun Microsystems, Inc.
 */

/*
 * General assembly language routines that are architecture-specific and
 * can't be shared across architectures.
 * General assembly language routines that are architecture-independent,
 * or that can be shared across architectures, belong in
 * ../sparc/sparc_subr.s.
 */

#include <machine/asm_linkage.h>
#include <machine/mmu.h>

	.seg	"text"
	.align	4

/*
 * Read the ID prom.
 * This is mapped from ID_PROM for IDPROMSIZE bytes in the
 * ASI_CTL address space for byte access only.
 */
	ENTRY(getidprom)
	set	ID_PROM, %o5
	clr	%o4
1:
	lduba	[%o5 + %o4]ASI_CTL, %o3	! get id prom byte
	add	%o4, 1, %o4		! interlock
	stb	%o3, [%o0]		! put it out
	cmp	%o4, IDPROMSIZE		! done yet?
	bne,a	1b
	add	%o0, 1, %o0		! delay slot
	retl				! leaf routine return
	nop				! delay slot
