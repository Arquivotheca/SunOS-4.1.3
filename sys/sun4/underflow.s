/*	@(#)underflow.s 1.1 92/07/30 SMI	*/

/*
 *	Copyright (c) 1989 by Sun Microsystems, Inc.
 */

#include <machine/asm_linkage.h>
#include <machine/param.h>
#include <machine/psl.h>
#include <machine/trap.h>
#include <machine/mmu.h>
#include <machine/pte.h>
#include <machine/reg.h>
#include <machine/buserr.h>
#include <machine/pcb.h>

#include "assym.s"

	.seg	"text"
	.align	4

PROT = (PG_V | PG_W) >> PG_S_BIT
PROT_INVAL = (PG_V) >> PG_S_BIT

/*
 * Window underflow trap handler.
 */
	.global	window_underflow
window_underflow:

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
	!
	! User underflow. Window to be restored is a user window.
	! We must check whether the user stack is resident where the window
	! will be restored from, which is pointed to by the windows sp.
	! The sp is the fp of the window which tried to do the restore,
	! so that it is still valid.
	!
	restore				! get into window to be restored
	!
	! Normally, we would check the alignment, and then probe the top
	! and bottom of the save area on the stack. However we optimize
	! this by checking that both ends of the save area are within a
	! 4k unit (the biggest mask we can generate in one cycle), and
	! the alignment in one shot. This allows us to do one probe to
	! the page map. NOTE: this assumes a page size of at least 4k.
	!
	and	%sp, 0xfff, %l0
#ifdef VA_HOLE
	! check if the sp points into the hole in the address space
	sethi	%hi(_hole_shift), %l5	! hole shift address
	ld	[%l5 + %lo(_hole_shift)], %l7
	add	%l0, (14*4), %l0	! interlock, bottom of save area
	sra	%sp, %l7, %l5
	inc	%l5
	andncc	%l5, 1, %l5
	bz	1f
	andncc	%l0, 0xff8, %g0
	b,a	wu_stack_not_res	! sp is in the hole
1:
#else
	add	%l0, (14*4), %l0
	andncc	%l0, 0xff8, %g0
#endif VA_HOLE
	bz,a	wu_sp_bot
	lda	[%sp]ASI_PM, %l1	! check for stack page resident
	!
	! Stack is either misaligned or crosses a 4k boundary.
	!
	btst	0x7, %sp		! test sp alignment
	bz	wu_sp_top
	add	%sp, (14*4), %l0	! delay slot, check top of save area

	!
	! A user underflow trap has happened with a misaligned sp.
	! Fake a memory alignment trap.
	!
	save				! get back to orig window
	save
	mov	%l3, %wim		! restore old wim, so regs are dumped
	b	sys_trap
	mov	T_ALIGNMENT, %l4	! delay slot, fake alignment trap

wu_sp_top:
#ifdef VA_HOLE
	sra	%l0, %l7, %l5
	inc	%l5
	andncc	%l5, 1, %l5
	bz,a	1f
	lda	[%l0]ASI_PM, %l1	! get pme for this address
	b,a	wu_stack_not_res	! address is in the hole
1:	srl	%l1, PG_S_BIT, %l1	! get vws bits
	sra	%sp, %l7, %l5
	inc	%l5
	andncc	%l5, 1, %l5
	bz	1f
	cmp	%l1, PROT		! look for valid, writeable, user
	b,a	wu_stack_not_res	! stack page can never be resident
1:	be,a	wu_sp_bot
	lda	[%sp]ASI_PM, %l1	! delay slot, check bottom of save area
	b,a	wu_stack_not_res	! stack page not resident
#else
	lda	[%l0]ASI_PM, %l1	! get pme for this address
	srl	%l1, PG_S_BIT, %l1	! get vws bits
	cmp	%l1, PROT		! look for valid, writeable, user
	be,a	wu_sp_bot
	lda	[%sp]ASI_PM, %l1	! delay slot, check bottom of save area
	b,a	wu_stack_not_res	! stack page not resident
#endif VA_HOLE


wu_sp_bot:
	srl	%l1, PG_S_BIT, %l1	! get vws bits
	cmp	%l1, PROT		! look for valid, writeable, user
	be	wu_ustack_res
	nop				! extra nop in rare case

	mov	%sp, %l0		! save fault address

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
	mov	%l7, %g1		! save computed sp
	mov	%l0, %g2		! save fault address
	mov	BE_INVALID, %g3		! compute bus error reg code
	btst	PROT_INVAL, %l1
	bnz,a	1f
	mov	BE_PROTERR, %g3
1:
	save				! back to last user window
	mov	%psr, %g4		! get CWP
	save				! back to trap window

	!
	! save remaining user state
	!
	mov	%g1, %sp		! setup kernel stack
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

	wr	%l0, PSR_ET, %psr	! enable traps
	mov	T_DATA_FAULT, %o0
	add	%sp, MINFRAME, %o1
	mov	%g2, %o2
	mov	%g3, %o3
	call	_trap			! trap(T_DATA_FAULT,
	mov	S_READ, %o4		!	rp, addr, be, S_READ)

	b,a	sys_rtt

	!
	! The user's save area is resident. Restore the window.
	!
wu_ustack_res:
	RESTORE_WINDOW(%sp)
	save				! get back to original window
	save
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
