	.seg	"data"
	.asciz  "@(#) low.s 1.1 92/07/30"
	.align	4
	.seg	"text"

! Copyright (C) 1987 by Sun Microsystems, Inc.
!
! This source code is a product of Sun Microsystems, Inc. and is provided
! for unrestricted use provided that this legend is included on all tape
! media and as a part of the software program in whole or part.  Users
! may copy or modify this source code without charge, but are not authorized
! to license or distribute it to anyone else except as part of a product or
! program developed by the user.
!
! THIS PROGRAM CONTAINS SOURCE CODE COPYRIGHTED BY SUN MICROSYSTEMS, INC.
! AND IS LICENSED TO SUNSOFT, INC., A SUBSIDIARY OF SUN MICROSYSTEMS, INC.
! SUN MICROSYSTEMS, INC., MAKES NO REPRESENTATIONS ABOUT THE SUITABLITY
! OF SUCH SOURCE CODE FOR ANY PURPOSE.  IT IS PROVIDED "AS IS" WITHOUT
! EXPRESS OR IMPLIED WARRANTY OF ANY KIND.  SUN MICROSYSTEMS, INC. DISCLAIMS
! ALL WARRANTIES WITH REGARD TO SUCH SOURCE CODE, INCLUDING ALL IMPLIED
! WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  IN
! NO EVENT SHALL SUN MICROSYSTEMS, INC. BE LIABLE FOR ANY SPECIAL, INDIRECT,
! INCIDENTAL, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING
! FROM USE OF SUCH SOURCE CODE, REGARDLESS OF THE THEORY OF LIABILITY.
!
! This source code is provided with no support and without any obligation on
! the part of Sun Microsystems, Inc. to assist in its use, correction, 
! modification or enhancement.
!
! SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
! INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY THIS
! SOURCE CODE OR ANY PART THEREOF.
!
! Sun Microsystems, Inc.
! 2550 Garcia Avenue
! Mountain View, California 94043


#include <sun4/trap.h>
#include <sun4/asm_linkage.h>
#include <sun4/psl.h>
#include "low.h"

	.seg	"text"
	.global  ___schedule
	.global  ___Curproc
	.global  ___EventsPending
	.global  ___AsynchLock
	.global  ___fake_sig
	.global  ___exchelpclean
	.global  ___NuggetSP
	.global	___lwpkill
	.global	___AllFlushed

!
! assembly language support routines
!

! %sp (%o6) is stack pointer and points to register save area for current win.
! %i7 is return address (ret will add 8 to be correct) to caller. 
! %o7 is return address from call of leaf routine (use retl).
! %fp (%i6) points to previous frame.
! %g's are treated as temps by the compiler -- if they become truely global,
! it may not be nec. to save/restore all of them since they behave like
! shared memory.
! %sp must point to a valid save area (16 words) when save is called.
! %fp must point to a valid save area (16 words) when restore is called.
! save/restore use %src in old window and %dst in new window
! %sp MUST ALWAYS be valid. If you try to to sigcleanup() or restore
! with a funky %sp, the kernel will do strange things. Note that
! C assumes a valid %fp and can generate access to addresses above it.

! __checkpoint()
! Save context and reschedule.
! The return address to the caller of checkpoint is in %o7.
! Eventually, schedule calls swtch which
! returns to the caller of checkpoint.
! locals and ins are saved in register save area.
! Since we call checkpoint voluntarily, %g's, %o's and %y need not be saved.
! We are on a fast path after flushing the windows: if schedule() calls
! swtch() directly, there is no need to flushh again.
! checkpoint should be called while LOCKED.  However, when
! checkpoint returns via swtch, the state is UNLOCKED.
LENTRY(checkpoint)
	ta	ST_FLUSH_WINDOWS	! save all registers
	set	1, %g1
	sethi	%hi(___AllFlushed), %o2
	st	%g1, [%lo(___AllFlushed)+%o2]	! ___AllFlushed = TRUE
