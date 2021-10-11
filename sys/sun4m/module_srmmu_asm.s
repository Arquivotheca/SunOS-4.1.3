/*
 *	@(#)module_srmmu_asm.s 1.1 92/07/30 SMI
 *	Copyright (c) 1990 by Sun Microsystems, Inc.
 *
 * assembly code interface to flexible module routines
 */

#include <sys/param.h>
#include <machine/asm_linkage.h>
#include <machine/mmu.h>
#include <machine/pte.h>
#include <machine/cpu.h>
#include <machine/psl.h>
#include <machine/trap.h>
#include <machine/devaddr.h>
#ifdef	MULTIPROCESSOR
#include <percpu_def.h>
#endif	MULTIPROCESSOR
#include "assym.s"

	.seg	"text"
	.align	4

	ALTENTRY(srmmu_mmu_getcr)				! int	mmu_getcr(void);
	set	RMMU_CTL_REG, %o1			! get srmmu control register
	retl
	lda	[%o1]ASI_MOD, %o0

	ALTENTRY(srmmu_mmu_getctp)				! int	mmu_getctp(void);
	set	RMMU_CTP_REG, %o1			! get srmmu context table ptr
	retl
	lda	[%o1]ASI_MOD, %o0

	ALTENTRY(srmmu_mmu_getctx)				! int	mmu_getctx(void);
	set	RMMU_CTX_REG, %o1			! get srmmu context number
	retl
	lda	[%o1]ASI_MOD, %o0

	ALTENTRY(srmmu_mmu_probe)
	and	%o0, MMU_PAGEMASK, %o0			! virtual page number
	or	%o0, FT_ALL<<8, %o0			! match criteria
	retl
	lda	[%o0]ASI_FLPR, %o0

	ALTENTRY(srmmu_mmu_setcr)				! void	srmmu_mmu_setcr(int v);
	set	RMMU_CTL_REG, %o1			! set srmmu control register
	retl
	sta	%o0, [%o1]ASI_MOD

	ALTENTRY(srmmu_mmu_setctp)				! void	srmmu_mmu_setctp(int v);
	set	RMMU_CTP_REG, %o1			! set srmmu context table pointer
	retl
	sta	%o0, [%o1]ASI_MOD

	ALTENTRY(srmmu_mmu_setctx)				! void	srmmu_mmu_setctx(int v);
	set	RMMU_CTX_REG, %o5			! set srmmu context number
	retl
	sta	%o0, [%o5]ASI_MOD

	ALTENTRY(srmmu_mmu_flushall)			! void	srmmu_mmu_flushall(void)
	or	%g0, FT_ALL<<8, %o0			! flush entire mmu
	set	RMMU_CTX_REG, %o4			! flush mapping from context in mmu
	retl
	sta	%g0, [%o0]ASI_FLPR			! do the flush

