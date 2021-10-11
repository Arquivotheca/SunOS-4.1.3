.data
.asciz "@(#) low.s 1.1 92/07/30 Copyr 1986 Sun Micro"
.even
.text

| Copyright (C) 1986 by Sun Microsystems, Inc.
! @(#) low.s 1.1 92/07/30 
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


#include "low.h"

.text
.globl	___schedule
.globl	___real_sig
.globl	___do_agent
.globl	___asmdebug
.globl	___Curproc
.globl	___EventsPending
.globl	___AsynchLock
.globl	___fake_sig
.globl	___exchelpclean
.globl	___exc_trap
.globl	___NuggetSP


|
| assembly language support routines
|

| a7 is the stack

| __swtch()
| switch context when not at interrupt level and the new process
| was not switched out asynchronously.
| Volatile registers are not restored (d0/d1, a0/a1).
| Must be called while LOCKED.
|
| Context is restored BEFORE UNLOCK (i.e., clearing AsynchLock)
| so that any interrupt will capture the correct context
| (i.e., save the "backing store" version of the registers,
| not the values in the registers at the time ___swtch is called).
| We LOCK before calling fake_sig to prevent intrerrupts from
| changing anything before fake_sig causes a scheduling decision.
| We UNLOCK before testing EventsPending so a interrupt will
| cause scheduling (and not just save info and set EventsPending),
| even if we decided that EventsPending was false
| (and then immediately got an interrupt).
| Once we decided that EventsPending is true, it is safe to LOCK
| again because setting EventsPending is idempotent.

ENTRY(swtch)
	movl	___Curproc, a0		| pointer to client context
	moveml	a0@(REGOFFSET), #0xfcfc	| restore client regs and new sp
	clrl	___AsynchLock		| allow interrupts
	tstl	___EventsPending	| check for pending events
	jeq	1f			| none
	movl	#1, ___AsynchLock	| prevent interrupts from touching regs
	movl	___NuggetSP, sp		| don't use client stack for interrupts
	jbsr	___fake_sig		| fake an interrupt
	| NOTREACHED
1:	rts				| back to client

| __checkpoint()
| Save context and reschedule.
| The context will have the return address of the caller of
| ___checkpoint on the stack so upon rescheduling it will look
| like checkpoint() just returned.
| Should be called while LOCKED. When checkpoint() "returns" though,
| the state is UNLOCKED.

ENTRY(checkpoint)
	movl	___Curproc, a0
	moveml	#0xfcfc, a0@(REGOFFSET)	| save volatile registers
	movl	___NuggetSP, sp		| don't use client stack now
	jra	___schedule
	| NOTREACHED


| __unlock()
| Unlock a critical section.
| If events occurred while LOCKED, call fake_sig
| to simulate an interrupt and cause all pending events to be processed.
| Context must be saved before calling fake_sig because the saved context
| does not reflect reality and fake_sig will cause a scheduling decision.
| Context is saved while LOCKED to prevent interrupts from preserving
| an incorrect state before the correct state is established.
| EventsPending is tested while UNLOCKED to avoid deadlocks as described in
| the swtch() code.

ENTRY(unlock)
	clrl	___AsynchLock
	tstl	___EventsPending
	jeq	1f
	movl	#1, ___AsynchLock	| prevent interrupts from touching regs
	movl	___Curproc, a0
	moveml	#0xfcfc, a0@(REGOFFSET)	| save return cntxt; __fake_sig doesn't
	movl	___NuggetSP, sp		| don't use client stack now
	jbsr	___fake_sig
	| NOTREACHED
1:	rts

| __excreturn()
| return value to exception handler upon an excraise.
| the return address is on the stack.
| note that normal context switching does not restore d0
| so we do the exception handle return
| as if the user was calling a function returning a value.

ENTRY(excreturn)
	movl	sp@+, d0		| return pattern
	rts

| __exccleanup()
| a procedure handling exceptions has its return address replaced
| by exccleanup.
| initially, the procedure return address is at the top of the stack.
| it is replaced by exccleanup which is called instead of returning.
| We pretend that the stack is as it would be if we were about to
| return to the procedure. __exchelpclean makes this true and we return.

ENTRY(exccleanup)
	subqw	#4, sp			| pretend real return address is there
	movl	d0, sp@-		| save return value if any
	jsr	___exchelpclean		| this guy restores the original ret addr
	movl	sp@+, d0		| restore return value
	rts

| __sigtrap()
| signum is on the stack
|
ENTRY(sigtrap)
	moveml	#0xC0C0, sp@-	/* save C scratch regs */
	movl	sp@(16), sp@-
	jsr	___exc_trap
	addqw	#4, sp
	moveml	sp@+,#0x0303	/* restore regs */
	addqw	#4, sp
	rts

| __interrupt()
| our own version of sigtramp with no floating point stuff saved
| called when an interrupt or trap occurs.

ENTRY(interrupt)
	moveml	a0/a1/d0/d1,sp@-	/* save C scratch regs */
	movl	sp@(0+28),sp@-		/* push addr */
	movl	sp@(4+24),sp@-		/* push scp address */
	movl	sp@(8+20),sp@-		/* push code */
	movl	sp@(12+16),sp@-		/* push signal number */
	jsr	___real_sig		/* call realsig */
	addl	#16,sp			/* pop args */
	tstl	d0			/* need to save full state? */
	jeq	1f			/* no */
	movl	___Curproc, a0
	moveml	a2-a6/d2-d7, a0@(REGOFFSET)	| save general regs except sp
	movl	sp@(0), a0@(REGOFFSET+48+0)	| d0
	movl	sp@(4), a0@(REGOFFSET+48+4)	| d1
	movl	sp@(8), a0@(REGOFFSET+48+8)	| a0
	movl	sp@(12), a0@(REGOFFSET+48+12)	| a1
	jsr	___do_agent			| map interrupt
1:
	moveml	sp@+,a0/a1/d0/d1	/* restore regs */
	addl	#8,sp			/* pop signo, code, scp now on top */
	pea	139			/* call sigcleanup(scp) */
	trap	#0
	/*NOTREACHED*/

| __fp_save()
| routines to save and restore floating point context.
| Each is called with a pointer to the floating point context as an arg.
| This context has FPSAVESIZE bytes of space.

#define FP_UNSPECIFIED	0
#define FP_SOFTWARE	1
#define FP_SKYFFP	2
#define FP_MC68881	3
#define FP_SUNFPA	4

.globl	_getfptype, ffpa_save, f68881_save, fsky_save
ENTRY(fp_save)
	jsr	_getfptype		| get fp_switch value into d0
	movl	sp@(4), a0		| get pointer to fp context
	movel	d0, a0@+		| save fp_switch for fp_restore
	cmpw	#FP_SUNFPA, d0
	jne	3f
	jsr	ffpa_save
	bras	9f
3:
	cmpw	#FP_MC68881, d0
	bnes	2f
	jsr	f68881_save
	bras	9f
2:
	cmpw	#FP_SKYFFP, d0
	bnes	9f
	jsr	fsky_save
9:
	rts

| __fp_restore()
.globl	ffpa_restore, f68881_restore, fsky_restore
ENTRY(fp_restore)
	movl	sp@(4), a0
	movel	a0@+, d0	| retrieve saved fp_switch
	cmpw	#FP_SUNFPA, d0
	jne	3f
	jsr	ffpa_restore
	bras	9f
3:
	cmpw	#FP_MC68881, d0
	bnes	2f
	jsr	f68881_restore
	bras	9f
2:
	cmpw	#FP_SKYFFP, d0
	bnes	9f
	jsr	fsky_restore
9:
	rts

| stripped-down version of fp stuff from /usr/src/lib/libc/crt/fp_save.s
| with ALL regs saved/restored

#define	COMREG	-4	/* Sky command register offset from __skybase. */
#define	STCREG	-2	/* Sky status register offset from __skybase. */
#define SKYSAVE 0x1040	/* Sky context save command. */
#define SKYRESTORE 0x1041 /* Sky context restore command. */
#define SKYNOP 0x1063 	/* Sky noop command. */
#define IOCTL	0x40020000
#define	FPABASEADDRESS	0xe0000000

f68881_save:
	fmovem	fp0-fp7, a0@
	fmovem	fpcr/fpsr/fpiar, a0@(96)
	rts

f68881_restore:
	fmovem	a0@+, fp0-fp7
	fmovem	a0@, fpcr/fpsr/fpiar
	rts

.globl	_getskyfd
fsky_save:
	movel	__skybase,a1
1:
	movew	a1@(STCREG),d0
	btst	#14,d0
	beqs	2f			| Branch if busy.
	movew	#SKYNOP,a0@(32)
	bras	8f
2:
	tstw	a1@(STCREG)
	bpls	1b			| Branch if i/o not ready.

	pea	a0@(32)			| Push address of pc save area.
	movel	#IOCTL,sp@-
	jsr	_getskyfd		| obtain __sky_fd
	movl	d0,sp@-			| Push sky descriptor.

skyioctl:
	jsr	_ioctl
	addl	#12,sp			| Remove parameters.
8:
	movew	#SKYNOP,a1@(COMREG)
	movew	#SKYSAVE,a1@(COMREG)
	movel	a1@,a0@
	movel	a1@,a0@(4)
	movel	a1@,a0@(8)
	movel	a1@,a0@(12)
	movel	a1@,a0@(16)
	movel	a1@,a0@(20)
	movel	a1@,a0@(24)
	movel	a1@,a0@(28)
	rts

fsky_restore:
	movel	__skybase,a1
1:
	movew	a1@(STCREG),d0
	btst	#14,d0
	beqs	2f			| Branch if busy.
	bras	8f
2:
	tstw	a1@(STCREG)
	bpls	1b			| Branch if i/o not ready.

	pea	a0@(36)			| Push address of scratch area.
	movel	#IOCTL,sp@-
	jsr	_getskyfd
	movel	d0,sp@-			| Push sky descriptor.
rskyioctl:
	jsr	_ioctl
	addl	#12,sp			| Remove parameters.
8:
	movew	#SKYNOP,a1@(COMREG)
	movew	#SKYRESTORE,a1@(COMREG)
	movel	a0@,a1@
	movel	a0@(4),a1@
	movel	a0@(8),a1@
	movel	a0@(12),a1@
	movel	a0@(16),a1@
	movel	a0@(20),a1@
	movel	a0@(24),a1@
	movel	a0@(28),a1@
1:
	movew	a1@(STCREG),d0
	btst	#14,d0
	beqs	1b
	movew	a0@(32),a1@(COMREG)
	rts

ffpa_save:
	fmovem	fp0-fp7,a0@
	addl	#96,a0
	fmovem	fpcr/fpsr/fpiar,a0@+
	movel	#(FPABASEADDRESS+0xf14),a1
	movel	a1@+,a0@+		| Stable read imask.
	movel	a1@+,a0@+		| Stable read load_ptr.
	movel	a1@+,a0@+		| Stable read ierr.
	movel	a1@+,a0@+		| Active instruction.
	movel	a1@+,a0@+		| Active data 1.
	movel	a1@+,a0@+		| Active data 2.
	movel	a1@+,a0@+		| Next instruction.
	movel	a1@+,a0@+		| Next data 1.
	movel	a1@+,a0@+		| Next data 2.
	movel	a1@+,a0@+		| Stable read weitek mode.
	movel	a1@+,a0@+		| Stable read weitek status.
	movel	#0,0xe0000f84		| Clear pipe.
	movel	#(FPABASEADDRESS+0xe00),a1
	movel	a1@+,a0@+		| fpa0
	movel	a1@+,a0@+		| fpa0
	movel	a1@+,a0@+		| fpa1
	movel	a1@+,a0@+		| fpa1
	movel	a1@+,a0@+		| fpa2
	movel	a1@+,a0@+		| fpa2
	movel	a1@+,a0@+		| fpa3
	movel	a1@+,a0@+		| fpa3

|	Set up almost like a new context.

	movel	#(FPABASEADDRESS),a1
	movel	#0,a1@(0xf1c)		| Clear ierr.
	movel	#0,a1@(0xf14)		| Set imask = 0 since SIGFPE
					| may already be signalled.
	fpmove@1 #2,fpamode		| Set FPA mode to default.
	fmovel	#0,fpsr
	fmovel	#0,fpcr
	rts

ffpa_restore:
	movel	#0,0xe0000f84		|  Clear pipe.
	fmovem	a0@+,fp0-fp7
	fmovem	a0@+,fpcr/fpsr/fpiar
	addl	#44,a0
	movel	#(FPABASEADDRESS+0xc00),a1
	movel	a0@+,a1@+		| fpa0
	movel	a0@+,a1@+		| fpa0
	movel	a0@+,a1@+		| fpa1
	movel	a0@+,a1@+		| fpa1
	movel	a0@+,a1@+		| fpa2
	movel	a0@+,a1@+		| fpa2
	movel	a0@+,a1@+		| fpa3
	movel	a0@+,a1@+		| fpa3
	subl	#76,a0
	movel	#(FPABASEADDRESS+0xf14),a1
	movel	a0@+,a1@+ 		|  imask.
	movel	a0@+,a1@+		|  load_ptr.
	movel	a0@+,a1@+		|  ierr.
	movel	#(FPABASEADDRESS),a1
	movel	a0@(24),d0
	andl	#0xf,d0			| Clear all but bottom bits.
	fpmove@1	d0,fpamode
	fpmove@1	a0@(28),fpastatus
	movel	#0,a1@(0x9c4)		| Rewrite shadow registers!
	movel	a0@,d0			| Load active instruction.
	lea	a0@(8),a0		| Point to active data.
	jsr	ffpa_rewrite		| Restore active instruction.
	movel	a0@(-4),d0		| Load next instruction.
	lea	a0@(8),a0		| Point to next data.
	jsr	ffpa_rewrite		| Restore next instruction.
	rts

ffpa_rewrite:
	swap	d0
	btst	#15,d0
	bnes	9f			| Quit if invalid.
	andw	#0x1ffc,d0		| Clear bogus bits.
	movel	a0@,a1@(0,d0:w)
	swap	d0
	btst	#15,d0
	bnes	9f			| Quit if invalid.
	andw	#0x1ffc,d0		| Clear bogus bits.
	movel	a0@(4),a1@(0,d0:w)
9:
	rts
