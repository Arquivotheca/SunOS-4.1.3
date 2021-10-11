	.seg	"data"
	.asciz  "@(#) low.s 1.1 92/07/30"
	.align	4
	.seg	"text"

! Copyright (C) 1987 by Sun Microsystems, Inc.

#include <sun4/trap.h>
#include <sun4/asm_linkage.h>
#include <sun4/psl.h>
#include <machlwp/low.h>
#include "assym.s"

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
	.global _flush_windows
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
! swtch() directly, there is no need to flush again.
! checkpoint should be called while LOCKED.  However, when
! checkpoint returns via swtch, the state is UNLOCKED.
!
! Now, we will reuse this window mercilesslly until there is a context switch.
! We must be very careful to not flush or restore from this point
! until we are ready, or we will store garbage in the current frame.
! We are LOCKED, but this does
! not prevent signals from arriving. We make sure %sp and %fp are valid
! since we will call schedule() in C. Switching stacks also
! ensures that if an interrupt happens,
! the window flush will store the garbage on the nugget stack, not the client's.
LENTRY(checkpoint)
	sethi	%hi(___Curproc), %o2
	ld	[%o2 + %lo(___Curproc)], %o1	! %o1 = __Curproc
	st	%o7, [%o1+PC_OFFSET]	! pc when this thread resumes
	st	%sp, [%o1+SP_OFFSET]	! save sp

	save
	call	_flush_windows	! save all registers
	nop
	restore
	set	1, %g1
	sethi	%hi(___AllFlushed), %o2
	st	%g1, [%lo(___AllFlushed)+%o2]	! ___AllFlushed = TRUE

	sethi	%hi(___NuggetSP), %o1		! get nugget stack
	ld	[%o1 + %lo(___NuggetSP)], %fp	! switch stacks
	ba	___schedule + 4		! add 4 to skip save at the beginning
	sub	%fp, SA(MINFRAME), %sp		! safely switch
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
	call	_flush_windows
	nop
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
	call	_flush_windows
	nop
	sethi	%hi(___NuggetSP), %o1	! switch to NuggetSP (AFTER saving state)
	call	___fake_sig
	ld	[%lo(___NuggetSP) + %o1], %sp	! switch stacks
	unimp	0			! just in case it returns
	! NOTREACHED
1:	retl
	nop

! __exc_longjmp(pattern, sp, pc)
! flush windows so we can fault in his window set.
! restore his sp and pc and restore to underflow into the window
! of the corresponding exc_handle.
LENTRY(exc_longjmp)
	call	_flush_windows
	nop
	mov	%o1, %fp		! restore sp in previous window (%o6)
	sub	%fp, WINDOWSIZE, %sp	! paranoid
	mov	%o2, %o7
	retl
	restore				! o0 is return value

! exc_jmpandclean(pattern, sp, pc, prevretadr)
! same as exc_longjmp, but restore the saved return address
! (which was replaced by exc_cleanup). 
LENTRY(exc_jmpandclean)
	call	_flush_windows
	nop
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
	call	_flush_windows
	nop
	set	___ClientSp, %o0	! %fp(%i6) == caller's sp
	st	%fp, [%o0]
	set	___ClientRetAdr, %o0	! %i7 == caller's ret. addr
	retl
	st	%i7, [%o0]

! __stacksafe()
! __stacksafe returns 1 if the stack is safe, else 0.
! "safe" means that the sp is not below the
! base of the stack (___CurStack + CKSTKOVERHEAD). 
! It is used by check macro.
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
