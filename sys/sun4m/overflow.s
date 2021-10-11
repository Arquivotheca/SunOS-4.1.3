/*      @(#)overflow.s 1.1 92/07/30 SMI      */

#undef	WO_TRAP_CHECKS_CPUID

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
#ifdef	MULTIPROCESSOR
#include "percpu_def.h"
#endif	MULTIPROCESSOR
#include "assym.s"

	.seg	"text"
	.align	4

/*
 * window overflow trap handler
 */
	.global	window_overflow
window_overflow:
	sethi	%hi(_nwindows), %l6
	ld	[%l6+%lo(_nwindows)], %l6
	dec	%l6

#ifdef	WO_TRAP_CHECKS_CPUID
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
#endif	WO_TRAP_CHECKS_CPUID

	mov	%wim, %l3

#ifdef PERFMETER
	sethi	%hi(_overflowcnt), %l5
	ld	[%l5 + %lo(_overflowcnt)], %l7
	inc	%l7
	st	%l7, [%l5 + %lo(_overflowcnt)]
#endif PERFMETER

	mov	%g1, %l7		! save %g1
	mov	%g2, %l5		! save %g2
	srl	%l3, 1, %g1		! next WIM = %g1 = ror(WIM, 1, NW)
	sll	%l3, %l6, %l4		! trap vector set %l6 = NW - 1
	btst	PSR_PS, %l0		! test for user or sup trap
	bz	wo_user
	or	%l4, %g1, %g1		! delay slot

	!
	! Overflow from supervisor mode. Determine whether the window
	! to be saved is a user window.
	! U.u_pcb.pcb_uwm has a bit set for each user window which is still
	! in the register file. If u.u_pcb.pcb_uwm has any bits on, then it
	! is a user window that must be saved.
	!
	sethi	%hi(_uunix), %g2	! XXX - global u register?
	ld	[%g2 + %lo(_uunix)], %g2
	ld	[%g2+PCB_UWM], %l4	! if (u.u_pcb.pcb_uwm != 0) sup window
	tst	%l4
	bne,a	wo_user_window
	bclr	%g1, %l4		! delay slot, UWM &= ~(new WIM)

	!
	! Window to be saved is a supervisor window.
	! Put it on the stack.
	!
	save				! get into window to be saved
	mov	%g1, %wim		! install new wim
wo_stack_res:
	SAVE_WINDOW(%sp)
	restore				! go back to trap window
	mov	%l5, %g2		! restore g2
	mov	%l0, %psr		! reinstall system PSR_CC
	mov	%l7, %g1		! restore g1
	jmp	%l1			! reexecute save
	rett	%l2

wo_user_window:
	!
	! Window to be saved is a user window.
	!
	sethi	%hi(_uunix), %g2
	ld	[%g2 + %lo(_uunix)], %g2
	st	%l4, [%g2+PCB_UWM]	! update u.u_uwm

