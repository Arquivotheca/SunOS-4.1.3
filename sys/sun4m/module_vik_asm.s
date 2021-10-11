/*
 *	@(#)module_vik_asm.s 1.1 92/07/30 SMI
 *	Copyright (c) 1990 by Sun Microsystems, Inc.
 *
 * assembly code support for modules based on the
 * TI VIKING chip set.
 */

#include <sys/param.h>
#include <machine/asm_linkage.h>
#include <machine/mmu.h>
#include <machine/pte.h>
#include <machine/cpu.h>
#include <machine/psl.h>
#include <machine/trap.h>
#include <machine/devaddr.h>
#include <machine/module.h>
#include <percpu_def.h>
#include "assym.s"

	ALTENTRY(vik_mmu_getasyncflt)
! %%%	need to work out how to talk with
! %%%	viking write pipe.
! get mfsr/mfar registers
! Viking/NE: a store buffer error will cause both a trap 0x2b and
! 	     a broadcast l15 interrupt. The trap will be taken first, but
! 	     afterwards there will be a pending l15 interrupt waiting for
! 	     this module. 
! Viking/E:  async fault status is in the mxcc error register
! 
! %o0    = MFSR
! %o0+4  = MFAR
! %o0+8  = -1 in Mbus mode;
!	 = Error register <63:32>
! %o0+12 = Error register <31:0>
!

	set	RMMU_FAV_REG, %o1		! MFSR and MFAR
	lda	[%o1]ASI_MOD, %o1
	st	%o1, [%o0+4]
	set	RMMU_FSR_REG, %o1
	lda	[%o1]ASI_MOD, %o1
	st	%o1, [%o0]

	sethi	%hi(_mod_info), %o1
	ld	[%o1+%lo(_mod_info)], %o1	! get mod_info[0].mod_type
	cmp	%o1, CPU_VIKING			! in MBus mode?
	be	1f
	set	-1, %o4

	set	MXCC_ERROR, %o1
	ldda	[%o1]ASI_MXCC, %o2		! get 64-bit error register
	st	%o2, [%o0+12]			! %o2 has PA<31 : 0>
	mov	%o3, %o4 			! %o3 (high nibble) status
	sub	%g0, 1, %o2			! set 0xFFFFFFFF to %o2
	sub	%g0, 1, %o3			! set 0xFFFFFFFF to %o3
	stda	%o2, [%o1]ASI_MXCC		! clear error register
1:
	retl
	st	%o4, [%o0+8]

	ALTENTRY(vik_mmu_chk_wdreset)
	set	RMMU_FSR_REG, %o0
	lda	[%o0]ASI_MOD, %o0
	set	MMU_SFSR_EM, %o1
	retl
	and	%o0, %o1, %o0

	ENTRY(probe_pte_0)

	set	0xFFF, %o1
	andn	%o0, %o1, %o0
	retl
	lda	[%o0]ASI_FLPR, %o0

	ENTRY(probe_1)

	set	0xFFF, %o1
	andn	%o0, %o1, %o0
	set	0x100, %o1
	or	%o0, %o1, %o0
	retl
	lda	[%o0]ASI_FLPR, %o0


	ENTRY(probe_2)

	set	0xFFF, %o1
	andn	%o0, %o1, %o0
	set	0x200, %o1
	or	%o0, %o1, %o0
	retl
	lda	[%o0]ASI_FLPR, %o0

	ENTRY(probe_3)

	set	0xFFF, %o1
	andn	%o0, %o1, %o0
	set	0x300, %o1
	or	%o0, %o1, %o0
	retl
	lda	[%o0]ASI_FLPR, %o0

/*
 * 	Breakpoint register
 *	set or unset bits
 * 	bpt_reg(onbits, offbits)
 */
	ENTRY(bpt_reg)
	lda	[%g0]ASI_MBAR, %o2	/* get the contents */
	or	%o0, %o2, %o0		/* set the "onbits" bits */

	andn	%o0, %o1, %o0		/* reset th "offbits" bits */

	retl
	sta	%o0, [%g0]ASI_MBAR