! Now, we will reuse this window mercilesslly until there is a context switch.
! We must be very careful to not flush or restore from this point
! until we are ready, or we will store garbage in the current frame.
! We are LOCKED, but this does
! not prevent signals from arriving. We make sure %sp and %fp are valid
! since we will call schedule() in C. Switching stacks also
! ensures that if an interrupt happens,
! the window flush will store the garbage on the nugget stack, not the client's.
	sethi	%hi(___Curproc), %o2
	ld	[%o2 + %lo(___Curproc)], %o1	! %o1 = __Curproc
	st	%o7, [%o1+PC_OFFSET]	! pc when this thread resumes
	st	%sp, [%o1+SP_OFFSET]	! save sp
	sethi	%hi(___NuggetSP), %o1	! get nugget stack
	ld	[%o1 + %lo(___NuggetSP)], %sp	! switch stacks
	mov	%sp, %fp			! valid %fp
	ba	___schedule + 4		! add 4 to skip save at the beginning
	add	%sp, -SA(MINFRAME), %sp	! simulate <%sp, %fp> paradigm wo save
	! NOTREACHED
	
! __swtch()
! switch context when not at interrupt level and the new thread
! was not switched out asynchronously.
! Since this was a voluntary scheduling (via a procedure call of the
! thread), we don't need to worry about restoring psl and %o's which
! were not saved in the save area, and %g's and %y which are not
! preserved across procedure call.
! Must be called while LOCKED.
!
! Context is restored BEFORE UNLOCK (i.e., clearing AsynchLock)
! so that any interrupt will capture the correct context
! (i.e., save the "backing store" version of the registers,
! not the values in the registers at the time ___swtch is called).
! We LOCK before calling fake_sig to prevent intrerrupts from
! changing anything before fake_sig causes a scheduling decision.
! We UNLOCK before testing EventsPending so a interrupt will
! cause scheduling (and not just save info and set EventsPending),
! even if we decided that EventsPending was false
! (and then immediately got an interrupt).
! Once we decided that EventsPending is true, it is safe to LOCK
! again because setting EventsPending is idempotent.
! If there is an interrupt just before we lock, it may cause the event
! list to be cleaned out and the thread will be rescheduled here.
! In that case, fake_sig may be called redundantly, but it won't hurt.
! Note that lwp_full state is FALSE for fake_sig, so the pc will
! be retrieved from the context, not the interrupt pc, and we will
! add 8 to that to simulate the retl here.
! fake_sig always returns via full_swtch.
LENTRY(swtch)
	ta	ST_FLUSH_WINDOWS	! so restore causes an underflow
LENTRY(fastswtch)			! skip flush, already done
	sethi	%hi(___Curproc), %o2
	ld	[%o2 + %lo(___Curproc)], %o1	! %o1 = __Curproc
	ld	[%o1+PC_OFFSET], %i7	! restore pc into HIS %o7
	ld	[%o1+SP_OFFSET], %fp	! restore sp into HIS %o6
	restore				! get back to the window where
					! checkpoint was called, with underflow
					! restoring %i's and %l's
					! ASSERT(%sp is valid) before restore
	sethi	%hi(___AsynchLock), %o1
	clr	[%o1 + %lo(___AsynchLock)]	! Allow interrupts
	sethi	%hi(___EventsPending), %o2
	ld	[%o2 + %lo(___EventsPending)], %o1
	cmp	%g0, %o1		! check for pending events
	be	1f			! none
	or	%g0,1, %o1		! lock before calling fake_sig
	sethi	%hi(___AsynchLock), %o2
	st	%o1, [%lo(___AsynchLock) + %o2]
! His state is still valid in the context block and his windows
! were flushed when he was scheduled out, so no need to flush here
	sethi	%hi(___NuggetSP), %o1	! swtch to __NuggetSP
	call	___fake_sig		! process events -- fake an interrupt
	ld	[%lo(___NuggetSP) + %o1], %sp	! don't use client sp for intr.
					! ___fake_sig will save to provide %fp
	unimp	0			! just in case it returns
	/*NOTREACHED*/
1:	retl				! jump to lowinit or back to caller of
	nop				! checkpoint

! __lowinit()
! Control gets here from returning from swtch() the first time a thread runs.
! We copy the %i's (parameters to thread initial routine) to the %o's
! to make it look like we are calling the thread with these args.
! The frame is already set up to have any additional args on it, etc.
!
LENTRY(lowinit)
	mov	%i0, %o0		! put parameters to right place
	mov	%i1, %o1
	mov	%i2, %o2
	mov	%i3, %o3
	mov	%i4, %o4
	mov	%i5, %o5
	set	___lwpkill - 8, %o7	! When dying, go here: %i7 of thread
	jmp	%i7			! load pc
	mov	%i6, %o6		! 	and sp