wo_user:
	!
	! The window to be saved is a user window.
	!
	mov	%g3, %l4		! save %g3
	! must execute a save before we install the new wim
	! because, otherwise, the save would cause another
	! window overflow trap (watchdog).  also, must install
	! the new wim here before doing the restore below, or
	! else the restore would cause a window overflow.  these
	! restrictions are responsible for alot of inefficiency
	! in this code.  the situation would be alot better if
	! the locals for the trap window were actually globals.
	save				! get into window to be saved
	mov	%g1, %wim		! install new wim
	!
	! In order to save the window onto the stack, the stack
	! must be aligned on a word boundary, and the part of the
	! stack where the save will be done must be present.
	! We first check for alignment.
	btst	0x7, %sp		! test sp alignment
	bz	wo_assume_stack_res
	nop

	!
	! Misaligned sp. If this is a userland trap fake a memory alignment
	! trap. Otherwise, put the window in the window save buffer so that
	! we can catch it again later.
	!
	mov	%psr, %g1		! get psr (not in trap window)
	btst	PSR_PS, %g1		! test for user or sup trap
	bnz	wo_save_to_buf		! sup trap, save window in uarea buf
	nop

	restore
	mov	%l4, %g3		! restore g3
	mov	%l5, %g2		! restore g2
	mov	%l7, %g1		! restore g1
	mov	%l3, %wim		! restore old wim, so regs are dumped
	b	sys_trap
	mov	T_ALIGNMENT, %l4	! delay slot, fake alignment trap

	! Most of the time the stack is resident in main memory,
	! so we don't verify its presence before attempting to save
	! the window onto the stack.  Rather, we simply set the
	! no-fault bit of the SRMMU's control register, so that
	! doing the save won't cause another trap in the case
	! where the stack is not present.  By checking the 
	! synchronous fault status register, we can determine
	! whether the save actually worked (ie. stack was present),
	! or whether we first need to fault in the stack.
	! Other sun4 trap handlers first probe for the stack, and
	! then, if the stack is present, they store to the stack.
	! This approach CANNOT be used with a multiprocessor system
	! because of a race condition: between the time that the
	! stack is probed, and the store to the stack is done, the
	! stack could be stolen by the page daemon.
wo_assume_stack_res:

!!! SECURITY NOTE: this per-processor stuff uses supervisor
!!! mode stores, so the MMU never checks user permissions.
!!! So, check here that he is garfing user space.
	set	KERNELBASE, %g1
	cmp	%g1, %sp		! if in supervisor space,
	mov	0x8E, %g1		!   [fake up SFSR]
	bleu	wo_stack_not_res	!   branch out to "nonresident"
	mov	%sp, %g2		!   [fake up SFAR]

	sethi   %hi(_v_mmu_wo), %g1
	ld      [%g1 + %lo(_v_mmu_wo)], %g1
	jmp	%g1
	nop
	! here is where we return after attempting to save the
	! overflowed window onto the stack.  check to see if the
	! stack was actually present.
	.global wo_chk_flt
wo_chk_flt:
	btst	MMU_SFSR_FAV, %g1	! did a fault occurr?
	bz	wo_out			! no; just clean up and leave.
	nop
	! a fault occurred, so the stack is not resident.
wo_stack_not_res:
	mov	%psr, %g3		! not in trap window.
	btst	PSR_PS, %g3		! test for user or sup trap
	bnz	wo_save_to_buf		! sup trap, save window in uarea buf
	nop
	!
	! We first save the window in the first window buffer in the u area.
	! Then we fake a user data fault. If the fault succeeds, we will
	! reexecute the save and overflow again, but this time the page
	! will be resident
	sethi	%hi(_uunix), %g3
	ld	[%g3 + %lo(_uunix)], %g3
	st	%sp, [%g3+PCB_SPBUF]	! save sp
	SAVE_WINDOW(%g3+PCB_WBUF)
	restore				! get back into original window
	!
	! Compute the user window mask (u.u_pcb.pcb_uwm), which is a mask of
	! which windows contain user data. In this case it is all the register
	! except the one at the old WIM and the one we just saved.
	!
	mov	%wim, %g3		! get new WIM
	or	%g3, %l3, %g3		! or in old WIM
	not	%g3
	mov	-2, %l3			!note %l3 was freed up above.
	sll	%l3, %l6, %l3
	andn	%g3, %l3, %g3
	sethi	%hi(_uunix), %l3
	ld	[%l3 + %lo(_uunix)], %l3
	st	%g3, [%l3+PCB_UWM]	! u->u_pcb.pcb_uwm = ~(OWIM|NWIM)
	!
	! Set the save buffer ptr to next buffer
	!
	mov	1, %g3
	st	%g3, [%l3+PCB_WBCNT]	! u->u_pcb.pcb_wbcnt = 1
	set	_masterprocp, %g3
	ld	[%g3], %g3
	ld	[%g3 + P_STACK], %sp
	mov	%g1, %l3		! save SFSR
	mov	%g2, %l6		! save SFAR
	mov	%l7, %g1		! restore g1, so we can save it
	mov	%l5, %g2		! restore g2, so we can save it
	mov	%l4, %g3		! restore g3, so we can save it
	SAVE_GLOBALS(%sp + MINFRAME)
	SAVE_OUTS(%sp + MINFRAME)
	st	%l0, [%sp + MINFRAME + PSR*4] ! psr
	st	%l1, [%sp + MINFRAME + PC*4] ! pc
	st	%l2, [%sp + MINFRAME + nPC*4] ! npc

