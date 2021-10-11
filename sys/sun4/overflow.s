/*	@(#)overflow.s 1.1 92/07/30 SMI	*/

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

PROT	= (PG_V | PG_W) >> PG_S_BIT
P_INVAL	= (PG_V) >> PG_S_BIT

/*
 * window overflow trap handler
 */
	.global	window_overflow
window_overflow:

#ifdef PERFMETER
	sethi	%hi(_overflowcnt), %l5
	ld	[%l5 + %lo(_overflowcnt)], %l7
	inc	%l7
	st	%l7, [%l5 + %lo(_overflowcnt)]
#endif PERFMETER

	! wim stored into %l3 by trap vector
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
	! We must check whether the user stack is resident where the window
	! will be saved, which is pointed to by the window's sp.
	! We must also check that the sp is aligned to a word boundary.
	!
	save				! get into window to be saved
	mov	%g1, %wim		! install new wim
	!
	! Normally, we would check the alignment, and then probe the top
	! and bottom of the save area on the stack. However we optimize
	! this by checking that both ends of the save area are within a
	! 4k unit (the biggest mask we can generate in one cycle), and
	! the alignment in one shot. This allows us to do one probe to
	! the page map. NOTE: this assumes a page size of at least 4k.
	!
	and	%sp, 0xfff, %g1
#ifdef VA_HOLE
	! check if the sp points into the hole in the address space
	sethi	%hi(_hole_shift), %g2	! hole shift address
	ld	[%g2 + %lo(_hole_shift)], %g2		
	add	%g1, (14*4), %g1	! interlock, bottom of save area 
	sra	%sp, %g2, %g2
	inc	%g2
	andncc	%g2, 1, %g2
	bz	1f
	andncc	%g1, 0xff8, %g0
	b,a	wo_stack_not_res	! stack page is in the hole
1:
#else
	add	%g1, (14*4), %g1
	andncc	%g1, 0xff8, %g0
#endif VA_HOLE
	bz,a	wo_sp_bot
	lda	[%sp]ASI_PM, %g1	! check for stack page resident
	!
	! Stack is either misaligned or crosses a 4k boundary.
	!
	btst	0x7, %sp		! test sp alignment
	bz	wo_sp_top
	add	%sp, (14*4), %g1	! delay slot, check top of save area

	!
	! Misaligned sp. If this is a userland trap fake a memory alignment
	! trap. Otherwise, put the window in the window save buffer so that
	! we can catch it again later.
	!
	mov	%psr, %g1		! get psr (we are not in trap window)
	btst	PSR_PS, %g1		! test for user or sup trap
	bnz	wo_save_to_buf		! sup trap, save window in uarea buf
	nop

	restore				! get back to orig window
	mov	%l5, %g2		! restore g2
	mov	%l7, %g1		! restore g1
	mov	%l3, %wim		! restore old wim, so regs are dumped
	b	sys_trap
	mov	T_ALIGNMENT, %l4	! delay slot, fake alignment trap

wo_sp_top:
#ifdef VA_HOLE
	sethi	%hi(_hole_shift), %g2	! hole shift address
	ld	[%g2 + %lo(_hole_shift)], %g2		
	sra	%g1, %g2, %g2
	inc	%g2
	andncc	%g2, 1, %g2
	bz,a	1f
	lda	[%g1]ASI_PM, %g1	! get pme for this address
	b,a	wo_stack_not_res	! stack page can never be resident
1:
	sethi	%hi(_hole_shift), %g2	! hole shift address
	ld	[%g2 + %lo(_hole_shift)], %g2		
	srl	%g1, PG_S_BIT, %g1	! get vws bits
	sra	%sp, %g2, %g2
	inc	%g2
	andncc	%g2, 1, %g2
	bz,a	1f
	cmp	%g1, PROT		! look for valid, writeable, user
	b,a	wo_stack_not_res	! stack page can never be resident
1:
#else
	lda	[%g1]ASI_PM, %g1	! get pme for this address
	srl	%g1, PG_S_BIT, %g1	! get vws bits
	cmp	%g1, PROT		! look for valid, writeable, user
#endif VA_HOLE
	be,a	wo_sp_bot
	lda	[%sp]ASI_PM, %g1	! delay slot, check bottom of save area

	b	wo_stack_not_res	! stack page not resident
	bset	1, %sp			! note that this is top of save area