/*
 * vik_pte_rmw: update a pte.
 * does no flushing.
 *
 * breaks if we update the m-bit
 * between storing the new data
 * and flusing our tlb, so we
 * disable traps and do no stores.
 *
 * %o0 - virtual address of pte
 * %o1 - bits to turn off
 * %o2 - bits to turn on
 * %o3 - flush address
 * %o4 - 
 * %o5 - pte value
 */
	ALTENTRY(vik_pte_rmw)		! (ppte, aval, oval)
	mov	%psr, %g2
	andn	%g2, PSR_ET, %g1
	mov	%g1, %psr		! disable traps
	nop; nop; nop			! psr delay
	ld	[%o0], %o5		! get old value
	andn	%o5, %o1, %o5		! turn some bits off
	or	%o5, %o2, %o5		! turn some bits on
	swap	[%o0], %o5		! update the PTE
	mov	%g2, %psr		! restore traps
	nop				! psr delay
	retl
	mov	%o5, %o0

#ifdef	VAC
/*
 *	Actually, this is a PAC, not a VAC,
 *	but this seems to be the right place
 *	to put the code. Note: it is extremely
 *	rare to need to flush physical caches,
 *	like just when going from cacheable to
 *	noncacheable. It is also fairly hard
 *	to flush the viking cache.
 */
	ALTENTRY(vik_vac_init_asm)	! (clrbits, setbits)

/*
 *      Don't clear the tags here. The viking bootprom will clear the cache
 *      tags and leave the cache on.
 *      If we clear the tags here, all hell will break loose.
 *
 *      sub     %g0, 1, %g1
 *      sta     %g1, [%g0]ASI_ICFCLR    ! clear i-$ lock bits
 *      sta     %g0, [%g0]ASI_ICFCLR    ! clear i-$ valid bits
 *      sta     %g1, [%g0]ASI_DCFCLR    ! clear d-$ lock bits
 *      sta     %g0, [%g0]ASI_DCFCLR    ! clear d-$ valid bits
 */

	set	RMMU_CTL_REG, %o5	! initialize viking
	lda	[%o5]ASI_MOD, %o4	! read control register
	andn	%o4, %o0, %o4		! turn some bits off
	or	%o4, %o1, %o4		! turn some bits on
	sta	%o4, [%o5]ASI_MOD	! update control register

	retl
	nop

/* %%% TODO: initialize MXCC
 *		- write 0xFFFFFFFFFFFFFFFF to mxcc error register.  individual
 *		  bits in this register are write-1-to-clear.  This register
 *		  is not affected by system reset.
 *		- initialize MXCC control register
 *		  CE <bit 2>- E$ enable.
 *		  PE <bit 3>- Parity enable.
 *		  MC <bit 4>- Multiple command enable
 *		  PF <bit 5>- Prefetch enable
 *		  RC <bit 9>- Read reference count only
 */
	ALTENTRY(vik_mxcc_init_asm)	! (clrbits, setbits)
	set	MXCC_ERROR, %o4
	sub	%g0, 1, %o2		! set 0xFFFFFFFFFFFFFFFF to %o2 and %o3
	sub	%g0, 1, %o3		! set 0xFFFFFFFFFFFFFFFF to %o2 and %o3
	stda	%o2, [%o4]ASI_MXCC

	set	MXCC_CNTL, %o4
	lda	[%o4]ASI_MXCC, %o5	! read mxcc control reg
	andn	%o5, %o0, %o5		! turn some bits off
	or	%o5, %o1, %o5		! turn some bits on
	sta	%o5, [%o4]ASI_MXCC	! update mxcc control reg

	retl
	nop

	ALTENTRY(vik_cache_on)
! %%% what do we stuff in here?
!     do we need to turn on MXCC here?
	set	CACHE_VIK_ON, %o2		!MBus mode
	set	RMMU_CTL_REG, %o0
	lda	[%o0]ASI_MOD, %o1
	andcc	%o1, CPU_VIK_MB, %g0
	bnz	1f				!MBus mode
	nop
	set	CACHE_VIK_ON_E, %o2		!CC mode
1:	or	%o1, %o2, %o1
	sta	%o1, [%o0]ASI_MOD
	retl
	nop

	ALTENTRY(mxcc_vac_parity_chk_dis)
	set	MMU_SFSR_UD, %o3
	btst	%o3, %o0
	bnz	1f
	set	MMU_SFSR_P, %o4
	btst	%o4, %o0
	bnz	1f
	set	MXCC_ERR_CP, %o5
	btst	%o5, %o1
	bnz	1f
	nop

	retl
	mov	%g0, %o0
