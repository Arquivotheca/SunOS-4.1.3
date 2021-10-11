!	@(#)addupc.s 1.1 92/07/30 SMI
!	Copyright (c) 1986 by Sun Microsystems, Inc.

	.seg	"text"
	.align	4

#include <machine/asm_linkage.h>

!
! Add to user profiling counters.
!
! struct uprof {		/* profile arguments */
! 	short	*pr_base;	/* buffer base */
! 	unsigned pr_size;	/* buffer size */
! 	unsigned pr_off;	/* pc offset */
! 	unsigned pr_scale;	/* pc scaling */
! } ;
!
! addupc( pc, pr, incr)
!	register int pc;
!	register struct uprof *pr;
!	int incr;
! {
! 	register short *slot;
! 	short counter;
!
! 	slot = pr->pr_base
! 		+ (((pc - pr->pr_off) * pr->pr_scale) >> 16)/(sizeof *slot);
! 	if (slot >= pr->pr_base &&
! 	    slot < (short *)(pr->pr_size + (int)pr->pr_base)) {
! 		if ((counter=fusword(slot))<0) {
! 			pr->pr_scale = 0;	/* turn off profiling */
! 		} else {
! 			counter += incr;
! 			susword(slot, counter);
! 		}
! 	}
! }
!

	.global	.mul

PR_BASE =	0
PR_SIZE	=	4
PR_OFF	=	8
PR_SCALE=	12

/*
 * addupc(pc, pr, incr)
 */
	ENTRY(addupc)
	save	%sp, -SA(MINFRAME), %sp

	ld	[%i1 + PR_OFF], %l0	! pr->pr_off
	subcc	%i0, %l0, %o0		! pc - pr->pr_off
	bl	out
	srl	%o0, 1, %o0		! /2, for sign bit
	ld	[%i1 + PR_SCALE], %o1
	call	.mul			! ((pc - pr->pr_off) * pr->pr_scale)/4
	srl	%o1, 1, %o1		! /2

	sra	%o1, 14, %g1		! overflow after >>14?
	tst	%g1
	bnz	out
	srl	%o0, 14, %l0		! >>14 double word
	sll	%o1, 32-14, %g1
	or	%l0, %g1, %l0
	bclr	1, %l0			! make even
	ld	[%i1 + PR_SIZE], %l1	! check length
	cmp 	%l0, %l1
	bgeu	out
	ld	[%i1 + PR_BASE], %l1	! add base
	add	%l1, %l0, %l0
	call	_fusword		! fetch counter from user
	mov	%l0, %o0		! delay slot
	tst	%o0
	bge,a	1f			! fusword succeeded
	add	%o0, %i2, %o1		! counter += incr
	!
	! Fusword failed turn off profiling.
	!
	clr	[%i1 + PR_SCALE]
	ret
	restore

	!
	! Store new counter
	!
1:
	call	_susword		! store counter, checking permissions
	mov	%l0, %o0		! delay slot

out:
	ret
	restore
