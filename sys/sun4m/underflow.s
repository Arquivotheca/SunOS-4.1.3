/*      @(#)underflow.s 1.1 92/07/30 SMI      */

#undef	WU_TRAP_CHECKS_CPUID

/*
 *	Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <sys/resource.h>
#include <machine/asm_linkage.h>
#include <machine/param.h>
#include <machine/psl.h>
#include <machine/trap.h>
#include <machine/mmu.h>
#include <machine/pte.h>
#include <machine/reg.h>
#include <machine/buserr.h>
#include <machine/pcb.h>
#include "percpu_def.h"
#include "assym.s"

	.seg	"text"
	.align	4

/*
 * Window underflow trap handler.
 */
	.global	window_underflow
window_underflow:
	sethi	%hi(_nwindows), %l6
	ld	[%l6+%lo(_nwindows)], %l6
	dec	%l6

#ifdef	WU_TRAP_CHECKS_CPUID
	set	_cpuid, %l5
	lda	[%l5]ASI_FLPR, %l7
	and	%l7, 3, %l7
	cmp	%l7, 2			! cpuid not mapped
	bne	1f
	nop

	sta	%g0, [%l5]ASI_FLPR	! flush tlb
	set	CACHE_LINES*CACHE_LINESZ-1, %l3
	and	%l5, %l3, %l7		! modulo cache size
	set	PAGEMASK, %l3
	and	%l7, %l3, %l7		! round down to base of page
	set	KERNELBASE + CACHE_LINES*CACHE_LINESZ, %l3
	ld	[%l3 + %l7], %g0	! replace the line

	ld	[%l5], %l7
	GETCPU(%l5)
	cmp	%l5, %l7
	bne	_cpuid_bad
	nop

!!! add more tests here, as desired.

1:
#endif	WU_TRAP_CHECKS_CPUID

	mov	%wim, %l3

#ifdef PERFMETER
	sethi	%hi(_underflowcnt), %l5
	ld	[%l5 + %lo(_underflowcnt)], %l7
	inc	%l7
	st	%l7, [%l5 + %lo(_underflowcnt)]
#endif PERFMETER

	! wim stored into %l3 by trap vector
	sll	%l3, 1, %l4		! next WIM = rol(WIM, 1, NW)
	srl	%l3, %l6, %l5		! trap vector set %l6 = NW-1
	or	%l5, %l4, %l5
	mov	%l5, %wim		! install it
	btst	PSR_PS, %l0		! (wim delay 1) test for user trap
	bz	wu_user			! (wim delay 2)
	restore				! delay slot

	!
	! Supervisor underflow.
	! We do one more restore to get into the window to be restored.
	! The first one was done in the delay slot coming here.
	! We then restore from the stack.
	!
	restore				! get into window to be restored
wu_stack_res:
	RESTORE_WINDOW(%sp)
	save				! get back to original window
	save
	mov	%l0, %psr		! reinstall sup PSR_CC
	nop				! psr delay
	jmp	%l1			! reexecute restore
	rett	%l2

wu_user:
	restore				! get into window to be restored
	!
	! In order to restore the window from the stack, the stack
	! must be aligned on a word boundary, and the part of the
	! stack where the restore will be done must be present.
	! We first check for alignment.
	btst	0x7, %sp		! test sp alignment
	bz	wu_assume_stack_res
	nop

	!
	! A user underflow trap has happened with a misaligned sp.
	! Fake a memory alignment trap.
	!
	save				! get back to orig window
	save
	mov	%l3, %wim		! restore old wim, so regs are dumped
	b	sys_trap
	mov	T_ALIGNMENT, %l4	! delay slot, fake alignment trap

	! Most of the time the stack is resident in main memory,
	! so we don't verify its presence before attempting to restore
	! the window from the stack.  Rather, we simply set the
	! no-fault bit of the SRMMU's control register, so that
	! doing the restore won't cause another trap in the case
	! where the stack is not present.  By checking the
	! synchronous fault status register, we can determine
	! whether the restore actually worked (ie. stack was present),
	! or whether we first need to fault in the stack.
	! Other sun4 trap handlers first probe for the stack, and
	! then, if the stack is present, they restore from the stack.
	! This approach CANNOT be used with a multiprocessor system
	! because of a race condition: between the time that the
	! stack is probed, and the restore from the stack is done, the
	! stack could be stolen by the page daemon.
