/* Copyright (C) 1986 Sun Microsystems Inc. */

#include <lwp/common.h>
#include <lwp/queue.h>
#include <machlwp/machdep.h>
#include <lwp/lwperror.h>
#include <lwp/schedule.h>
#include <lwp/alloc.h>
#include <lwp/condvar.h>
#include <lwp/process.h>
#include <lwp/monitor.h>
#include <lwp/cntxt.h>
#ifndef lint
SCCSID(@(#) schedule.c 1.1 92/07/30 Copyr 1987 Sun Micro);
#endif lint

lwp_t Nullproc;			/* have something for lwp_errno until init */
lwp_t *__Curproc = &Nullproc;	/* the current running process */
qheader_t *__RunQ = QHNULL;	/* set of run queues (1 per priority) */
int __MaxPrio = MINPRIO;	/* highest available scheduling priority */
int __UniqId = 0;		/* unique id for locks and keys */

extern label_t kernelret;

/*
 * __lwp_init: initialize library.
 * N.B. Order of initialization is important.
 */
void
__lwp_init()
{
	extern void __init_util();

	INIT_QUEUE(&Nullproc.lwp_ctx);
	Nullproc.lwp_prio = NUGPRIO;
	__RunQ = (qheader_t *)MEM_ALLOC(sizeof (qheader_t) * (__MaxPrio + 1));
	__init_lwp();
	__init_mon();
	__init_cv();
	__init_machdep();
	__init_ctx();
	__init_util();
}

/*
 * Scheduler loop for the set of lwps.
 * Find the highest priority runnable lwp and set it going.
 * If returning from an interrupt to the same process, it is probably cheaper
 * to restore the saved context than to return back through the call
 * chain to the rti.
 * We had to always save full context on interrupt because we didn't know
 * in advance if this would be the case.
 */

void
__schedule()
{
	register int i;
	register lwp_t *oldproc = __Curproc;
	register lwp_t *newproc;
	register qheader_t *rq;
	int s;

	s = splclock();
#ifdef	MULTIPROCESSOR
	klock_require(__FILE__, __LINE__, "__schedule");
#endif	MULTIPROCESSOR
	LWPTRACE(tr_SCHEDULE, 5, ("scheduling\n"));

	for (i = __MaxPrio; i >= NUGPRIO; i--) {
		rq = &__RunQ[i];
#ifndef	MULTIPROCESSOR
		if (!ISEMPTYQ(rq)) {
			newproc = __Curproc = (lwp_t *)FIRSTQ(rq);
#else	MULTIPROCESSOR
		for (newproc = (lwp_t *)FIRSTQ(rq);
		     newproc != (lwp_t *)0;
		     newproc = newproc->lwp_next)
		    if (lwp_can_run(newproc)) {
			__Curproc = newproc;
#endif	MULTIPROCESSOR
			if (((oldproc != LWPNULL) &&
			     !ISEMPTYQ(&oldproc->lwp_ctx)) ||
			    !ISEMPTYQ(&newproc->lwp_ctx)) {
				__ctx_change(oldproc, newproc);
			}
			LWPTRACE(tr_SCHEDULE, 2,
				 ("about to schedule %x\n", (int)__Curproc));
			if (!newproc->lwp_full) {

				/*
				 * do a simple context switch (only non-volatile
				 * registers, no temps or sr.)
				 * This is the case on a yield or where
				 * there is no asynchronous event indirectly
				 * responsible for the context switch (i.e.,
				 * waiting for a monitor or a message).
				 */

				LWPTRACE(tr_SCHEDULE, 2, ("swtch call\n"));
				(void) splx(s);
				CSWTCH();

				/*NOTREACHED*/

			} else {

				/*
				 * do a full context switch,restoring temps,sr,
				 * etc., as needed.
				 */

				LWPTRACE(tr_SCHEDULE, 2, ("full swtch call\n"));
				__panic("full_swtch attempt");
				/* NOTREACHED */
			}
			__panic("return from swtch");
			/* NOTREACHED */
		}
	}

	LWPTRACE(tr_SCHEDULE, 1, ("schedule exiting\n"));

	/* return to resume, etc. */
	__Curproc = &Nullproc;
	(void) splx(s);
#ifdef	MULTIPROCESSOR
	klock_require(__FILE__, __LINE__, "__schedule");
#endif	MULTIPROCESSOR
	longjmp(&kernelret);
}
