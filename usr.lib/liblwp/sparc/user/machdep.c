/* Copyright (C) 1987 Sun Microsystems Inc. */
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
#include <sun4/psl.h>
#include <sun4/asm_linkage.h>
#ifndef lint
SCCSID(@(#) machdep.c 1.1 92/07/30 Copyr 1987 Sun Micro);
#endif lint

int __AllFlushed;	/* TRUE if windows currently flushed */

/*
 * PRIMITIVES contained herein:
 * lwp_setregs(tid, machstate)
 * lwp_getregs(tid, machstate)
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
	pid->lwp_full = TRUE;	/* allow psr, etc. to be loaded too */
	UNLOCK();
	return (0);
}

/*
 * lwp_getregs -- PRIMITIVE.
 * Get the context (registers) for a given thread.
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
	if (!pid->lwp_full) {	/* where he will be at next swtch() */
		machstate->sparc_pc += 8;
		machstate->sparc_npc += 8 + 4;
	}
	UNLOCK();
	return (0);
}

stkalign_t __IdleStack[IDLESTACK];	/* stack for idle thread */
stkalign_t *__NuggetSP;		/* nugget stack (when no lwps are running) */

/*
 * The routines here, in except.c, in machsig.c, in cntxt.c,
 * and those in low.s are machine-dependent parts that need to be changed
 * when porting to a different environment.
 * alloc.[ch] may also require modification, especially when paged
 * memory is needed.
 */

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
		if (__LwpRunCnt == __NActiveSlaves + 1) {/* only slaves, idle */
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
 * Upon a caught UNIX signal, __interrupt() is invoked (on the interrupt stack).
 * Unix takes care of flushing the windows for us.
 * This routine saves %g's and %o's (since flushing the latter
 * would save them on the (reused) interrupt stack).
 * It then invokes __real_sig with interrupts disabled
 * (signals are set up to not nest. The event list is only accessed
 * at highest priority).
 * If LOCKED, __interrupt restores state and returns.
 * XXX There is room for tuning here since low.s copies things
 * that real_sig copies as well.
 */
void
__real_sig(signum, code, cntxt, addr, regs, yreg)
	int signum;
	int code;
	register struct sigcontext *cntxt;
	caddr_t addr;
	int *regs;	/* g2-7, o0-7 */
	long yreg;	/* %y */
{
	register lwp_t *cp = __Curproc;

	if (__Eventinfo[signum].int_start == TRAPEVENT) { /* synchronous trap */
		*(--(int *)cntxt->sc_sp) = cntxt->sc_pc;
		*(--(int *)cntxt->sc_sp) = signum;
		cntxt->sc_pc = (int)__sigtrap;
		cntxt->sc_npc = cntxt->sc_pc + 4;
		return;
	}

	/*
	 * indicate that interrupts are masked. No need to DISABLE
	 * since interrupts don't nest and we're already at interrupt
	 * prio.
	 */
	__SigLocked++;

	if (IS_LOCKED()) {	/* in a critical section */
		/*
		 * Just save signal info and return.  Since we were in
		 * a critical section when the interrupt arrived,
		 * __save_event() must be careful to avoid using shared
		 * memory structures.  __EventsPending remains TRUE since
		 * we haven't completely processed the signal yet.
		 */
		__save_event(__Curproc, signum, code, addr);
		LWPTRACE(tr_ASYNCH, 1, ("CRITICAL SECTION LOCKED\n"));
		__SigLocked = 0;
	} else { /* not in a critical section */

		LOCK();	/* Now in critical section; so LOCK-UNLOCK work ok */
		cp->lwp_full = TRUE;
		{
			register int i;
			register int *r = regs;

			/* 
			 * save all context. UNIX saved %o0, %o6, pc and %g1.
			 */
			for (i = 0; i < 6; i++)
				cp->mch_xglobals[i+2] = *r++;
			for (i = 0; i < 8; i++)
				cp->mch_xoregs[i] = *r++;
		}
			
		/* save signal state (volatile device registers) */
		cp->mch_y = yreg;
		cp->mch_sp = cntxt->sc_sp;
		cp->mch_pc = cntxt->sc_pc;
		cp->mch_npc = cntxt->sc_npc;
		cp->mch_g1 = cntxt->sc_g1;
		cp->mch_o0 = cntxt->sc_o0;
		cp->mch_psr = cntxt->sc_psr;

		__save_event(cp, signum, code, addr);

		LWPTRACE(tr_ASYNCH, 1, ("realsig %d saved full context\n", signum));
		__do_agent();
	}
}

/* Hold info to pass to system via sigcleanup() on rti() */
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
 * Restore the volatile and nonvolatile registers saved
 * in __Curproc's context.
 * This routine also has the job of restoring interrupt priority,
 * signal mask and signal stack to the pre-interrupt state.
 * This is done (atomically) by sigcleanup.
 */
void
__full_swtch()
{
	register struct sigcontext *cntxt = &__Context;
	register lwp_t *cp = __Curproc;

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
	/*
	 * remove __SigLocked and __AsynchLock to achieve the state that
	 * we will safely have when sigcleanup returns.
	 * Since we return to unlocked state, we must zero rather
	 * than decrement.
	 */
	LOCK_OFF();
	cntxt->sc_onstack = FALSE;
	cntxt->sc_mask = 0;  /* permit all interrupts */
	cntxt->sc_sp = cp->mch_sp;
	if (!cp->lwp_full) {
		/*
		 * We won't do a normal CSWTCH because we need to escape
		 * high prio via rti.
		 * Dummy up a sigcontext and let rti() return
		 * us to where we were at the coroutine call.
		 */
		cntxt->sc_pc = cp->mch_pc + 8;
		cntxt->sc_psr = PSL_USER;

		  /* from coroutine call, add 8 to pc to simulate ret */
		cntxt->sc_npc = cp->mch_pc + 8 + 4;
	} else {
		cp->lwp_full = FALSE;
		cntxt->sc_pc = cp->mch_pc;
		cntxt->sc_npc = cp->mch_npc;
		cntxt->sc_g1 = cp->mch_g1;
		cntxt->sc_o0 = cp->mch_o0;
		cntxt->sc_psr = cp->mch_psr;
	}
	LWPTRACE(tr_ASYNCH, 2, ("FULL SWTCH: ABOUT TO RETURN TO %x, <pc %x sp %x>\n",
	  cp, cntxt->sc_pc, cntxt->sc_sp));
	__rti();
	__panic("return from sigcleanup");
}

STATIC stkalign_t Sigstk[INTSTKSZ];	/* stack for handling interrupts */

/* Signal stack */
STATIC struct sigstack Sss = {
	/* stack to use for signals. Leave WINDOWSIZE in case need to flush. */
	(caddr_t)STACKTOP(Sigstk, INTSTKSZ - (WINDOWSIZE / sizeof(int))),
	FALSE, /* not currently using it */
};

/*
 * Create a stack for the nugget to use. This should not come from
 * paged memory. Allow a frame on this stack just in case.
 * Create a stack for interrupts to operate on.
 */
void
__init_machdep()
{
	__AllFlushed = FALSE;
	__NuggetSP = (stkalign_t *)
	  (STACKALIGN
	  (((int)MEM_ALLOC(NUGSTACKSIZE)) + NUGSTACKSIZE - MINFRAME));
	if (sigstack(&Sss, (struct sigstack *)0) < 0) {
		perror("__init_machdep");
		__panic("sigstack call failed");
	}
}
