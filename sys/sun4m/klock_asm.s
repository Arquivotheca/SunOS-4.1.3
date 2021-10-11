	.ident	"@(#)klock_asm.s 1.1 92/07/30 SMI"

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <machine/devaddr.h>
#include <machine/mmu.h>
#include <machine/asm_linkage.h>
#include <machine/psl.h>

! We are activated before the PerCPU area has
! been mapped in, so we need to know these
! physical addresses to talk with the ITR
! and the SetSoftInt registers.

_4m_itr_pa = 0xF1410010		! physical addr in ASI_CTL
_4m_ssi_pa = 0xF1400008		! physical addr in ASI_CTL

#define	GETKLV(m,l)		\
	GETMID(m)		;	\
	set	0xFF000000, l	;	\
	or	l, m, l

	.seg	"data"
	.align	4
	.global	_klock

/*
 * bytes are busy, [rsv], [rsv], mymid
 * initial value is "held by cpu zero"
 *	   VALUE	STATE
 *	00000000	free
 *	FF000000	held by "someone" (who has traps disabled)
 *	FF000008	held by cpu zero
 *	FF000009	held by cpu one
 *	FF00000A	held by cpu two
 *	FF00000B	held by cpu three
 */
_klock:
	.word	0

	.seg	"text"
	.align	4

/*
 * klock_init: initialize the kernel lock to point
 * to the current processor.
 *
 * NOTE: the first call to this routine happens with the prom's value
 * in the %tbr, so the actual MID number is not yet set up and we are
 * some random processor; the key element is that subsequent calls
 * to klock_enter do not block. When we get the proper %tbr set up
 * with our MID encoded, we call this routine again, and everything
 * becomes proper.
 */
	ALTENTRY(klock_init)
	sethi	%hi(_klock), %o0
	GETKLV(%o1,%o2)
	retl
	st	%o2, [%o0+%lo(_klock)]	! set lock ownership

/*
 * klock_enter: enter the kernel lock.
 * properly handles already owning the lock
 * and spins if the lock is busy elsewhere.
 *
 * Due to call-site restrictions, this routine
 * must only use %o registers.
 */

	ALTENTRY(klock_enter)

	mov	2, %o5
	sethi	%hi(_cpu_supv), %o0	! entering supv mode [SPIN]
	st	%o5, [%o0 + %lo(_cpu_supv)]

	mov	%psr, %o4
	sethi	%hi(_klock), %o0
	GETKLV(%o1,%o2)
			! spin loop comes back here.
5:
	ld	[%o0+%lo(_klock)], %o3
	cmp	%o3, %o2
	bnz	6f
	nop
			! we own the lock.
	mov	1, %o5
	sethi	%hi(_cpu_supv), %o0	! entering supv mode [NOSPIN]
	st	%o5, [%o0 + %lo(_cpu_supv)]

	retl
	nop
			! we do not own the lock.
6:
	tst	%o3
	bnz	5b
	nop
			! lock is free.
7:
	wr	%o4, PSR_ET, %psr		! DISABLE TRAPS
	nop ; nop ; nop		! psr delay
	ldstub	[%o0+%lo(_klock)], %o3	! contend for the lock.
	tst	%o3
	bnz,a	5b			! loop back if we lost
	  mov	%o4, %psr		! ENABLE TRAPS

	st	%o2, [%o0+%lo(_klock)]	!   store our mid
	mov	%o4, %psr		! ENABLE TRAPS

	mov	1, %o5
	sethi	%hi(_cpu_supv), %o0	! entering supv mode [NOSPIN]
	retl
	st	%o5, [%o0 + %lo(_cpu_supv)]


/*
 * klock_knock: try to acquire the lock. Return true
 * for success, false for failure. Traps might be disabled.
 */

	ALTENTRY(klock_knock)
	sethi	%hi(_klock), %o5
	GETKLV(%o1, %o2)
	mov	%psr, %o4
	andn	%o4, PSR_ET, %o1

				! see if lock already held.
1:
	ld	[%o5+%lo(_klock)], %o3
	cmp	%o3, %o2
	bnz	2f
	nop
				! lock already held.
	retl
	mov	2, %o0

2:
	tst	%o3
	bz	4f
	nop
				! lock not available.
	btst	0xF, %o3
	bz	1b			! in transit -- try again.
	nop

3:
				! lock held elsewhere.
	nop				! psr delay
	retl
	clr	%o0
				! lock is free.
4:
	mov	%o1, %psr		! DISABLE TRAPS
	nop; nop; nop
	ldstub	[%o5+%lo(_klock)], %o3
	tst	%o3			! if we lost,
	bnz,a	3b			! loop back to failure
	  mov	%o4, %psr		!   RESTORE TRAPS

	st	%o2, [%o5+%lo(_klock)]	! mark ownership
	mov	%o4, %psr		! RESTORE TRAPS

	mov	1, %o5
	sethi	%hi(_cpu_supv), %o0	! entered supv mode
	st	%o5, [%o0 + %lo(_cpu_supv)]

	retl
	mov	1, %o0


/*
 * klock_intlock: if this interrupt level requires the kernlock,
 * try to acquire it; if the attempt fails, forward soft ints
 * and discard hard ints. Returns zero if we should process
 * this interrupt, else returns nonzero.
 *
 * NOTE: this routine is called with traps disabled.
 *
 * Problems with ethernet are pointing at this routine
 * as the culpret, so for now I have removed all the
 * optimisations and trailer-fills. This will get smaller
 * and faster when things have been made reliable.
 */

	ALTENTRY(klock_intlock)
			! ITR bit for interrupt in %o0
	set	0xC000C000, %o1		! == bitmap of interrupts serviced outside klock
	btst	%o1, %o0
	bz	0f
	nop
			! do it outside the lock.
	retl
	clr	%o0		! allow service

			! this level needs the lock.
