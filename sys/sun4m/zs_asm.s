/*
 *	Copyright (c) 1987, 1988, 1989 by Sun Microsystems, Inc.
 */
#include "assym.s"
#include <machine/asm_linkage.h>
#include <machine/mmu.h>
#include <machine/psl.h>
#include <sundev/zsreg.h>
#include <machine/intreg.h>

	.seg	"data"
	.ident	"@(#)zs_asm.s 1.1 92/07/30"
	.align	4
	.seg	"text"

/*
 * We assume that the most recently interrupting
 * chip is interrupting again and read the vector out of the
 * chip and branch to the right routine. The "special receive"
 * interrupt routine gets control if the assumption was wrong
 * and performs the longer task of figuring out who is really interrupting
 * and why.
 *
 * %%% actually, SIPR tells us which zs is interrupting, we just
 * have to take a look at it. DO THIS LATER.
 */
	.global	_zscurr
	.global	_zsNcurr

	ENTRY(zsintr_hi)
	save	%sp, -SA(MINFRAME), %sp	! get a new window
	set	_zscurr, %g1		! most active channel desc pointer
	ld	[%g1], %l0		! read it
	mov	2, %g2			! register number 2 in channel B
	ld	[%l0 + ZS_ADDR], %l1	! get address of hardware channel
	stb	%g2, [%l1]		! set register to read
	ldub	[%l1], %l2		! get interrupt vector number
	btst	8, %l2			! channel A?
	bnz,a	1f			! if not anull next instruction
	sub	%l0, ZSSIZE, %l0	! point to channel A's descriptor
1:
	set	_zsNcurr, %g1		! stash the [modified] channel
	st	%l0, [%g1]		! where it MIGHT be changed
	and	%l2, 6, %l2		! get vetor bits
	add	%l2, %l2, %l2		! get multiple of 4
	ld	[%l0 + %l2], %g1	! get address of routine to call
	call	%g1			! go to it (no meaningful return value)
	mov	%l0, %o0		! delay slot, fix argument

	set	_zsNcurr, %g1		! restore the channel, since
	ld	[%g1], %l0		! it might have been changed
	ld	[%l0 + ZS_ADDR], %l1	! get address of hardware channel
	mov	ZSWR0_CLR_INTR, %g2	! value to clear interrupt
	stb	%g2, [%l1]		! clear interrupt
	add	%g0, 1, %i0		! always return TRUE.
	ret
	restore			! pass rtn value back

/*
 * Turn on a zs soft interrupt.
 */
	ENTRY(setzssoft)
	set	IR_SOFT_INT(ZS_SOFT_INT),%o0
	b	_set_intreg
	mov	1,%o1

/*
 * Test and clear a zs soft interrupt.
 */
	ENTRY(clrzssoft)
	set	IR_SOFT_INT(ZS_SOFT_INT),%o0
	b	_set_intreg
	mov	0,%o1

/*
 * Read a control register in the chip
 */
	ENTRY(zszread)
	stb	%o1, [%o0]		! write register number
	retl
	ldub	[%o0], %o0		! read register

/*
 * Write a control register in the chip
 */
	ENTRY(zszwrite)
	stb	%o1, [%o0]		! write register number
	retl
	stb	%o2, [%o0]		! write data
