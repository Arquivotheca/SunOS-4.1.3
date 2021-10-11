/*	@(#)gt_asm.s 1.1 92/07/30 SMI	*/

/*
 *	Copyright (c) 1992 by Sun Microsystems, Inc.
 */

/*
 * Gets/sets bits in the specified sbus slot configuration register.
 * This should be done by the device OpenBoot PROM, but this
 * isn't done in pre-existing devices.
 *
 * XXX: This shouldn't be device-specific.
 */
#include <machine/asm_linkage.h>
#include <machine/mmu.h>

	.seg	"text"
	.align	4

	ALTENTRY(gt_ldsbusreg)
	retl
	lda	[%o0]ASI_CTL, %o0


	ALTENTRY(gt_stsbusreg)
	retl
	sta	%o1, [%o0]ASI_CTL