! __unlock()
! Unlock a critical section.
! If events have occurred while LOCKED, call fake_sig (which calls
! do_agent) to simulate a signal and cause all pending evnets to
! be processed.  Context must be saved before calling fake_sig
! because fake_sig will cause a scheduling decision.
! Context is saved while LOCKED to prevent signals from preserving
! an incorrect state before the correct state is established.
! EventsPending is tested while UNLOCKED for the same reason as
! in swtch. Note that we use the nugget stack to process interrupts on:
! some clients may use the area below their stack for a quick and dirty
! malloc and we don't want to trash it.
LENTRY(unlock)
	sethi	%hi(___AsynchLock), %o1		! ___AsynchLock = 0;
	clr	[%lo(___AsynchLock) + %o1]
	sethi	%hi(___EventsPending), %o3	! if (___EventsPending == 0)
	ld	[%lo(___EventsPending) + %o3], %o2
	tst	%o2
	be	1f				! nothing to do
	or	%g0,1, %o2
	st	%o2, [%lo(___AsynchLock) + %o1] 	! ___AsynchLock = 1;
	sethi	%hi(___Curproc), %o2
	ld	[%lo(___Curproc) + %o2], %o1	! %o1 = __Curproc
	st	%sp, [%o1+SP_OFFSET]	! store sp
	st	%o7, [%o1+PC_OFFSET]	! pc when this thread resumes
	sethi	%hi(___NuggetSP), %o1	! switch to NuggetSP (AFTER saving state)
	ta	ST_FLUSH_WINDOWS	! save his window state -- it's
					! not kept anywhere else now
	call	___fake_sig
	ld	[%lo(___NuggetSP) + %o1], %sp	! switch stacks
	unimp	0			! just in case it returns
	! NOTREACHED
1:	retl
	nop

