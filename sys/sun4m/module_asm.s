/*
 *	@(#)module_asm.s 1.1 92/07/30 SMI
 *	Copyright (c) 1990 by Sun Microsystems, Inc.
 *
 * assembly code support for dynamicly selectable modules
 */

#include <sys/param.h>
#include <machine/asm_linkage.h>
#include <machine/mmu.h>
#include <machine/pte.h>
#include <machine/cpu.h>
#include <machine/psl.h>
#include <machine/trap.h>
#include <machine/devaddr.h>
#include "assym.s"

	.seg	"text"
	.align	4

#ifndef	MULTIPROCESSOR
#define	READCXN(r)
#else	MULTIPROCESSOR
#define	READCXN(r)				\
	set	RMMU_CTX_REG, r			; \
	lda	[r]ASI_MOD, r
#endif	MULTIPROCESSOR

#define	REVEC(name, r)				\
	sethi	%hi(_v_/**/name), r		; \
	ld	[r+%lo(_v_/**/name)], r		; \
	jmp	r				; \
	nop

	ENTRY(mmu_getcr)
	REVEC(mmu_getcr, %o5)

	ENTRY(mmu_getctp)
	REVEC(mmu_getctp, %o5)

	ENTRY(mmu_getctx)
	REVEC(mmu_getctx, %o5)

	ENTRY(mmu_probe)
	REVEC(mmu_probe, %o5)

	ENTRY(mmu_setcr)
	REVEC(mmu_setcr, %o5)

	ENTRY(mmu_setctp)
	REVEC(mmu_setctp, %o5)

	ENTRY(mmu_setctx)
	REVEC(mmu_setctx, %o5)

	ENTRY(mmu_flushall)
	REVEC(mmu_flushall, %o5)

	ENTRY(mmu_flushctx)
!
! If the specified context has not been mapped,
! don't bother doing the call.
!
	set	_contexts, %o1
	sll	%o0, 2, %o2				! offset is four bytes
							! per entry
	add	%o1, %o2, %o1				! context number ...
	ld	[%o1], %o1				! grab the LOPTP
	and	%o1, 3, %o1				! shave off teh type
	cmp	%o1, MMU_ET_PTP				! is it a PTP?
	beq	1f					! if so, go ahead
	mov	%o0, %o1				! dup ctx to expected
							! loc
	retl						! otherwise, do nothing
	nop