wu_assume_stack_res:

!!! SECURITY NOTE: this per-processor stuff uses supervisor
!!! mode stores, so the MMU never checks user permissions.
!!! So, check here that he is garfing user space.
	set	KERNELBASE, %l4
	cmp	%l4, %sp		! if in supervisor space,
	bleu	wu_stack_not_user	!   branch out to "nonresident"
	nop

	sethi   %hi(_v_mmu_wu), %l4
	ld	[%l4 + %lo(_v_mmu_wu)], %l4
	jmp	%l4
	nop

wu_stack_not_user:
	save
	save
	mov	0x0E, %l4		!   [fake up SFSR]
	b	wu_stack_not_res
	mov	%sp, %l5		!   [fake up SFAR]

	! here is where we return after attempting to restore the
	! underflowed window from the stack.  check to see if the
	! stack was actually present.
	.global	wu_chk_flt
wu_chk_flt:
	btst    MMU_SFSR_FAV, %l4	! did a fault occurr?
	bz      wu_out                  ! no; just clean up and leave.
	nop

wu_stack_not_res:
	!
	! Restore area on user stack is not resident.
	! We punt and fake a page fault so that trap can bring the page in.
	! If the page fault is successful we will reexecute the restore,
	! and underflow with the page now resident.
	!
	sethi	%hi(_masterprocp), %l6
	ld	[%l6 + %lo(_masterprocp)], %l6
	ld	[%l6 + P_STACK], %l7	! setup kernel stack
	SAVE_GLOBALS(%l7 + MINFRAME)

	restore				! back to last user window
	mov	%psr, %g4		! get CWP
	save				! back to trap window

	! now that we are back in the trap window, note that
	! we have access to the following saved values:
	! l7 -- computed sp.
	! l4 -- fault status.
	! l5 -- fault address.

	!
	! save remaining user state
	!
	mov	%l7, %sp		! setup kernel stack
	SAVE_OUTS(%sp + MINFRAME)

	st	%l0, [%sp + MINFRAME + PSR*4] ! psr
	st	%l1, [%sp + MINFRAME + PC*4] ! pc
	st	%l2, [%sp + MINFRAME + nPC*4] ! npc

	mov	%l3, %wim		! reinstall old wim
	mov	1, %g1			! UWM = 0x01 << CWP
	sll	%g1, %g4, %g1
	sethi	%hi(_uunix), %l6
	ld	[%l6 + %lo(_uunix)], %l6
	st	%g1, [%l6 + PCB_UWM]	! setup u.u_pcb.pcb_uwm
	clr	[%l6 + PCB_WBCNT]

#ifdef	PC_cpu_tpsr
	sethi	%hi(_cpu_tpsr), %o0
	st	%l0, [%o0 + %lo(_cpu_tpsr)]
#endif

	wr	%l0, PSR_ET, %psr	! enable traps

	mov	T_DATA_FAULT, %o0
	add	%sp, MINFRAME, %o1
	mov	%l5, %o2		! fault address
	mov	%l4, %o3		! fault status
	call	_trap			! trap(T_DATA_FAULT,
	mov	S_WRITE, %o4		!	rp, addr, be, S_WRITE)

	b,a	sys_rtt

wu_out:
	sethi	%hi(_uunix), %l3
	ld	[%l3 + %lo(_uunix)], %l3
	ld	[%l3+PCB_FLAGS], %l3
	clr	%l5			! interlock slot, clear a used reg
	btst	CLEAN_WINDOWS, %l3	! check for clean window maint.
	bz	1f
	mov	%l0, %psr		! reinstall system PSR_CC

	!
	! Maintain clean windows. We only need to clean the registers
	! used in underflow as we know this is a user window.
	!
	mov	%l1, %o6		! put pc, npc in an unobtrusive place
	mov	%l2, %o7
	clr	%l0			! clean the used ones
	clr	%l1
	clr	%l2
	clr	%l3
	clr	%l4
	jmp	%o6			! reexecute restore
	rett	%o7
1:
	nop
	jmp	%l1			! reexecute restore
	rett	%l2
