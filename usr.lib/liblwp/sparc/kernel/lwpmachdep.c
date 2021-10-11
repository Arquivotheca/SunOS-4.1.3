/* Copyright (C) 1987 Sun Microsystems Inc. */


#include <lwp/common.h>
#include <lwp/queue.h>
#include <lwp/asynch.h>
#include <machlwp/machsig.h>
#include <machlwp/machdep.h>
#include <lwp/cntxt.h>
#include <lwp/lwperror.h>
#include <lwp/message.h>
#include <lwp/process.h>
#include <lwp/schedule.h>
#include <lwp/agent.h>
#include <lwp/alloc.h>
#include <lwp/monitor.h>
#include <machine/psl.h>
#include <machine/asm_linkage.h>

#ifndef lint
SCCSID(@(#) lwpmachdep.c 1.1 92/07/30 Copyr 1987 Sun Micro);
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

stkalign_t *__NuggetSP;		/* nugget stack (when no lwps are running) */

/*
 * The routines here, in except.c, in machsig.c, in cntxt.c,
 * and those in low.s are machine-dependent parts that need to be changed
 * when porting to a different environment.
 * alloc.[ch] may also require modification, especially when paged
 * memory is needed.
 */

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
	panic("full swtch");
}

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
}