1:
	REVEC(mmu_flushctx, %o5)

	ENTRY(mmu_flushrgn)
	srl	%o0, MMU_STD_RGNSHIFT, %o0 ! round address to
	sll	%o0, MMU_STD_RGNSHIFT, %o0 ! base of region.
	READCXN(%o1)
	REVEC(mmu_flushrgn, %o5)

	ENTRY(mmu_flushseg)
	srl	%o0, MMU_STD_SEGSHIFT, %o0 ! round address to
	sll	%o0, MMU_STD_SEGSHIFT, %o0 ! base of segment.
	READCXN(%o1)
	REVEC(mmu_flushseg, %o5)

	ENTRY(mmu_flushpage)
	srl	%o0, MMU_STD_PAGESHIFT, %o0 ! round address to
	sll	%o0, MMU_STD_PAGESHIFT, %o0 ! base of page.
	READCXN(%o1)
	REVEC(mmu_flushpage, %o5)

	ENTRY(mmu_flushpagectx)
	srl	%o0, MMU_STD_PAGESHIFT, %o0 ! round address to
	sll	%o0, MMU_STD_PAGESHIFT, %o0 ! base of page.
	REVEC(mmu_flushpagectx, %o5)


	! BE CAREFUL - register useage must correspond to code
	! in locore.s that calls this routine!

	! AT THIS TIME, on entry, %g6 holds the text/data information,
	! %o7 holds our return address, and %g7 is available; we
	! return with %g6 set to the sync fault status register and
	! %g7 set to the sync fault address register. Check the caller
	! to verify these register assignments.

	ALTENTRY(mmu_getsyncflt)
	REVEC(mmu_getsyncflt, %g7)

	! Passed a ptr to where to store asynchronous fault reg contents
	! Stored in the following order:
	! AFSR (lowest MID master)
	! AFAR (lowest MID master)
	! AFSR (highest MID master) -- -1 if not defined
	! AFAR (highest MID master)
	ALTENTRY(mmu_getasyncflt)
	REVEC(mmu_getasyncflt, %o5)

	ALTENTRY(mmu_chk_wdreset)
	REVEC(mmu_chk_wdreset, %o5)

	ALTENTRY(mmu_log_module_err)
	REVEC(mmu_log_module_err, %o5)

	ALTENTRY(mmu_print_sfsr)
	REVEC(mmu_print_sfsr, %o5)

	! The following four routines (mmu_sys_ovf, mmu_sys_unf, 
	! mmu_wo, and mmu_wu) implement MMU module specific
	! operations for register window overflow/underflow
	! trap handling.

	! MMU module dependent part of sys_trap window overflow
	! handling. 

	! mmu_sys_ovf: module specific window overflow code used
	! by locore.s(st_sys_ovf) to handle window overflow code.

	! NOTE: entry is direct, not via call; return by branching
	! directly to st_chk_flt

	! NOTE: register usage is highly constrained. Verify usage
	! against caller named above.

	ALTENTRY(mmu_sys_ovf)
	REVEC(mmu_sys_ovf, %g1)

	! MMU module specific part of the code used by trap return
	! code (locore.s) to handle a register window underflow.

	! NOTE that this routine is jumped to, and that it jumps back
	! to a fixed location.

	! BE CAREFUL WITH REGISTER USAGE -- REGISTERS MUST CORRESPOND
	! TO trap return (locore.s) CODE THAT JUMPS HERE.

	ALTENTRY(mmu_sys_unf)
	REVEC(mmu_sys_unf, %g1)

	! MMU module dependent part of window overflow handling.
	! see overflow.s.

	! NOTE that this routine is jumped to, and that it jumps back
	! to a fixed location.

	! BE CAREFUL WITH REGISTER USEAGE -- REGISTERS MUST CORRESPOND
	! TO overflow.s CODE THAT JUMPS HERE.

	ALTENTRY(mmu_wo)
	REVEC(mmu_wo, %g1)


	! MMU module dependent part of window underflow handling.
	! see underflow.s

	ALTENTRY(mmu_wu)
	REVEC(mmu_wu, %l4)

	ALTENTRY(pte_offon)
	REVEC(pte_offon, %g1)

	ALTENTRY(module_wkaround)
	REVEC(module_wkaround, %g1)

#ifdef	VAC

	ENTRY(vac_noop)
	retl
	nop

	ENTRY(vac_inoop)
	retl
	mov %g0, %o0

	ENTRY(vac_init)
	REVEC(vac_init, %g1)

	ENTRY(vac_flushall)
	sethi	%hi(_vac), %g1
	ld	[%g1 + %lo(_vac)], %g1
	tst	%g1			! check if cache is turned on
	bz	_vac_noop
	nop

	REVEC(vac_flushall, %g1)

	ENTRY(vac_usrflush)
	sethi	%hi(_vac), %g1
	ld	[%g1 + %lo(_vac)], %g1
	tst	%g1			! check if cache is turned on
	bz	_vac_noop
	nop

	set	_flush_cnt, %o5
	ld	[%o5 + FM_USR], %g1
	inc	%g1			! increment flush count
	st	%g1, [%o5 + FM_USR]

	REVEC(vac_usrflush, %g1)

! void vac_ctxflush(ctx)	flush user pages in context from cache

	ENTRY(vac_ctxflush)
	! this part of the code won't be used for viking
	! the code changes here is just to eliminate the use of ASI_MEM
	! we should be able to use "contexts" to get the ctx
	sethi	%hi(_vac), %g1
	ld	[%g1 + %lo(_vac)], %g1
	tst	%g1			! check if cache is turned on
	bz	_vac_noop
	nop

	set	_contexts, %o1
	sll	%o0, 2, %o2
	add	%o1, %o2, %o1
	ld	[%o1], %o1		! check if ctx has kernel mapped
	and	%o1, 3, %o1
	subcc	%o1, MMU_ET_PTP, %g0
	bne	_vac_noop
	nop

	set	_flush_cnt, %o5
	ld	[%o5 + FM_CTX], %g1
	inc	%g1			! increment flush count
	st	%g1, [%o5 + FM_CTX]

	mov	%o0, %o2		! dup ctx number to expected place

	REVEC(vac_ctxflush, %g1)

