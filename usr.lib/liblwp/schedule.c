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
#include <lwp/common.h>
#include <lwp/queue.h>
#include <lwp/asynch.h>
#include <machlwp/machsig.h>
#include <machlwp/machdep.h>
#include <lwp/cntxt.h>
#include <lwp/lwperror.h>
#include <lwp/schedule.h>
#include <lwp/alloc.h>
#include <lwp/agent.h>
#include <lwp/clock.h>
#include <lwp/condvar.h>
#include <machlwp/except.h>
#include <lwp/message.h>
#include <lwp/process.h>
#include <lwp/monitor.h>
#ifndef lint
SCCSID(@(#) schedule.c 1.1 92/07/30 Copyr 1986 Sun Micro);
#endif lint

#ifndef SHAREDLIB
int __LwpLibInit = 0;		/* non-zero if initialized */
#endif SHAREDLIB
int __HighPrio;			/* highest priority current runnable lwp */
lwp_t Nullproc;			/* have something for lwp_errno until init */
lwp_t *__Curproc = &Nullproc;	/* the current running process */
qheader_t *__RunQ = QHNULL;	/* set of run queues (1 per priority) */
int __MaxPrio = MINPRIO;	/* highest available scheduling priority */
int __UniqId = 0;		/* unique id for locks and keys */

/*
 * PRIMITIVES contained herein:
 * pod_setmaxpri(prio)
 * pod_getmaxsize()
 * pod_getmaxpri()
 */

/*
 * pod_getmaxsize() -- PRIMITIVE.
 * Return maximum possible size of pod.
 */
int
pod_getmaxsize()
{
	return (MAXPRIO);
}

/*
 * pod_getmaxpri() -- PRIMITIVE.
 * Return current highest legal prio for pod.
 */
int
pod_getmaxpri()
{
	return (__MaxPrio);
}

/* 
 * pod_setmaxpri -- PRIMITIVE.
 * set maximum pod size. Visible priorities range from MINPRIO to __Maxprio
 * so there are a total of __MaxPrio+1 priorities since we always allocate
 * a priority (NUGPRIO) for the library. In reality, priorities
 * range from [NUGPRIO .. __MaxPrio].
 */
int
pod_setmaxpri(prio)
	int prio;	/* new number of scheduling priorities */
{
	qheader_t *tempq = QHNULL;
	register int i;

	if (__LwpLibInit == 0) { /* initialize by hand to avoid 2 MEM_ALLOCS */
		__MaxPrio = prio;
		__LwpLibInit++;
		__lwp_init();
		return (0);
	}
	LOCK();
	ERROR((prio > MAXPRIO), LE_NOROOM);
	ERROR((prio < __MaxPrio), LE_INVALIDARG);
	if (prio == __MaxPrio) {
		UNLOCK();
		return (0);
	}
	tempq = (qheader_t *)MEM_ALLOC(sizeof (qheader_t) * (prio + 1));
	if ((tempq == QHNULL) || (__RunQ == QHNULL)) {
		__panic("Can't create pod");
		/* NOTREACHED */
	}

	/* copy RunQ to tempq */
	for (i = NUGPRIO; i <= __MaxPrio; i++) {
		tempq[i].hq_head = __RunQ[i].hq_head;
		tempq[i].hq_tail = __RunQ[i].hq_tail;
	}
	MEM_FREE(__RunQ, (sizeof (qheader_t) * (__MaxPrio + 1)));
	__MaxPrio = prio;
	__RunQ = tempq;
	UNLOCK();
	for (i = 0; i < __SlaveThreads; i++) {
		if (!SAMETHREAD(__Slaves[i], THREADNULL))
			(void)lwp_setpri(__Slaves[i], __MaxPrio);
	}
	return (0);
}

/*
 * __lwp_init: initialize library.
 * N.B. Order of initialization is important.
 */
void
__lwp_init()
{
	thread_t idlethread;
	extern void __init_util();

	INIT_QUEUE(&Nullproc.lwp_ctx);
	Nullproc.lwp_prio = NUGPRIO;
	__RunQ = (qheader_t *)MEM_ALLOC(sizeof (qheader_t) * (__MaxPrio + 1));
	__HighPrio = NUGPRIO;
	__init_lwp();
	__init_sig();
	__init_asynch();
	__init_agent();
	__init_msg();
	__init_mon();
	__init_exc();
	__init_cv();
	__init_clock();
	__init_machdep();
	__init_ctx();
	__init_stks();
	__init_util();
	__maincreate();

	/* don't use LWPSERVER flag: this guy runs at lowest priority */
	(void)lwp_create(&idlethread, __idle, MINPRIO, 0,
	  (stkalign_t *)&__IdleStack[IDLESTACK], 0);

	/* give idle lower than legal (for client) priority */
	REMX_ITEM(&__RunQ[MINPRIO], idlethread.thread_id);
	INS_QUEUE(&__RunQ[NUGPRIO], idlethread.thread_id);
	((lwp_t *)(idlethread.thread_id))->lwp_prio = NUGPRIO;
	((lwp_t *)(idlethread.thread_id))->lwp_lock = UNIQUEID();
}

/*
 * Scheduler loop for the set of lwps.
 * Find the highest priority runnable lwp and set it going.
 * Note that __HighPrio can be the priority of the process that
 * last did a WAIT but may not be the priority of the next
 * eligible process. In the worst case, since the process that did the WAIT
 * was at the highest priority currently, only a priority less than
 * __HighPrio can be eligible.
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
	LOCK();
	LWPTRACE(tr_SCHEDULE, 5, ("scheduling\n"));
	for (i = __HighPrio; i >= NUGPRIO; i--, __HighPrio--) {
		rq = &__RunQ[i];
		if (!ISEMPTYQ(rq)) {
			newproc = __Curproc = (lwp_t *)FIRSTQ(rq);
			if (((oldproc != LWPNULL) && !ISEMPTYQ(&oldproc->lwp_ctx))
 			  || !ISEMPTYQ(&newproc->lwp_ctx)) {
				__ctx_change(oldproc, newproc);
			}
			LWPTRACE(tr_SCHEDULE, 2,
			  ("about to schedule %x\n", (int)__Curproc));
			if (!newproc->lwp_full && (__SigLocked == 0)) {

				/*
				 * do a simple context switch (only non-volatile
				 * registers, no temps or sr.)
				 * This is the case on a yield or where
				 * there is no asynchronous event indirectly
				 * responsible for the context switch (i.e.,
				 * waiting for a monitor or a message).
				 */

				LWPTRACE(tr_SCHEDULE, 2, ("swtch call\n"));
				CSWTCH();
			} else {

				/*
				 * do a full context switch, restoring temps, sr,
				 * etc., as needed.
				 */

				LWPTRACE(tr_SCHEDULE, 2, ("full swtch call\n"));
				__full_swtch();
			}
			__panic("return from swtch");
			/* NOTREACHED */
		}
	}
	UNLOCK();
	LWPTRACE(tr_SCHEDULE, 1, ("schedule exiting\n"));
	HALT();
	/* NOTREACHED */
}