1:
	set	MXCC_CNTL, %o1
	lda	[%o1]ASI_MXCC, %o2	! read mxcc control reg
	set	MXCC_PE, %o3
	andn	%o3, %o2, %o2

	set	RMMU_CTL_REG, %o3	! initialize viking
	lda	[%o3]ASI_MOD, %o4	! read control register
	set	CPU_VIK_PE, %o5
	andn	%o5, %o4, %o4

	sta	%o2, [%o1]ASI_MXCC	! Turn off E$ parity bit
	sta	%o4, [%o3]ASI_MOD	! Turn off Viking parity bit

	retl
	mov	0x1, %o0
#endif	VAC

/*
 * Taken from module_srmmu_asm.s in order to handle the manipulation
 * of the AC bit for Viking/MXCC.  Only this module should now use
 * these routines.  All others, including VIKING/NE should still
 * point to the "srmmu_XXX" routines.
 * %%% Maybe we dont need to do the ASI_MEM at all within BORROW_CONTEXT?
 *
 * BORROW_CONTEXT: temporarily set the context number
 * to that given in %o1.
 * 
 * NOTE: traps are disabled while we are in a borrowed context. It is
 * not possible to prove that the only traps that can happen while the
 * context is borrowed are safe to activate while we are in a borrowed
 * context (this includes random level-15 interrupts!).
 * 
 * %o0	flush/probe address [don't touch!]
 * %o1	context number to borrow
 * %o2	saved context number
 * %o3	saved mmu csr contents
 * %o4	psr temp / probe temp / RMMU_CTX_REG
 * %o5	saved psr
 *
 */

#define BORROW_CONTEXT			\
	mov	%psr, %o5;		\
	andn	%o5, PSR_ET, %o4;	\
	mov	%o4, %psr;		\
	nop ;	nop;			\
\
	set	RMMU_CTP_REG, %o4;	\
	lda	[%o4]ASI_MOD, %o4;	\
	sll	%o1, 2, %o2;		\
	sll	%o4, 4, %o4;		\
	add	%o4, %o2, %o4;		\
\
	set	CPU_VIK_AC, %o2;	\
	lda	[%g0]ASI_MOD, %o3;	\
	or	%o3, %o2, %o2;		\
	sta	%o2, [%g0]ASI_MOD;	\
\
	lda	[%o4]ASI_MEM, %o4;	\
	sta	%o3, [%g0]ASI_MOD;	\
	and	%o4, 3, %o4;		\
	subcc	%o4, MMU_ET_PTP, %g0;	\
\
	set	RMMU_CTX_REG, %o4;	\
	bne	1f;			\
	lda	[%o4]ASI_MOD, %o2;	\
	sta	%o1, [%o4]ASI_MOD;	\
1:

/*
 * RESTORE_CONTEXT: back out from whatever BORROW_CONTEXT did.
 * Assumes that two cycles of PSR DELAY follow.
 */
#define RESTORE_CONTEXT			\
	sta	%o2, [%o4]ASI_MOD;	\
	mov	%o5, %psr;		\
	nop

	ALTENTRY(vik_mmu_flushctx)			! void	vik_mmu_flushctx(int ctx);
	set	FT_CTX<<8, %o0
	BORROW_CONTEXT
	sta	%g0, [%o0]ASI_FLPR			! do the flush
	RESTORE_CONTEXT
	retl
	nop		! psr delay

	ALTENTRY(vik_mmu_flushrgn)			! void	vik_mmu_flushrgn(caddr_t base);
	b	vik_flushcommon			! flush region in context from mmu
	or	%o0, FT_RGN<<8, %o0

	ALTENTRY(vik_mmu_flushseg)			! void	vik_mmu_flushseg(caddr_t base);
	b	vik_flushcommon			! flush segment in context from mmu
	or	%o0, FT_SEG<<8, %o0

	ALTENTRY(vik_mmu_flushpage)			! void	vik_mmu_flushpage(caddr_t base)
	or	%o0, FT_PAGE<<8, %o0

vik_flushcommon:
#ifdef	MULTIPROCESSOR
	BORROW_CONTEXT
#endif	MULTIPROCESSOR
	sta	%g0, [%o0]ASI_FLPR			! do the flush
#ifdef	MULTIPROCESSOR
	RESTORE_CONTEXT
#endif	MULTIPROCESSOR
	retl
	nop		! PSR or MMU delay.

	ALTENTRY(vik_mmu_flushpagectx)		! void
						! vik_mmu_flushpagectx (caddr_t
						!		base, int ctx)
	or	%o0, FT_PAGE<<8, %o0
	BORROW_CONTEXT
	sta	%g0, [%o0]ASI_FLPR			! do the flush
	RESTORE_CONTEXT
	retl
	nop		! PSR or MMU delay.