! __interrupt()
! our own version of sigtramp with no floating point stuff saved.
! called when an interrupt or trap occurs.
!
! On entry sp points to:
! 	0 - 63: window save area
!	64: signal number
!	68: signal code
!	72: pointer to sigcontext
!	76: addr parameter
!
! A sigcontext looks like:
!	00: on signal stack flag
!	04: old signal mask
!	08: old sp
!	12: old pc
!	16: old npc
!	20: old psr
!	24: old g1
!	28: old o0
!
! We are in the old window with windows flushed. We first do a save in which we
! bump the sp down by enough to have an area to save the old globals (%g2-%g7)
! and the normal frame. We don't save fp state here (32 fp regs, %fsr).
! We aren't allowed to clobber anything (like globals) in Curproc if LOCKED,
! so we have to (potentially) copy globals twice: once on the stack, another
! time in the lwp context if not in a critical section.
! E.g., suppose we are LOCKED in lwp_setregs. An interrupt would then change
! the values we wanted to set if we stored state directly into the context.
! We could optimise this further by testing for LOCKED here; if not
! locked, then we could store the registers directly into the context (XXX).
! The %i's and %l's of the interrupted process were flushed by UNIX:
! the interrupted guy's windows were flushed.
! We need to save globals and interrupted guy's %o's (my %i's).
! We pass real_sig the address of the saved registers so it can copy them
! into the context if not LOCKED.
! This would all be better if UNIX passed us all of the registers in the
! sigcontext.
! Note that we don't need to restore the %o's if we return immediately
! since we don't touch %i's (after save, his %o's) here and
! if we return there is no need to save state. If we don't return,
! because the interrupt stack is not part of context, we must save the %o's.
!
LENTRY(interrupt)
	save	%sp, -(SA(MINFRAME)+SA(7*8)), %sp ! make frame on (new) stack
						! and avoid touching his %o's
	std	%g2, [%sp + SA(MINFRAME)+(0*8)] ! save globals and o's
	std	%g4, [%sp + SA(MINFRAME)+(1*8)]
	std	%g6, [%sp + SA(MINFRAME)+(2*8)]
	std	%i0, [%sp + SA(MINFRAME)+(3*8)]
	std	%i2, [%sp + SA(MINFRAME)+(4*8)]
	std	%i4, [%sp + SA(MINFRAME)+(5*8)]
	std	%i6, [%sp + SA(MINFRAME)+(6*8)]
	ld	[%fp + 64], %o0		! get signal number
	ld	[%fp + 68], %o1		! get code
	ld	[%fp + 72], %o2		! get ptr to sigcontext
	ld	[%fp + 76], %o3		! get addr
	clr	%o4
	mov	%y, %l1
	mov	%l1, %o5
	call	___real_sig	! __real_sig(sig,code,&sc,addr,regaddr,%y)
	add	%sp, (SA(MINFRAME) + (0*8)), %o4	! address of saved %'s
	! MAYRETURN
	! Restore state if needed -- e.g., if LOCKED.
	mov	%l1, %y
	ld	[%fp + 72], %i0		! set sigcontext-> in %o0 of old window
	ldd	[%sp + SA(MINFRAME)+(0*8)], %g2 ! restore globals
	ldd	[%sp + SA(MINFRAME)+(1*8)], %g4
	ldd	[%sp + SA(MINFRAME)+(2*8)], %g6
! note that I'm in his (interrupted guy's) window. Thus, sigcleanup
! needs to restore %o0 used for sigcontext arg. %g1 is used for syscall #'s
! so it is also part of the sigcontext.
	restore	%g0, 139, %g1		! sigcleanup system call
	ta	ST_SYSCALL		! in old window now
	unimp	0			! just in case it returns
	/*NOTREACHED*/

! __rti()
! rti() is called from full_swtch().
! Don't touch %o6 -- it will be reset by sigcleanup and we need
! a valid stack to run on now.
! Restore %g's and %o's and let sigcleanup restore everything else.
! We're in the same window before and after calling sigcleanup.
!
LENTRY(rti)
	sethi	%hi(___Curproc), %l5
	ld	[%lo(___Curproc) + %l5], %l4	! %l4 = __Curproc
	ldd	[%l4+G2_OFFSET+(0*8)], %g2	! restore %g;s and %o's
	ldd	[%l4+G2_OFFSET+(1*8)], %g4
	ldd	[%l4+G2_OFFSET+(2*8)], %g6
	ldd	[%l4+O0_OFFSET+(0*8)], %o0;
	ldd	[%l4+O0_OFFSET+(1*8)], %o2;
	ldd	[%l4+O0_OFFSET+(2*8)], %o4;
	ld	[%l4+O0_OFFSET+(3*8)+4], %o7;
	ld	[%l4+Y_OFFSET], %g1;
	mov	%g1, %y
	set	___Context, %o0		! argument to sigccleanup
	mov	139, %g1		! sigcleanup call
	ta	ST_SYSCALL
	unimp	0
	! NOTREACHED

! __exc_longjmp(pattern, sp, pc)
! flush windows so we can fault in his window set.
! restore his sp and pc and restore to underflow into the window
! of the corresponding exc_handle.
LENTRY(exc_longjmp)
	ta	ST_FLUSH_WINDOWS
	mov	%o1, %fp		! restore sp in previous window (%o6)
	sub	%fp, WINDOWSIZE, %sp	! paranoid
	mov	%o2, %o7
	retl
	restore				! o0 is return value

! exc_jmpandclean(pattern, sp, pc, prevretadr)
! same as exc_longjmp, but restore the saved return address
! (which was replaced by exc_cleanup). 
LENTRY(exc_jmpandclean)
	ta	ST_FLUSH_WINDOWS
	mov	%o0, %i0		! return value
	mov	%o1, %i6		! restore sp in previous window (%o6)
	mov	%o2, %i7		! set up his retl
	mov	%o3, %i5		! his saved return pc
	restore				! underflow into his window
	retl				! reset his PC
	mov	%o5, %i7		! reset his real return address

! __exccleanup()
! When an exit or exception handler is installed, this routine 
! replaces the normal return address. It will call exchelpclean()
! to flush exception handlers at or below our level, reinstall
! the correct return pc (supplied by exchelpclean()), and return
! normally.
LENTRY(exccleanup)
	save	%sp, -SA(MINFRAME), %sp	! let's be a real procedure (not a leaf)
					! to preserve real return value too
	call	___exchelpclean		! this guy restores the return addr
	nop
	mov	%o0, %i7		! put return addr to where it should be
	ret				! return normally
	restore

! __exc_getclient()
! A client routine C called an exception handling routine E.
! E can get C's return address (pointing at the call instruction -- need
! to add 8 to it) and C's sp into the globals __ClientSp and __ClientRetAdr
! respectively.
! We flush the windows so the exception code can grovel around in
! the saved register areas of old client frames.
!
LENTRY(exc_getclient)
	ta	ST_FLUSH_WINDOWS
	set	___ClientSp, %o0	! %fp(%i6) == caller's sp
	st	%fp, [%o0]
	set	___ClientRetAdr, %o0	! %i7 == caller's ret. addr
	retl
	st	%i7, [%o0]

