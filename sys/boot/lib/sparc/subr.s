/*
 *	.seg    "data"
 *	.asciz  "@(#)subr.s 1.1 92/07/30"
 *	Copyright (c) 1986 by Sun Microsystems, Inc.
 */
	.seg    "text"
	.align  4

#include <machine/asm_linkage.h>
#include <machine/psl.h>
#define PSR_PIL_BIT 8
#define PG_S_BIT 29

#define RAISE(level) \
	b	_splr; \
	mov	(level << PSR_PIL_BIT), %o0

#define SETPRI(level) \
	b	_splx; \
	mov	(level << PSR_PIL_BIT), %o0

	ENTRY(splimp)
	RAISE(6)

	ENTRY(splnet)
	RAISE(1)

	ENTRY2(spl6,spl5)
	SETPRI(12)

	ENTRY(splr)
	mov	%psr, %o1		! get current psr value
	and	%o1, PSR_PIL, %o3	! interlock slot, mask out rest of psr
	cmp	%o3, %o0		! if current pri < new pri, set new pri
	bl	pri_common		! use common code to set priority
	andn	%o1, PSR_PIL, %o2	! zero current priority level
	retl				! return old priority
	mov	%o3, %o0

	ENTRY(splx)
pri_set_common:
	mov     %psr, %o1
	andn    %o1, PSR_PIL, %o2
pri_common:
	and     %o0, PSR_PIL, %o0
	or      %o2, %o0, %o2
	mov     %o2, %psr
	and	%o1, PSR_PIL, %o0
	retl
	nop

/*
 * insque(entryp, predp)
 *
 * Insert entryp after predp in a doubly linked list.
 */
	ENTRY(_insque)
	ld      [%o1], %g1              ! predp->forw
	st      %o1, [%o0 + 4]          ! entryp->back = predp
	st      %g1, [%o0]              ! entryp->forw = predp->forw
	st      %o0, [%o1]              ! predp->forw = entryp
	retl
	st      %o0, [%g1 + 4]          ! predp->forw->back = entryp

/*
 * remque(entryp)
 *
 * Remove entryp from a doubly linked list
 */
	ENTRY(_remque)
	ld      [%o0], %g1              ! entryp->forw
	ld      [%o0 + 4], %g2          ! entryp->back
	st      %g1, [%g2]              ! entryp->back = entryp->forw
	retl
	st      %g2, [%g1 + 4]          ! entryp->forw = entryp->back