0:
	sethi	%hi(_klock), %o5
	GETKLV(%o1, %o2)

			! see if already held
1:
	ld	[%o5+%lo(_klock)], %o3
	cmp	%o3, %o2
	bnz	2f
	nop
			! we own the lock.
	retl
	clr	%o0		! allow service

			! we do not own the lock.
2:
	tst	%o3
	bz	4f		! check for busy
	nop
			! lock is busy.
	btst	0xFF, %o3
	bz	1b		! check for in-transit
	nop

			! lock owner in %o3
3:
	and	%o3, 3, %o3		! mask to cpu-id
	sll	%o3, 12, %o4		! shift to intreg offset
	set	_4m_ssi_pa, %o2
	set	0xFFFF0000, %o1		! == bitmap of ints to forward via ssi
	btst	%o1, %o0
	bnz,a	0f			! if softint,
	  sta	%o0, [%o2+%o4]ASI_CTL	!   forward soft int by setting SSI

	set	_4m_itr_pa, %o2		! else, find ITR;
	sta	%o3, [%o2]ASI_CTL	!   forward hard int by setting ITR
	lda	[%o2]ASI_CTL, %g0	!   interlock ITR to avoid repeated ints
0:
	retl
	mov	1, %o0		! prevent service

			! lock is free.
4:
	ldstub	[%o5+%lo(_klock)], %o3
	tst	%o3
	bnz	1b			! loop back if we lost
	nop				!   be sure to re-read owner field

	st	%o2, [%o5+%lo(_klock)]	! set owner
	retl
	clr	%o0		! allow service

/*
 * klock_exit: release the kernlock, if necessary.
 * callable from C ... but, if you stay in supervisor
 * mode, the next interrupt or trap is going to
 * leave you back inside the lock ...
 *
 * Due to call-site restrictions, this routine
 * must ONLY use %o registers.
 */
	ALTENTRY(klock_exit)
	sethi	%hi(_klock), %o0
	ld	[%o0+%lo(_klock)], %o3	! read lock word
	GETKLV(%o1, %o2)
	cmp	%o2, %o3
	beq,a	0f			! if I own it,
	  st	%g0, [%o0+%lo(_klock)]	!   release it.
0:	sethi	%hi(_cpu_supv), %o0	! leaving supvervisor mode ...
	retl
	st	%g0, [%o0 + %lo(_cpu_supv)]


/*
 * _klock_rett: intercepted "return from trap"
 *
 * This code runs within the trap window, so
 * it must only use %l registers and restore all
 * externally visible state (like the PSR!),
 * and it is terminated with "jmp/rett".
 *
 * If efficiency dictates, this may be moved
 * in-line instead of being branched to.
 */
	ALTENTRY(klock_rett)
	mov	%psr, %l0
	andcc	%l0, PSR_PS, %g0	! if returning to supv mode,
	bnz	1f			! do not release the lock.
	nop

	sethi	%hi(_cpu_supv), %l6	! leaving supvervisor mode ...
	st	%g0, [%l6 + %lo(_cpu_supv)]

	sethi	%hi(_klock), %l6
	ld	[%l6+%lo(_klock)], %l3	! read lock word
	GETKLV(%l4,%l5)
	cmp	%l3, %l5
	beq,a	0f			! if I own it,
	  st	%g0, [%l6+%lo(_klock)]	! release it.
0:
1:
	mov	%l0, %psr	! restore condition codes
	nop			! psr delay
	jmp	%l1
	rett	%l2

/*
 * _klock_rett_o6o7: intercepted "return from trap"
 *
 * Just like _klock_rett, except that pc and npc
 * are in %o6 and %o7, and we must clean all the
 * registers we touch.
 */
	ALTENTRY(klock_rett_o6o7)
	mov	%psr, %l0
	andcc	%l0, PSR_PS, %g0	! if returning to supv mode,
	bnz	1f			! do not release the lock.
	nop

	sethi	%hi(_cpu_supv), %l6	! leaving supvervisor mode ...
	st	%g0, [%l6 + %lo(_cpu_supv)]

	sethi	%hi(_klock), %l6
	ld	[%l6+%lo(_klock)], %l3	! read lock word
	GETKLV(%l4,%l5)
	cmp	%l3, %l5
	beq,a	0f			! if I own it,
	  st	%g0, [%l6+%lo(_klock)]	! release it.
0:
	clr	%l3
	clr	%l4		! clean the regs I scribbled on
	clr	%l5
	clr	%l6
1:
	mov	%l0, %psr	! restore condition codes
	clr	%l0		! clean up; also, psr delay.
	jmp	%o6
	rett	%o7

/*
 * klock_require: called from a site that
 * requires that we are inside the klock.
 * Calls to this routine are sanity checks
 * that should be removed before FCS, but
 * after we have verified that it never
 * triggers.
 */
	ALTENTRY(klock_require)
	sethi	%hi(_klock), %o3
	ld	[%o3+%lo(_klock)], %o3	! read lock word
	GETKLV(%o5, %o4)
	cmp	%o4, %o3		! compare for ownership
	bne	_klock_reqfail
 	nop
	retl
	nop

	ALTENTRY(klock_steal)
	sethi	%hi(_klock), %o1
	GETKLV(%o2, %o0)		! who am i?
	swap	[%o1+%lo(_klock)], %o0	! swap lock
	set	_4m_itr_pa, %o1
	and	%o0, 3, %o2
	sta	%o2, [%o1]ASI_CTL	! steal ITR
	retl
	lda	[%o1]ASI_CTL, %g0	! interlock ITR
