/*
 *	Copyright (c) 1987 by Sun Microsystems, Inc.
 */
#include "assym.s"
#include <machine/asm_linkage.h>
#include <machine/mmu.h>
#include <machine/psl.h>
#include <sundev/zsreg.h>
#include <machine/intreg.h>

	.seg	"data"
	.asciz	"@(#)zs_asm.s 1.1 92/07/30"
	.align	4
	.seg	"text"


/* 1.6 usec UART recovery time. XXX clock dependent.
 * currently tuned for 25Mhz sparc, i.e. 4/330 */
#define ZSRECOVER() \
	mov	13, %o5; \
0:	deccc	%o5; \
	bnz	0b; \
	nop;

/*
 * We come here for auto-vectored interrupts
 * We assume that the most recently interrupting
 * chip is interrupting again and read the vector out of the
 * chip and branch to the right routine. The "special receive"
 * interrupt routine gets control if the assumption was wrong
 * and performs the longer task of figuring out who is really interrupting
 * and why.
 */
	.global	_zscurr
	.global	_zsNcurr

	ENTRY(zslevel12)
	save	%sp, -SA(MINFRAME), %sp	! get a new window
	set	_zscurr, %g1		! most active channel desc pointer
	ld	[%g1], %l0		! read it
	mov	2, %g2			! register number 2 in channel B
	ld	[%l0 + ZS_ADDR], %l1	! get address of hardware channel
	stb	%g2, [%l1]		! set register to read
	ZSRECOVER();
	ldub	[%l1], %l2		! get interrupt vector number
	ZSRECOVER();
	btst	8, %l2			! channel A?
	bnz,a	1f			! if not anull next instruction
	sub	%l0, ZSSIZE, %l0	! point to channel A's descriptor
1:
	set	_zsNcurr, %g1		! stash the [modified] channel
	st	%l0, [%g1]		! where it MIGHT be changed
	and	%l2, 6, %l2		! get vetor bits
	add	%l2, %l2, %l2		! get multiple of 4
	ld	[%l0 + %l2], %g1	! get address of routine to call
	call	%g1			! go to it
	mov	%l0, %o0		! delay slot, fix argument

	set	_zsNcurr, %g1		! restore the channel, since
	ld	[%g1], %l0		! it might have been changed
	ld	[%l0 + ZS_ADDR], %l1	! get address of hardware channel
	mov	ZSWR0_CLR_INTR, %g2	! value to clear interrupt
	stb	%g2, [%l1]		! clear interrupt
	ZSRECOVER();
	ret
	restore

/*
 * Turn on a zs soft interrupt.
 */
	ENTRY(setzssoft)
	mov	%psr, %g2
	or	%g2, PSR_PIL, %g1	! spl hi to protect intreg update
	mov	%g1, %psr
	nop; nop;			! psr delay
	set	INTREG_ADDR, %o0	! set address of interrupt register
	ldub	[%o0], %o1		! read it
	bset	IR_SOFT_INT6, %o1	! set zs soft interrupt bit
	stb	%o1, [%o0]		! write it out
	mov	%g2, %psr		! restore psr
	nop				! psr delay
	retl				! leaf routine return
	nop

/*
 * Test and clear a zs soft interrupt.
 */
	ENTRY(clrzssoft)
	set	INTREG_ADDR, %o1	! set address of interrupt register
	ldub	[%o1], %o2		! read it
	btst	IR_SOFT_INT6, %o2	! was there an interrupt pending?
	bz,a	1f			! no, simply return 0
	mov	0, %o0			! return value

	mov	%psr, %g2
	or	%g2, PSR_PIL, %g1	! spl hi to protect intreg update
	mov	%g1, %psr
	nop; nop; 			! psr delay
	mov	1, %o0			! otherwise one was pending
	ldub	[%o1], %o2		! read it
	bclr	IR_SOFT_INT6, %o2 	! clear zs soft int bit
	stb	%o2, [%o1]		! write interrupt register
	mov	%g2, %psr		! restore psr
	nop				! psr delay
1:
	retl
	nop

/*
 * Read a control register in the chip
 */
	ENTRY(zszread)
	stb	%o1, [%o0]		! write register number
	ZSRECOVER();
	ldub	[%o0], %o0		! read register
	ZSRECOVER();
	retl
	nop

/*
 * Write a control register in the chip
 */
	ENTRY(zszwrite)
	stb	%o1, [%o0]		! write register number
	ZSRECOVER();
	stb	%o2, [%o0]		! write data
	ZSRECOVER();
	retl
	nop
