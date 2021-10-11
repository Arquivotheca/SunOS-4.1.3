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
	.global  ___NuggetSP
	.global	___lwpkill
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
!
LENTRY(checkpoint)
	mov	%psr, %g2		! save the psr
	or	%g2, PSR_PIL, %g1	! splhigh
	mov	%g1, %psr
	nop; nop
	sethi	%hi(___Curproc), %o2
	ld	[%o2 + %lo(___Curproc)], %o1	! %o1 = __Curproc
	st	%o7, [%o1+PC_OFFSET]	! pc when this thread resumes
	st	%sp, [%o1+SP_OFFSET]	! save sp
	save	%sp, -SA(MINFRAME), %sp
	call	_flush_windows	! save all registers
	nop

	sethi	%hi(___NuggetSP), %o1		! get nugget stack
	ld	[%o1 + %lo(___NuggetSP)], %fp	! switch stacks
	mov	%psr, %g1		! restore processor priority
	andn	%g1, PSR_PIL, %g1
	and	%g2, PSR_PIL, %g2
	or	%g1, %g2, %g2
	mov	%g2, %psr
	ba	___schedule + 4		! add 4 to skip save at the beginning
	sub	%fp, SA(MINFRAME), %sp		! safely switch
	nop
	! NOTREACHED

! __swtch()
! Restore to pc and sp. Switch to new thread. Although this code is called 
! in __schedule, there are 2 circumstances. First, when the thread runs the 
! first  time, and secondly when checkpoint branches schedule which then 
! calls  __swtch
! We are splling while changing sp and pc. Note a side effect of 
! flush windows is to set the WIM to be CWP + 1. The restore causes an
! underflow which moves the previously saved in's and locals from memory to
! the register file. The outs are the current window's ins.
!
LENTRY(swtch)
	mov	%psr, %g2		! save the psr
	or	%g2, PSR_PIL, %g1	! splhigh
	mov	%g1, %psr
	nop; nop
	save	%sp, -SA(MINFRAME), %sp
	call	_flush_windows
	nop
	sethi	%hi(___Curproc), %o2
	ld	[%o2 + %lo(___Curproc)], %o1	! %o1 = __Curproc
	ld	[%o1+PC_OFFSET], %i7	! restore pc into HIS %o7
	ld	[%o1+SP_OFFSET], %fp	! restore sp into HIS %o6
	mov	%psr, %g1		! restore processor priority
	andn	%g1, PSR_PIL, %g1
	and	%g2, PSR_PIL, %g2
	or	%g1, %g2, %g2
	mov	%g2, %psr
	nop; nop
	restore				! checkpoint
					! get back to the window where
					! checkpoint was called, with underflow
					! restoring %i's and %l's
					! ASSERT(%sp is valid) before restore
1:	retl				! jump to lowinit or back to caller of
	nop

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


! __stacksafe()
! __stacksafe returns 1 if the stack is safe, else 0.
! "safe" means that the sp is not below the
! base of the stack (___CurStack + CKSTKOVERHEAD).
! It is used by check macro.
!
LENTRY(stacksafe)
	sethi	%hi(___CurStack), %o1	! CurStack is cur. thread stack bottom
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

! __setstack()
! Give the nugget a stack to use when no lwps are running
! Flush is necessary in case we later remove stack, nugget
! overflows its windows, and tries to flush.
!
LENTRY(setstack)
	sethi	%hi(___NuggetSP), %o1
	retl
	ld	[%lo(___NuggetSP) + %o1], %sp