! __sparcfpp_save(vec)
! routines to save the floating point state: %fsr, 32 fp regs.
! The vec arg points to a struct containing 16 doubles for the %f's,
! and a long for the %fsr.
! There is a problem here in that saving the fsr could expose a
! fp exception in the fpa queue. As a result, we could generate
! the exception for the wrong thread if special contexts are optimised
! for the fpa. We take the easy way out and don't optimise.
! We should be fancy and see if __Curproc == self. If not,
! pend the trap for self. To pend the trap, use save_event.
! Save fsr first to cause fpu to reach equiblibrium.
! XXX May need to make save_event part of the interface.
LENTRY(sparcfpp_save)
	st	%fsr, [%o0 + (16*8)]	! save %fsr FIRST
	std	%f0, [%o0 + (0*8)]	! save all fp registers
	std	%f2, [%o0 + (1*8)]
	std	%f4, [%o0 + (2*8)]
	std	%f6, [%o0 + (3*8)]
	std	%f8, [%o0 + (4*8)]
	std	%f10, [%o0 + (5*8)]
	std	%f12, [%o0 + (6*8)]
	std	%f14, [%o0 + (7*8)]
	std	%f16, [%o0 + (8*8)]
	std	%f18, [%o0 + (9*8)]
	std	%f20, [%o0 + (10*8)]
	std	%f22, [%o0 + (11*8)]
	std	%f24, [%o0 + (12*8)]
	std	%f26, [%o0 + (13*8)]
	std	%f28, [%o0 + (14*8)]
	retl
	std	%f30, [%o0 + (15*8)]

! __sparcfpp_restore(vec)
! restore floating point state
LENTRY(sparcfpp_restore)
	ldd	[%o0 + (0*8)], %f0	! restore all fp registers
	ldd	[%o0 + (1*8)], %f2
	ldd	[%o0 + (2*8)], %f4
	ldd	[%o0 + (3*8)], %f6
	ldd	[%o0 + (4*8)], %f8
	ldd	[%o0 + (5*8)], %f10
	ldd	[%o0 + (6*8)], %f12
	ldd	[%o0 + (7*8)], %f14
	ldd	[%o0 + (8*8)], %f16
	ldd	[%o0 + (9*8)], %f18
	ldd	[%o0 + (10*8)], %f20
	ldd	[%o0 + (11*8)], %f22
	ldd	[%o0 + (12*8)], %f24
	ldd	[%o0 + (13*8)], %f26
	ldd	[%o0 + (14*8)], %f28
	ldd	[%o0 + (15*8)], %f30
	retl
	ld	[%o0 + (16*8)], %fsr

! __sigtrap()
! If traps are handled, return here instead of real client return address
! from the UNIX signal.
! On the client stack we pushed the real return pc and the signal number. 
! Call exc_trap(signum) to raise the exception.
! Then, pop the args real_sig() pushed on the stack and restore the
! real return address.
LENTRY(sigtrap)
	save	%sp, -SA(MINFRAME), %sp	! save to avoid messing up client's %o's
	ld	[%fp], %o0		! signum is at %fp->
	call	___exc_trap		! process the trap
	ld	[%fp + 4], %l1		! interrupted user pc
	add	%fp, 8, %fp		! pop signum and pc from user stack
	mov	%o0, %i0		! return value from sigtrap
	jmp	%l1			! return normally
	restore				! and return to client's window

! __stacksafe()
! __stacksafe returns 1 if the stack is safe, else 0.
! "safe" means that the sp is not below the
! base of the stack (___CurStack + CKSTKOVERHEAD). 
! It is used by CHECK macro.
! 
LENTRY(stacksafe)
	sethi	%hi(___CurStack), %o1	! CurStack is current thread stack bottom
	ld	[%lo(___CurStack) + %o1], %o2		! %o2 = __CurStack
	sethi	%hi(CKSTKOVERHEAD), %o3
	or	%o3, %lo(CKSTKOVERHEAD), %o3
	add	%o2, %o3, %o2		! if stack is shrunk by CKSTKOVERHEAD
	cmp	%sp, %o2		! %sp - %o2 > 0 means safe
	bg	1f
	retl
	mov	%g0, %o0		! not safe, return 0
1:	retl
	mov	1, %o0			! safe, return 1
