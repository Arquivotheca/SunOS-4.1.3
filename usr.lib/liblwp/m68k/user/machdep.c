/* Copyright (C) 1986 Sun Microsystems Inc. */
/*
 * This source code is a product of Sun Microsystems, Inc. and is provided
 * for unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify this source code without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 *
 * THIS PROGRAM CONTAINS SOURCE CODE COPYRIGHTED BY SUN MICROSYSTEMS, INC.
 * AND IS LICENSED TO SUNSOFT, INC., A SUBSIDIARY OF SUN MICROSYSTEMS, INC.
 * SUN MICROSYSTEMS, INC., MAKES NO REPRESENTATIONS ABOUT THE SUITABLITY
 * OF SUCH SOURCE CODE FOR ANY PURPOSE.  IT IS PROVIDED "AS IS" WITHOUT
 * EXPRESS OR IMPLIED WARRANTY OF ANY KIND.  SUN MICROSYSTEMS, INC. DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO SUCH SOURCE CODE, INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  IN
 * NO EVENT SHALL SUN MICROSYSTEMS, INC. BE LIABLE FOR ANY SPECIAL, INDIRECT,
 * INCIDENTAL, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING
 * FROM USE OF SUCH SOURCE CODE, REGARDLESS OF THE THEORY OF LIABILITY.
 * 
 * This source code is provided with no support and without any obligation on
 * the part of Sun Microsystems, Inc. to assist in its use, correction, 
 * modification or enhancement.
 *
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY THIS
 * SOURCE CODE OR ANY PART THEREOF.
 *
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California 94043
 */