wo_sp_bot:
	srl	%g1, PG_S_BIT, %g1	! get vws bits
	cmp	%g1, PROT		! look for valid, writeable, user
	be	wo_ustack_res
	nop				! extra nop

wo_stack_not_res:
	!
	! The stack save area for user window is not resident.
	!
	mov	%psr, %g1		! get psr (we are not in trap window)
	btst	PSR_PS, %g1		! test for user or sup trap
	bnz,a	wo_save_to_buf		! sup trap, save window in uarea buf
	bclr	1, %sp			! no need to know which end failed

	btst	1, %sp			! reconstruct fault address
	bz,a	1f			! top of save area?
	mov	%sp, %g1		! no, use sp
	bclr	1, %sp			! yes, clear "which end" flag
	add	%sp, (14*4), %g1	! add appropriate amount
1:
	!
	! We first save the window in the first window buffer in the u area.
	! Then we fake a user data fault. If the fault succeeds, we will
	! reexecute the save and overflow again, but this time the page
	! will be resident
	!
	sethi	%hi(_uunix), %g2
	ld	[%g2 + %lo(_uunix)], %g2
	st	%sp, [%g2+PCB_SPBUF]	! save sp
	SAVE_WINDOW(%g2+PCB_WBUF)
	restore				! get back into original window
	!
	! Set the save buffer ptr to next buffer
	!
	mov	1, %l4
	st	%l4, [%g2+PCB_WBCNT]	! u->u_pcb.pcb_wbcnt = 1
	!
	! Compute the user window mask (u.u_pcb.pcb_uwm), which is a mask of
	! which windows contain user data. In this case it is all the register
	! except the one at the old WIM and the one we just saved.
	!
	mov	%wim, %l4		! get new WIM
	or	%l4, %l3, %l4		! or in old WIM
	not	%l4
	mov	-2, %g2
	sll	%g2, %l6, %g2
	andn	%l4, %g2, %l4
	sethi	%hi(_uunix), %g2
	ld	[%g2 + %lo(_uunix)], %g2
	st	%l4, [%g2+PCB_UWM]	! u->u_pcb.pcb_uwm = ~(OWIM|NWIM)
	set	_masterprocp, %g2
	ld	[%g2], %g2
	ld	[%g2 + P_STACK], %sp
	mov	%g1, %l6		! save fault address, arg to _trap
	mov	%l7, %g1		! restore g1, so we can save it
	mov	%l5, %g2
	SAVE_GLOBALS(%sp + MINFRAME)
	SAVE_OUTS(%sp + MINFRAME)
	st	%l0, [%sp + MINFRAME + PSR*4] ! psr
	st	%l1, [%sp + MINFRAME + PC*4] ! pc
	st	%l2, [%sp + MINFRAME + nPC*4] ! npc
#ifdef VA_HOLE
	sethi	%hi(_hole_shift), %g1	! hole shift address
	ld	[%g1 + %lo(_hole_shift)], %g1		
	sra	%l6, %g1, %g1
	inc	%g1
	andncc	%g1, 1, %g1
	bz,a	2f
	lda	[%l6]ASI_PM, %g1	! get pme for this address
	b	1f			! stack page in the hole
	mov	BE_INVALID, %o3
2:
#else
	lda	[%l6]ASI_PM, %g1	! compute proper bus error reg
#endif VA_HOLE
	mov	BE_INVALID, %o3
	srl	%g1, PG_S_BIT, %g1
	btst	P_INVAL, %g1
	bnz,a	1f
	mov	BE_PROTERR, %o3
1:
	wr	%l0, PSR_ET, %psr	! enable traps
	mov	T_DATA_FAULT, %o0
	add	%sp, MINFRAME, %o1
	mov	%l6, %o2
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
	jmp	%l1			! reexecute save
	rett	%l2

	!
	! The user's save area is resident. Save the window.
	!
wo_ustack_res:
	SAVE_WINDOW(%sp)
	restore				! go back to trap window
wo_out:
	sethi	%hi(_uunix), %l3
	ld	[%l3 + %lo(_uunix)], %l3
	mov	%l5, %g2		! restore g2
	ld	[%l3+PCB_FLAGS], %l3	! check for clean window maintenance
	mov	%l7, %g1		! restore g1
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
	jmp	%l1			! reexecute save
	rett	%l2