/*
 * BORROW_CONTEXT: temporarily set the context number
 * to that given in %o1.
 * 
 * NOTE: if the kernel does not appear to be mapped into the requested
 * context, then the operation is done in the current context instead.
 * This may or may not be the most efficient thing to do, but it is
 * the safest.
 * 
 * NOTE: traps are disabled while we are in a borrowed context. It is
 * not possible to prove that the only traps that can happen while the
 * context is borrowed are safe to activate while we are in a borrowed
 * context (this includes random level-15 interrupts!).
 * 
 * %o0	flush/probe address [don't touch!]
 * %o1	context number to borrow
 * %o2	saved context number
 * %o3		[[appears to be free]]
 * %o4	psr temp / probe temp / RMMU_CTX_REG
 * %o5	saved psr
 *
 * The probe sequence (middle block) is documented more clearly in
 * the routine "mmu_flushctx".
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
	lda	[%o4]ASI_MEM, %o4;	\
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

	ALTENTRY(srmmu_mmu_flushctx)			! void	srmmu_mmu_flushctx(int ctx);
	set	FT_CTX<<8, %o0
	BORROW_CONTEXT
	sta	%g0, [%o0]ASI_FLPR			! do the flush
	RESTORE_CONTEXT
	retl
	nop		! psr delay

	ALTENTRY(srmmu_mmu_flushrgn)			! void	srmmu_mmu_flushrgn(caddr_t base);
	b	srmmu_flushcommon			! flush region in context from mmu
	or	%o0, FT_RGN<<8, %o0

	ALTENTRY(srmmu_mmu_flushseg)			! void	srmmu_mmu_flushseg(caddr_t base);
	b	srmmu_flushcommon			! flush segment in context from mmu
	or	%o0, FT_SEG<<8, %o0

	ALTENTRY(srmmu_mmu_flushpage)			! void	srmmu_mmu_flushpage(caddr_t base)
	or	%o0, FT_PAGE<<8, %o0

srmmu_flushcommon:
#ifdef	MULTIPROCESSOR
	BORROW_CONTEXT
#endif	MULTIPROCESSOR
	sta	%g0, [%o0]ASI_FLPR			! do the flush
#ifdef	MULTIPROCESSOR
	RESTORE_CONTEXT
#endif	MULTIPROCESSOR
	retl
	nop		! PSR or MMU delay.

	ALTENTRY(srmmu_mmu_flushpagectx)			! void	srmmu_mmu_flushpagectx(caddr_t base, int ctx)
	or	%o0, FT_PAGE<<8, %o0
	BORROW_CONTEXT
	sta	%g0, [%o0]ASI_FLPR			! do the flush
	RESTORE_CONTEXT
	retl
	nop		! PSR or MMU delay.

	!
	! BE CAREFUL - register useage must correspond to code
	! in locore.s that calls this routine!

	! See comments for "mmu_getsyncflt" for allocation information.

	ALTENTRY(srmmu_mmu_getsyncflt)
	set	RMMU_FAV_REG, %g7
	lda	[%g7]ASI_MOD, %g7
	set	RMMU_FSR_REG, %g6
	retl
	lda	[%g6]ASI_MOD, %g6

	ALTENTRY(srmmu_mmu_getasyncflt)
	set	RMMU_AFA_REG, %o1
	lda	[%o1]ASI_MOD, %o1
	st	%o1, [%o0+4]
	set	RMMU_AFS_REG, %o1
	lda	[%o1]ASI_MOD, %o1
	st	%o1, [%o0]
	set	-1, %o1
	retl
	st	%o1, [%o0+8]

	ALTENTRY(srmmu_mmu_chk_wdreset)
	set	RMMU_RST_REG, %o1		! D-side reset reg.
	lda	[%o1]ASI_MOD, %o0
	retl
	and	%o0, RSTREG_WD, %o0		! mask WD reset bit


	! see "mmu_sys_ovf" for constraints.

	ALTENTRY(srmmu_mmu_sys_ovf)
	set	RMMU_FSR_REG, %g1	! clear any old faults out
	lda	[%g1]ASI_MOD, %g0	! of the SFSR.

	set	RMMU_CTL_REG, %g2
	lda	[%g2]ASI_MOD, %g1	! turn on no-fault bit in
	or	%g1, MCR_NF, %g1	! mmu control register to
	sta	%g1, [%g2]ASI_MOD	! prevent taking a fault.

	SAVE_WINDOW(%sp)		! try to save reg window

	andn	%g1, MCR_NF, %g1	! turn off no-fault bit
	sta	%g1, [%g2]ASI_MOD

	set	RMMU_FAV_REG, %g2
	lda	[%g2]ASI_MOD, %g2	! read SFAR
	set	RMMU_FSR_REG, %g1
	lda	[%g1]ASI_MOD, %g1	! read SFSR

	b,a	st_chk_flt		! continue caller


	! see "mmu_sys_unf" for constraints.

	ALTENTRY(srmmu_mmu_sys_unf)
	set	RMMU_FSR_REG, %g1	! clear any old faults out
	lda	[%g1]ASI_MOD, %g0	! of the SFSR.

	set	RMMU_CTL_REG, %g2
	lda	[%g2]ASI_MOD, %g1	! turn on no-fault bit in
	or	%g1, MCR_NF, %g1	! mmu control register to
	sta	%g1, [%g2]ASI_MOD	! prevent taking a fault

	restore
	RESTORE_WINDOW(%sp)		! try to restore reg window
	save

	andn	%g1, MCR_NF, %g1	! turn off no-fault bit
	sta	%g1, [%g2]ASI_MOD

	set	RMMU_FAV_REG, %g2
	lda	[%g2]ASI_MOD, %g2	! read SFAR
	set	RMMU_FSR_REG, %g1
	lda	[%g1]ASI_MOD, %g1	! read SFSR

	b,a	sr_chk_flt		! continue caller

	! see mmu_wo for constraints

	ALTENTRY(srmmu_mmu_wo)
	set	RMMU_FSR_REG, %g1	! clear any old faults out
	lda	[%g1]ASI_MOD, %g0	! of the SFSR.

	set	RMMU_CTL_REG, %g2
	lda	[%g2]ASI_MOD, %g1	! turn on no-fault bit in
	or	%g1, MCR_NF, %g1	! mmu control register to
	sta	%g1, [%g2]ASI_MOD	! prevent taking a fault.

	SAVE_WINDOW(%sp)		! try to save reg window

	andn	%g1, MCR_NF, %g1	! turn off no-fault bit
	sta	%g1, [%g2]ASI_MOD

	set	RMMU_FAV_REG, %g2
	lda	[%g2]ASI_MOD, %g2	! read SFAR
	set	RMMU_FSR_REG, %g1
	lda	[%g1]ASI_MOD, %g1	! read SFSR

	b,a	wo_chk_flt		! continue caller

	ALTENTRY(srmmu_mmu_wu)
	! GENERIC SRMMU mmu module specific part of the code
	! used by underflow.s to handle a register window
	! underflow.
	! NOTE that this routine is jumped to, and that it jumps
	! back to a fixed location.
	! BE CAREFUL WITH REGISTER USEAGE -- REGISTERS MUST
	! CORRESPOND TO underflow.s CODE THAT JUMPS
	! HERE.
	!
	set	RMMU_FSR_REG, %l5
	lda	[%l5]ASI_MOD, %g0	! clear SFSR

	set	RMMU_CTL_REG, %l5
	lda	[%l5]ASI_MOD, %l4
	or	%l4, MCR_NF, %l4	! set no-fault bit in
	sta	%l4, [%l5]ASI_MOD	! control register

	RESTORE_WINDOW(%sp)
	save
	save

	set	RMMU_CTL_REG, %l5
	lda	[%l5]ASI_MOD, %l4
	andn	%l4, MCR_NF, %l4	! clear no-fault bit
	sta	%l4, [%l5]ASI_MOD	! in control register

	set	RMMU_FAV_REG, %l5
	lda	[%l5]ASI_MOD, %l5	! read SFAR
	set	RMMU_FSR_REG, %l4
	lda	[%l4]ASI_MOD, %l4	! read SFSR

	b	wu_chk_flt		! proceed with underflow
	nop