! void vac_rgnflush(va)		flush rgn [in current context or supv] from cache

	ENTRY(vac_rgnflush)
	sethi	%hi(_vac), %g1
	ld	[%g1 + %lo(_vac)], %g1
	tst	%g1			! check if cache is turned on
	bz	_vac_noop
	nop

	set	_flush_cnt, %o5
	ld	[%o5 + FM_REGION], %g1
	inc	%g1			! increment flush count
	st	%g1, [%o5 + FM_REGION]

	srl	%o0, MMU_STD_RGNSHIFT, %o0 ! round address to
	sll	%o0, MMU_STD_RGNSHIFT, %o0 ! base of region

	READCXN(%o2)
	REVEC(vac_rgnflush, %g1)

! void vac_segflush(va)		flush seg [in current context or supv] from cache

	ENTRY(vac_segflush)
	sethi	%hi(_vac), %g1
	ld	[%g1 + %lo(_vac)], %g1
	tst	%g1			! check if cache is turned on
	bz	_vac_noop
	nop

	set	_flush_cnt, %o5
	ld	[%o5 + FM_SEGMENT], %g1
	inc	%g1			! increment flush count
	st	%g1, [%o5 + FM_SEGMENT]

	READCXN(%o2)
	REVEC(vac_segflush, %g1)

! void vac_pageflush(va)	flush page [in current context or supv] from cache

	ENTRY(vac_pageflush)
	sethi	%hi(_vac), %g1
	ld	[%g1 + %lo(_vac)], %g1
	tst	%g1			! check if cache is turned on
	bz	_vac_noop
	nop

	set	_flush_cnt, %g1
	ld	[%g1 + FM_PAGE], %o5
	inc	%o5			! increment flush count
	st	%o5, [%g1 + FM_PAGE]

	srl	%o0, MMU_STD_PAGESHIFT, %o0 ! round address to
	sll	%o0, MMU_STD_PAGESHIFT, %o0 ! base of page

	READCXN(%o2)
	REVEC(vac_pageflush, %g1)

! void vac_pagectxflush(va, ctx)	! flush page in ctx [or supv] from cache

	ENTRY(vac_pagectxflush)
	sethi	%hi(_vac), %g1
	ld	[%g1 + %lo(_vac)], %g1
	tst	%g1			! check if cache is turned on
	bz	_vac_noop
	nop

	set	_flush_cnt, %g1
	ld	[%g1 + FM_PAGE], %o5
	inc	%o5			! increment flush count
	st	%o5, [%g1 + FM_PAGE]

	mov	%o1, %o2	! dup ctx; easiest if it is both 2nd and 3rd arg.

	srl	%o0, MMU_STD_PAGESHIFT, %o0 ! round address to
	sll	%o0, MMU_STD_PAGESHIFT, %o0 ! base of page.

	REVEC(vac_pagectxflush, %g1)

! void vac_flush(va, sz)	flush range [in current context or supv] from cache

	ENTRY(vac_flush)
	sethi	%hi(_vac), %g1
	ld	[%g1 + %lo(_vac)], %g1
	tst	%g1			! check if cache is turned on
	bz	_vac_noop
	nop

	set	_flush_cnt, %g1
	ld	[%g1 + FM_PARTIAL], %o5
	inc	%o5			! increment flush count
	st	%o5, [%g1 + FM_PARTIAL]

	READCXN(%o2)
	REVEC(vac_flush, %g1)

! void cache_on()		enable the cache

	ENTRY(cache_on)
	REVEC(cache_on, %g1)

! int vac_parity_chk_dis()		check parity, disable if necessary

	ENTRY(vac_parity_chk_dis)
	sethi	%hi(_vac), %g1
	ld	[%g1 + %lo(_vac)], %g1
	tst	%g1			! check if cache is turned on
	bz	_vac_inoop
	nop

	REVEC(vac_parity_chk_dis, %o5)

#endif	VAC