#include "common.h"
#include "queue.h"
#include "asynch.h"
#include "machsig.h"
#include "machdep.h"
#include "cntxt.h"
#include "lwperror.h"
#include "message.h"
#include "process.h"
#include "schedule.h"
#include "agent.h"
#include "alloc.h"
#include "monitor.h"
#include <signal.h>
#include "libc/libc.h"
#include <machine/psl.h>
#ifndef lint
SCCSID(@(#) machdep.c 1.1 92/07/30 Copyr 1987 Sun Micro);
#endif lint

/*
 * PRIMITIVES contained herein:
 * lwp_setregs(tid, machstate)
 * lwp_getregs(tid, machstate)
 */

/*
 * The routines here, in except.c, in machsig.c, in lwputil.c,
 * and those in low.s are machine-dependent parts that need to be changed
 * when porting to a different environment.
 * alloc.[ch] may also require modification.
 */

/*
 * lwp_setregs -- PRIMITIVE.
 * Set the context (registers) for a given thread.
 * Can be used to simulate longjmp-like effects.
 */
int
lwp_setregs(tid, machstate)
	thread_t tid;
	register machstate_t *machstate;
{
	register lwp_t *pid = (lwp_t *)(tid.thread_id);

	CLR_ERROR();
	LOCK();
	ERROR((pid->lwp_lock != tid.thread_key), LE_NONEXIST);
	bcopy((caddr_t)machstate, (caddr_t)&(pid->lwp_machstate),
	  sizeof(machstate_t));
        pid->lwp_full = TRUE;	/* so can set sr, etc. */
	UNLOCK();
	return (0);
}

/*
 * lwp_getregs -- PRIMITIVE.
 * Retrieve the machine registers for a given thread.
 */
int
lwp_getregs(tid, machstate)
	thread_t tid;
	register machstate_t *machstate;
{
	register lwp_t *pid = (lwp_t *)(tid.thread_id);

	CLR_ERROR();
	LOCK();
	ERROR((pid->lwp_lock != tid.thread_key), LE_NONEXIST);
	bcopy((caddr_t)&(pid->lwp_machstate), (caddr_t)machstate,
	  sizeof(machstate_t));
        if (!pid->lwp_full) {	/* Get PC that will be valid at next swtch() */
                register stkalign_t *sp = (stkalign_t *)pid->lwp_context[SP];
                if (sp != STACKNULL)
                        machstate->mc_temps[TEMPPC] = *(int *)sp;
        }
	UNLOCK();
	return (0);
}

stkalign_t __IdleStack[IDLESTACK];	/* stack for idle thread */
stkalign_t *__NuggetSP;			/* nugget stack */

/*
 * lightweight process to wait for interrupts.
 */
void
__idle() {
	int i;

	LWPTRACE(tr_SCHEDULE, 2, ("idle\n"));
	for (;;) {
		LWPTRACE(tr_SCHEDULE, 6, ("."));
		/*
		 * Test for termination. Any lwp's created by
		 * the client have had a chance to run before we get
		 * here (idle runs at a priority lower than tha
		 * client can run). If idle() is the only guy left, there
		 * is no hope of a signal coming in as all agents have
		 * been destroyed. 
		 */
		if (__LwpRunCnt == __NActiveSlaves + 1) { /* only slaves, idle */
			break;
		} else {
			/* analogous to PDP-11 WAIT or 68000 STOP instruction */
			(void) sigpause(0);	/* avoid chewing up cycles */
		}
	}
	for (i = 0; i < __SlaveThreads; i++) {
		if (!SAMETHREAD(__Slaves[i], THREADNULL))
			(void)lwp_destroy(__Slaves[i]);
	}
	__do_exithand();
}

/*
 * Called by interrupt upon a caught UNIX signal.
 * Other interrupts are masked during signal processing
 * (signals are set up to not nest. The event list is only accessed
 * at highest priority).
 * Return 0 if we don't need to save full state; else 1.
 * Note that we could in theory return from do_agent
 * if no higher-priority process is awakened. 
 * We don't currently do this optimisation.
 */
int
__real_sig(signum, code, cntxt, addr)
	int signum;
	int code;
	struct sigcontext *cntxt;
	caddr_t addr;
{
	register int *temps;

	/*
	 * This code uses the client stack to raise the exception
	 * on. There are problems if the exception was due to stack
	 * overflow. On the other hand, we don't want to execute
	 * on the nugget stack on behalf of the client since
	 * the nugget isn't scheduled and has a fixed stack and
	 * the client is not predictable (e.g., infinite loop).
	 */
	if (__Eventinfo[signum].int_start == TRAPEVENT) { /* synchronous trap */
		*(--(int *)cntxt->sc_sp) = cntxt->sc_pc;
		*(--(int *)cntxt->sc_sp) = signum;
		cntxt->sc_pc = (int)__sigtrap;
		return(0);
	}

	/*
	 * indicate that interrupts are masked. No need to DISABLE
	 * since interrupts don't nest and we're already at interrupt
	 * prio.
	 */
	__SigLocked++;

	/* save signal state (volatile device registers) */
	__save_event(__Curproc, signum, code, addr);

	if (IS_LOCKED()) {	/* in a critical section */
		/*
		 * __save_event()
		 * must be careful to avoid using shared memory structures
		 * used for interrupt handling
		 * since LOCK() is the way critical sections are protected
		 * and LOCK() does not mask interrupts.
		 * __EventsPending remains TRUE since we haven't
		 * completely processed the signal yet.
		 * Note that the lowest-level code taking the interrupt
		 * is careful to ensure that pre-interrupt machine state
		 * is preserved.
		 */
		LWPTRACE(tr_ASYNCH, 1, ("CRITICAL SECTION LOCKED\n"));
		__SigLocked = 0;
		return(0);
	} else {	/* not in a critical section */
		LOCK(); /* Now in critical section; so LOCK-UNLOCK work ok */
		temps = __Curproc->lwp_temps;
		temps[TEMPPC] = cntxt->sc_pc;
		temps[TEMPPS] = cntxt->sc_ps;

		/* fixup stack to be accurate */
		__Curproc->lwp_context[SP] = cntxt->sc_sp;
		__Curproc->lwp_full = TRUE;

		LWPTRACE(tr_ASYNCH, 1, ("realsig %d saved full context\n", signum));
		/* 
		 * return to assembly language to save all regs but sp and call
		 * do_agent.
		 */
		return(1);
	}
}

/* Hold sigcleanup argument here */
struct sigcontext __Context;

/*
 * Full context switch to __Curproc. We use full_swtch whenever
 * context switching as a result of an interrupt (to return
 * to pre-interrupt priority), or switching to a process that
 * was scheduled out as a result of an asynchronous event.
 * In the former case, if the process was not previously scheduled
 * out as a result of an interrupt, we dummy up a full context
 * for sigcleanup.
 * Always called while LOCKED (once a context is selected by the scheduler,
 * don't want to trash its state by taking an interrupt before DISABLING).
 * Restore the PS, PC, volatile and nonvolatile registers saved
 * in __Curproc's context.
 * This routine also has the job of restoring interrupt priority,
 * signal mask and signal stack to the pre-interrupt state.
 * This is done (atomically) by sigcleanup.
 */
void
__full_swtch()
{
	register int *temps;
	register lwp_t *cp = __Curproc;
	register struct sigcontext *cntxt = &__Context;

	DISABLE();	/* in case we were not called at interrupt level */

	/*
	 * If schedule() was interrupted, for example, __EventsPending
	 * can be TRUE when schedule() calls full_swtch().
	 * do_agent will enter the scheduler with interrupts disabled
	 * so there is no possible recursion here.
	 */
	if (__EventsPending) {
		__do_agent();
	}
	temps = __Curproc->lwp_temps;
	if (!cp->lwp_full) {	/* dummy up a pc */
		temps[TEMPPS] = PSL_USER;
		temps[TEMPPC] = *(int *)(cp->lwp_context[SP]);
		/*
		 * We won't do a normal CSWTCH because we need to escape
		 * high prio via rti (sigcleanup).
		 * Thus, the frame that WAIT pushed needs to get discarded
		 * and we pop the WAIT stack frame (just the ret address).
		 * Note that a brand-new thread also has a single frame
		 * to be popped (the call from the exit routine to the start
		 * routine). This effect isn't needed though as it is
		 * impossible for a virgin thread to get scheduled as a
		 * result of an event (none can be established yet for it).
		 */
		cp->lwp_context[SP] += 4;
	} else {
		cp->lwp_full = FALSE;
	}
	LWPTRACE(tr_ASYNCH, 2,
	  ("FULL SWTCH: ABOUT TO RETURN TO %x\n", temps[TEMPPC]));

	/*
	 * remove __SigLocked and __AsynchLock to achieve the state that
	 * we will safely have when sigcleanup returns.
	 * Since we return to unlocked state, we must zero rather
	 * than decrement.
	 */
	LOCK_OFF();

	/* dummy up a context for sigcleanup */
	cntxt->sc_pc = temps[TEMPPC];
	cntxt->sc_sp = cp->lwp_context[SP];
	cntxt->sc_ps = temps[TEMPPS];
	cntxt->sc_onstack = FALSE;
	cntxt->sc_mask = 0;	/* permit all interrupts */

	/* 
	 * restore registers now that we're done using them.
	 * Can't use the register variables anymore.
	 */
	asm ("movl ___Curproc, a0");	/* pointer to client context */
	asm ("moveml a0@(8), #0xfcfc");	/* REGOFFSET is 8 NB: changes sp */
	asm ("moveml a0@(8+48), #0x0303");	/* restore temps */

	/* atomically return to client via sigcleanup */
	SETSTACK();	/* don't use client sp (can't omit sp in prior moveml) */
	asm ("pea ___Context");
	asm ("pea 139");
	asm ("trap #0");
	__panic("return from sigcleanup");
}

/* generally useful printf for debugging low.s */
/* VARARGS0 */
/* ARGSUSED */
void
__asmdebug(a1, a2, a3, a4, a5, a6)
	int a1, a2, a3, a4, a5, a6;
{
	LWPTRACE(tr_ASM, 1, ("ASMDEBUG: %d %d %d %d %d %d\n", a1, a2, a3, a4, a5, a6));
}

/* dump the stack pointer. trick: not called with any args. */
/* VARARGS0 */
/* ARGSUSED */
void
__dumpsp(retadr)
	unsigned int *retadr;
{
#ifdef lint
	retadr = retadr;
#endif lint
	LWPTRACE(tr_ASM, 1, ("stack is %x\n", (int) &retadr));
}

STATIC stkalign_t Sigstk[INTSTKSZ];	/* stack for handling interrupts */

/* Signal stack */
STATIC struct sigstack Sss = {
	(caddr_t) STACKTOP(Sigstk, INTSTKSZ),	/* stack to use for signals */
	FALSE,					/* not currently using it */
};

/*
 * Create a stack for the nugget to use. This should not come from
 * paged memory.
 * Create a stack for interrupts to operate on.
 */
void
__init_machdep()
{
	__NuggetSP = (stkalign_t *)
	  STACKALIGN(MEM_ALLOC(NUGSTACKSIZE) + NUGSTACKSIZE);
	if (sigstack(&Sss, (struct sigstack *)0) < 0) {
		perror("__init_machdep");
		__panic("sigstack call failed");
	}
}