#ifdef	PC_cpu_tpsr
	sethi	%hi(_cpu_tpsr), %o0
	st	%l0, [%o0 + %lo(_cpu_tpsr)]
#endif
	wr	%l0, PSR_ET, %psr	! enable traps

	mov	T_DATA_FAULT, %o0
	add	%sp, MINFRAME, %o1
	mov	%l6, %o2		! fault address.
	mov	%l3, %o3		! fault status.
	call	_trap			! trap(T_DATA_FAULT,
	mov	S_WRITE, %o4		!	rp, addr, be, S_WRITE)

	b,a	sys_rtt			! return

wo_save_to_buf:
	!
	! The user's stack is not accessable while trying to save a user window
	! during a supervisor overflow. We save the window in the u area to
	! be processed when we return to the user.
	!
	sethi	%hi(_uunix), %g2
	ld	[%g2 + %lo(_uunix)], %g2
	ld	[%g2 + PCB_WBCNT], %g1
	sll	%g1, 2, %g1

	add	%g1, %g2, %g1
	st	%sp, [%g1 + PCB_SPBUF] ! save sp
	sub	%g1, %g2, %g1

	sll	%g1, 4, %g1		! convert to offset

	add	%g2, PCB_WBUF, %g2
	add	%g1, %g2, %g1
	SAVE_WINDOW(%g1)
	sub	%g1, %g2, %g1
	sub	%g2, PCB_WBUF, %g2

	srl	%g1, 6, %g1		! increment u.u_pcb.pcb_wbcnt
	add	%g1, 1, %g1

	set	_uunix, %g2
	ld	[%g2], %g2
	st	%g1, [%g2 + PCB_WBCNT]
	restore				! get back to orig window

	!
	! Return to supervisor. Rett will not underflow since traps
	! were never disabled.
	!
	mov	%l0, %psr		! reinstall system PSR_CC
	mov	%l7, %g1		! restore g1
	mov	%l5, %g2		! restore g2
	mov	%l4, %g3		! restore g3

	jmp	%l1			! reexecute save
	rett	%l2

wo_out:
	!
	restore				! get back to orig window
	sethi	%hi(_uunix), %l3
	ld	[%l3 + %lo(_uunix)], %l3
	mov	%l5, %g2		! restore g2
	ld	[%l3+PCB_FLAGS], %l3	! check for clean window maintenance
	mov	%l7, %g1		! restore g1
	mov	%l4, %g3		! restore g3
	btst	CLEAN_WINDOWS, %l3
	bz	1f
	mov	%l0, %psr		! reinstall system PSR_CC

	!
	! Maintain clean windows.
	!
	mov	%l1, %o6		! put pc, npc in an unobtrusive place
	mov	%l2, %o7
	clr	%l0			! clean the rest
	clr	%l1
	clr	%l2
	clr	%l3
	clr	%l4
	clr	%l5
	clr	%l6
	clr	%l7
	clr	%o0
	clr	%o1
	clr	%o2
	clr	%o3
	clr	%o4
	clr	%o5
	jmp	%o6			! reexecute save
	rett	%o7
1:
	nop			! psr delay
	jmp	%l1			! reexecute save
	rett	%l2
