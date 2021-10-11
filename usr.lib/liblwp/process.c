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
#include <lwp/lwperror.h>
#include <lwp/cntxt.h>
#include <lwp/message.h>
#include <lwp/process.h>
#include <lwp/schedule.h>
#include <lwp/alloc.h>
#include <lwp/condvar.h>
#include <lwp/monitor.h>
#include <lwp/agent.h>
#include <machlwp/except.h>
#ifndef lint
SCCSID(@(#) process.c 1.1 92/07/30 Copyr 1987 Sun Micro);
#endif lint

/*
 * PRIMITIVES contained herein:
 * lwp_self()
 * lwp_create(tid, pc, prio, flags, stack, nargs, arg1, arg2, ..., argn)
 * lwp_destroy(tid)
 * lwp_resched(prio)
 * lwp_setpri(tid, prio)
 * lwp_sleep(timeout)
 * lwp_suspend(tid)
 * lwp_resume(tid)
 * lwp_yield(tid)
 * lwp_enumerate(vec, maxsize)
 * lwp_geterr()
 * lwp_defstk()
 * pod_setexit(status)
 * pod_getexit()
 * lwp_join(tid)
 * lwp_ping(tid)
 * lwp_getstate(tid, statvec)
 */

qheader_t __SuspendQ;		/* place to put suspended processes */
qheader_t __SleepQ;		/* place to put processes sleeping on an alarm */
qheader_t __ZombieQ;		/* place for zombies (killed blocked on send) */
int __LwpRunCnt;		/* total number of lwp's in the system */
STATIC int LwpType;		/* cookie for context caches */
STATIC int LwpStatus;		/* exit status for pod */
thread_t __ThreadNull = {CHARNULL, 0};		/* the non-existent thread */
thread_t __Slaves[MAXSLAVES];	/* service threads promoted upon pod_setmaxpri */
int __SlaveThreads;		/* index of free server thread */
int __NActiveSlaves;		/* Number of existing slave threads */
int __Nrunnable;		/* Number of runnable threads */


/*
 * Automatic kill on process exit
 */

void
__lwpkill()
{
	thread_t corpse;

	/*
	 * If an interrupt causes this process to die
	 * in another way, we never return here.
	 * __Curproc will always be correct when
	 * this process is running so there is no race.
	 */
	LWPTRACE(tr_PROCESS, 1, ("process died automatically\n"));
	corpse.thread_id = (caddr_t)__Curproc;
	corpse.thread_key = __Curproc->lwp_lock;
	(void)lwp_destroy(corpse);
	__panic("return from __lwpkill");
}

/* awaken a sleeping process (called from alarm routine only) */
STATIC void
lwp_swakeup(pid)
	register lwp_t *pid;
{
	REMX_ITEM(&__SleepQ, pid);
	UNBLOCK(pid);
}

/*
 * for each thread blocked on "tid", remove from tid->lwp_joinq
 * and put back on runq. No need to yield since the caller
 * is dying and will do that for us.
 */
STATIC void
dojoin(tid)
	lwp_t *tid;
{
	register lwp_t *proc;
	register qheader_t *que = &(tid->lwp_joinq);

	while (!ISEMPTYQ(que)) {
		REM_QUEUE((lwp_t *), que, proc);
		UNBLOCK(proc);
	}
}

/*
 * This process is really dead. We can safely remove all traces of
 * it that remain: its context.
 * We also generate a "soft" agent message to inform any reaper threads
 * that they can free resources allocated to this thread.
 * The LASTRITES signal is delivered as a side effect of unlocking
 * which will either happen explicitly or as a result of scheduling.
 * LASTRITES is not handlable as a trap since a dying thread can't
 * process exceptions.
 */
void
__freeproc(lwp)
	register lwp_t *lwp;	/* completely stiff corpse */
{
	register caddr_t laststack;

	ASSERT(IS_LOCKED());
	LWPTRACE(tr_PROCESS, 2, ("__freeproc %x\n", lwp));
	lwp->lwp_run = RS_UNINIT;
	lwp->lwp_type = BADOBJECT_TYPE;
	laststack = (caddr_t) lwp->lwp_stack;

	/* free any special contexts */
	__freectxt(lwp);

	/* process joins */
	dojoin(lwp);

	if ((lwp->lwp_flags & LWPNOLASTRITES) == 0) {
		__save_event(lwp, LASTRITES, laststack, CHARNULL);
	}
	lwp->lwp_lock = INVALIDLOCK;	/* in case someone refs this mem */

	/*
	 * free the process descriptor itself. Once we do this,
	 * the lwp must not be on any queues (it will be on
	 * the free list after this).
	 * Note that it is unsafe to remove thread stack until now
	 * since there may be messages in progress to dead thread.
	 */
	FREECHUNK(LwpType, lwp);
}

/*
 * search __ZombieQ for any processes "blocked" on corpse.
 * If found, remove the zombie and free its memory and context.
 */
STATIC void
zombiecleanup(corpse)
	lwp_t *corpse;
{
	register qheader_t *queue = &__ZombieQ;
	register __queue_t *q = FIRSTQ(queue);
	register __queue_t *prev = QNULL;
	register lwp_t *tmp;

	LWPTRACE(tr_PROCESS, 2, ("Zombiecleanup %x\n", (int)corpse));
	while (q != QNULL) {
		if (corpse == (lwp_t *)(((lwp_t *)q)->lwp_blockedon)) {
			if (prev == QNULL) {
				(queue)->q_head = q->q_next;
			} else {
				prev->q_next = q->q_next;
			}
			if ((queue)->q_tail == q)
				(queue)->q_tail = prev;
			tmp = (lwp_t *)q;
			q = q->q_next;	/* prev stays the same */
			__freeproc(tmp);
		} else {
			prev = q;
			q = q->q_next;
		}
	}
}

/*
 * go through "queue" searching for anyone blocked on "corpse".
 * If a match is found, remove the entry and unblock it
 * with lwp_lwperrno set to "error".
 */
void
__rem_corpse(queue, corpse, error)
	qheader_t *queue;
	caddr_t corpse;		/* agent or thread being destroyed */
	lwp_err_t error;	/* error for thread blocked on corpse */
{
	register __queue_t *cq = FIRSTQ(queue);
	register __queue_t *prev = QNULL;
	register lwp_t *tmp;

	while (cq != QNULL) {
		if (corpse == ((lwp_t *)cq)->lwp_blockedon) {
			if (prev == QNULL) {
				(queue)->q_head = cq->q_next;
			} else {
				prev->q_next = cq->q_next;
			}
			if ((queue)->q_tail == cq)
				(queue)->q_tail = prev;
			tmp = (lwp_t *)cq;
			cq = cq->q_next;	/* prev stays the same */
			ASSERT(tmp != __Curproc);
			UNBLOCK(tmp);
			SET_ERROR(tmp, error);
		} else {
			prev = cq;
			cq = cq->q_next;
		}
	}
}

/*VARARGS*/
/*
 * lwp_create() -- PRIMITIVE.
 * create a (runnable) lightweight process and start it running if
 * it's priority warrants it.
 * It is required that the new task start at a procedure entry
 * to make it easier to keep track of how to set up the stack (see machdep.c).
 * We assume that the stack points to the top of the (already allocated)
 * stack, and that the stack is mapped in.
 */

int
lwp_create(va_alist)
	va_dcl			/* arguments to new thread */
{
	va_list argptr;		/* argument pointer for varargs package */
	thread_t *tid;		/* thread being created */
	void (*pc)();		/* initial procedure defining the new task */
	int prio;		/* scheduling priority (higher preempts lower) */
	int flags;		/* options to create thread with */
	stkalign_t *stack;	/* top of client-supplied stack */
	int nargs;		/* number of arguments */

	register lwp_t *baby;	/* the new little process we are making */
	int funarg;

	LWPINIT();
	CLR_ERROR();
	LOCK();
	va_start (argptr);
	tid = va_arg(argptr, thread_t *);
	funarg = va_arg(argptr, int); pc = (void(*)())funarg;
	prio = va_arg(argptr, int);
	flags = va_arg(argptr, int);
	stack = va_arg(argptr, stkalign_t *);
	nargs = va_arg(argptr, int);
	ERROR((nargs > MAXARGS), LE_INVALIDARG);
	ERROR(((prio > __MaxPrio) || (prio < MINPRIO)), LE_ILLPRIO);
	GETCHUNK((lwp_t *), baby, LwpType);
	ERROR((baby == LWPNULL), LE_NOROOM);
	baby->lwp_stack = (int)stack;
	stack = (stkalign_t *)STACKALIGN(stack);
	if (stack != (stkalign_t *)0) {
		MAKE_STACK(baby, argptr, stack, __lwpkill, pc, nargs);
	} else {	/* force suspension; can't run yet */
		flags |= LWPSUSPEND;
	}
	va_end (argptr);
	baby->lwp_full = FALSE;
	INIT_QUEUE(&baby->lwp_ctx);
	baby->lwp_type = PROCESS_TYPE;
	__LwpRunCnt++;
	LWPTRACE(tr_PROCESS, 2, ("numprocesses %d\n", __LwpRunCnt));
	INIT_QUEUE(&baby->lwp_messages);
	INIT_QUEUE(&baby->lwp_exchandles);
	INIT_QUEUE(&baby->lwp_joinq);
	baby->lwp_blockedon = NOBODY;
	baby->lwp_curmon = MONNULL;
	baby->lwp_suspended = FALSE;
	baby->lwp_prio = prio;
	baby->lwp_run = RS_UNINIT;
	baby->lwp_lwperrno = LE_NOERROR;
	baby->lwp_monlevel = 0;
	baby->lwp_flags = flags;
	baby->lwp_lock = UNIQUEID();
	baby->lwp_cancel = 0;
	if (tid != (thread_t *)0) {
		tid->thread_id = (caddr_t)baby;
		tid->thread_key = baby->lwp_lock;
	}
	if (flags & LWPSERVER) {
		prio = baby->lwp_prio = __MaxPrio;
		__Slaves[__SlaveThreads].thread_id = (caddr_t)baby;
		__Slaves[__SlaveThreads].thread_key = baby->lwp_lock;
		__SlaveThreads++;
		__NActiveSlaves++;
	}
	if (flags & LWPSUSPEND) {
		INS_QUEUE(&__SuspendQ, baby);
		baby->lwp_run = RS_SUSPEND;
		baby->lwp_suspended = TRUE;
		UNLOCK();
	} else {
		UNBLOCK(baby);
		YIELD_CPU(prio);
	}
	return (0);
}

/*
 * Create a context for main. Runs at maximum priority so, e.g., idle
 * will not run until main blocks or otherwise yields control.
 * All we do is create a context for main (which somehow called pod_create).
 */
void
__maincreate()
{
	register lwp_t *baby;	/* the new little process we are making */

	CLR_ERROR();
	GETCHUNK((lwp_t *), baby, LwpType);
	baby->lwp_full = FALSE;
	baby->lwp_type = PROCESS_TYPE;
	__LwpRunCnt++;
	INIT_QUEUE(&baby->lwp_messages);
	INIT_QUEUE(&baby->lwp_exchandles);
	INIT_QUEUE(&baby->lwp_ctx);
	INIT_QUEUE(&baby->lwp_joinq);
	baby->lwp_blockedon = NOBODY;
	baby->lwp_curmon = MONNULL;
	baby->lwp_suspended = FALSE;
	baby->lwp_prio = __MaxPrio;
	baby->lwp_run = RS_UNINIT;
	baby->lwp_lwperrno = LE_NOERROR;
	baby->lwp_monlevel = 0;
	baby->lwp_stack = 0;
	baby->lwp_flags = 0;
	baby->lwp_lock = UNIQUEID();
	__Curproc = baby;
	UNBLOCK(baby);
}

/*
 * lwp_self -- PRIMITIVE.
 * Set identity of current thread.
 */
int
lwp_self(tid)
	thread_t *tid;	/* will hold identity of current thread */
{
	LWPINIT();
	CLR_ERROR();
	tid->thread_id = (caddr_t)__Curproc;
	tid->thread_key = __Curproc->lwp_lock;
	return (0);
}

/*
 * lwp_enumerate() -- PRIMITIVE.
 * A process belongs to exactly one queue at a time:
 * List all processes by searching __RunQ, __SuspendQ, __MonitorQ, __CondvarQ,
 * __SendQ, __ReceiveQ, __SleepQ. We don't care if this is fairly expensive.
 * Return total number of processes.
 * MIN(maxsize, lwp_enumerate()) pids will be filled in.
 */
int
lwp_enumerate(vec, maxsize)
	thread_t vec[];	/* array to be filled in with pids */
	int maxsize;	/* size of vec */
{
	register lwp_t *pid;
	register int i = 0;
	register int k;
	register monps_t *monps;
	register cvps_t *cvps;
	lwp_t *jid;

#define	ADDLIST(p) {					\
	if (i < maxsize) {				\
		vec[i].thread_id = (caddr_t)p;		\
		vec[i].thread_key = p->lwp_lock;	\
	}						\
	i++;						\
}

	LWPINIT();
	CLR_ERROR();
	LOCK();
	for (pid = (lwp_t *)FIRSTQ(&__SuspendQ);
	  pid != LWPNULL; pid = pid->lwp_next)
		ADDLIST(pid);
	for (pid = (lwp_t *)FIRSTQ(&__SleepQ);
	  pid != LWPNULL; pid = pid->lwp_next)
		ADDLIST(pid);
	for (pid = (lwp_t *)FIRSTQ(&__SendQ);
	  pid != LWPNULL; pid = pid->lwp_next)
		ADDLIST(pid);
	for (pid = (lwp_t *)FIRSTQ(&__ReceiveQ);
	  pid != LWPNULL; pid = pid->lwp_next)
		ADDLIST(pid);
	for (monps = (monps_t *)FIRSTQ(&__MonitorQ); monps != MONPSNULL;
	    monps = monps->monps_next) {
		for (pid = (lwp_t *)FIRSTQ(&monps->monps_monid->mon_waiting);
		    pid != LWPNULL; pid = pid->lwp_next) {
			ADDLIST(pid);
		}
	}
	for (cvps = (cvps_t *)FIRSTQ(&__CondvarQ); cvps != CVPSNULL;
	    cvps = cvps->cvps_next) {
		for (pid = (lwp_t *)FIRSTQ(&cvps->cvps_cvid->cv_waiting);
		    pid != LWPNULL; pid = pid->lwp_next) {
			ADDLIST(pid);
		}
	}
	for (k = NUGPRIO + 1; k <= __MaxPrio; k++) {
		for (pid = (lwp_t *)FIRSTQ(&__RunQ[k]);
		  pid != LWPNULL; pid = pid->lwp_next) {
			ADDLIST(pid);
			for (jid = (lwp_t *)FIRSTQ(&(((lwp_t *)pid)->lwp_joinq));
			  jid != LWPNULL; jid = jid->lwp_next) {
				ADDLIST(jid);
			}
		}
	}
	ASSERT (i == (__LwpRunCnt - 1));	/* skip idle process */
	UNLOCK();
	return (i);

#undef ADDLIST
}

/*
 * lwp_destroy -- PRIMITIVE.
 * destroy a lwp or agent and free all resources it owns (such as monitor locks).
 */
int
lwp_destroy(body)
	thread_t body;
{
	register lwp_t *corpse = (lwp_t *)(body.thread_id);
	register int prio;
	register int i;
	CLR_ERROR();
	if (corpse == LWPNULL) {	/* self-destruction */
		body.thread_id =  (caddr_t)__Curproc;
		corpse = __Curproc;
		body.thread_key = corpse->lwp_lock;
	}
	if (TYPE(corpse) == AGENT_TYPE) {
		return (__agt_destroy((agent_t *)corpse, body.thread_key));
	}
	LOCK();
	ERROR((body.thread_key != corpse->lwp_lock), LE_NONEXIST);
	prio = corpse->lwp_prio;

	/* cancel any timers that may have been set */
	if (corpse->lwp_run == RS_SLEEP)
		(void) __cancelalarm(lwp_swakeup, (int)corpse);
	else if (corpse->lwp_run == RS_RECEIVE) /* possible no-op */
		(void) __cancelalarm(__lwp_rwakeup, (int)corpse);

	/*
	 * Remove this process from whatever queue it belonged to.
	 * Must dequeue BEFORE freeing the context itself.
	 * In effect, we make the process look like it's running from
	 * the point of view of the cleanup routines as the process
	 * will not be on any blocked queue. (It won't be on the __RunQ
	 * either though).
	 */
	switch (corpse->lwp_run) {
	  case RS_SEND:
		REMX_ITEM(&__SendQ, corpse);
		break;
	  case RS_RECEIVE:
		REMX_ITEM(&__ReceiveQ, corpse);
		break;
	  case RS_MONITOR:
		REMX_ITEM(&(((monitor_t *)(corpse->lwp_blockedon))->mon_waiting),
		  corpse);
		break;
	  case RS_CONDVAR:
		REMX_ITEM(&(((condvar_t *)(corpse->lwp_blockedon))->cv_waiting),
		  corpse);
		break;
	  case RS_SUSPEND:
		REMX_ITEM(&__SuspendQ, corpse);
		break;
	  case RS_SLEEP:
		REMX_ITEM(&__SleepQ, corpse);
		break;
	  case RS_UNBLOCKED:
		REMX_ITEM(&__RunQ[prio], corpse);
		__Nrunnable--;
		break;
	  case RS_ZOMBIE:
		TERROR(LE_NONEXIST);
	  case RS_JOIN:
		REMX_ITEM(&(((lwp_t *)(corpse->lwp_blockedon))->lwp_joinq),
		  corpse);
		break;
	  default:
		__panic("bad destroy");
	}
	__LwpRunCnt--;
	LWPTRACE(tr_PROCESS, 3, ("Remove process, %d left\n", __LwpRunCnt));

	/* remove monitor locks (if any) held by this process */
	__moncleanup(corpse);

	/*
	 * Any process waiting for a message from corpse or
	 * sending to corpse (unreplied to as yet) will
	 * be deadlocked. __msgcleanup() aborts any such deadlock
	 * by scanning __ReceiveQ and __SendQ for processes blocked on corpse
	 * and unblocking them. Note that condition variables have
	 * looser semantics and we can't know how to unblock
	 * someone waiting on a given cv based on process death.
	 */
	__msgcleanup(corpse);

	/* remove any zombies "blocked" on corpse */
	zombiecleanup(corpse);

	/*
	 * Remove any agents owned by this process. This may entail
	 * some additional garbage collecting.
	 * A process may only own agents and messages. Condition variables
	 * and monitors have no ownership associated with them
	 * and are only cleaned up on explicit destroy.
	 */
	__agentcleanup(corpse);

	/* remove any freed exception handlers */
	__exceptcleanup(corpse);

	/*
	 * Before calling freeproc, be sure that I'm not depending on
	 * the client stack for anything.
	 * freeproc generates the thread termination agent message
	 * that could result in stack cleanup.
	 * At this point, we're sure that the client will die.
	 * Since we are locked, nobody will try to save registers
	 * on behalf of a non-valid __Curproc.
	 */
	if (corpse == __Curproc) {
		SETSTACK();
	}

	/*
	 * we don't want to free any of the process' memory
	 * involved with a send in progress (which is being killed now)
	 * until the reply has been made (the tag and what it points to
	 * can still be referenced).
	 * In the kernel, we should be able to keep
	 * better track of the processes' memory (via ref. counts on shared mem)
	 * including stuff the process itself malloc'ed).
	 * Here, we take the precaution that the stack is not freed
	 * (via LASTRITES agent in stack.c)
	 * until a reply is sent. This prevents the replier from
	 * possibly clobbering memory that would be reallocated later.
	 * (scenario: sender sends message, replier receives message,
	 * sender is killed, a new process is allocated, replier
	 * fills a buffer allocated to that new process (intending to
	 * fill the original reply buffer). The problem is that once
	 * a message is received, there is no validity check on the
	 * memory which the send has given to the receiver.
	 */
	if (corpse->lwp_run != RS_SEND) {
		__freeproc(corpse);
	} else {
		corpse->lwp_run = RS_ZOMBIE;
		INS_QUEUE(&__ZombieQ, corpse);
		LWPTRACE(tr_PROCESS, 2, ("Zombie %x\n", corpse));
	}

	/* if this is a slave, remove it from list of slaves */
	for (i = 0; i < __SlaveThreads; i++) {
		if (SAMETHREAD(body, __Slaves[i])) {
			__Slaves[i] = THREADNULL;
			__NActiveSlaves--;
			break;
		}
	}
	if (corpse == __Curproc) {
		__Curproc = &Nullproc; /* NB: if inter. now, this is the victim */
		__schedule();
		/* NOTREACHED */
	} else {	/* removing somebody other than caller */
		LWPTRACE(tr_PROCESS, 4,
		  ("removing process in middle of run queue\n"));

		/*
		 * a different guy can be made runnable by a lot of ways:
		 * replies to zombie sends, waking up receives, agents going away
		 * (unblocking a specific send or receive), monitors unlocked.
		 * Easier to just let the scheduler figure out who runs next.
		 */
		WAIT();
	}
	return (0);
}

/*
 * lwp_resched -- PRIMITIVE.
 * alter the scheduling priority of the "front" process of the
 * specified priority (less than the current priority).
 * Note that this primitive does not affect queues other than the run queue.
 * On an MP, it may be conceivable that a
 * lower prio thread reschedules a higher one.
 */
int
lwp_resched(prio)
	register int prio;
{
	register qheader_t *q;

	LWPINIT();
	CLR_ERROR();
	if (prio == __Curproc->lwp_prio) {
		return (lwp_yield(THREADNULL));
	}
	LOCK();
	ERROR((prio > __Curproc->lwp_prio), LE_INVALIDARG);
	q = &__RunQ[prio];
	ERROR((ISEMPTYQ(q)), LE_ILLPRIO);
	BACKX_QUEUE(q);
	UNLOCK();
	return (0);
}

/*
 * lwp_setpri -- PRIMITIVE.
 * Alter (raise or lower) the scheduling priority of the specified
 * process.
 */
int
lwp_setpri(tid, newprio)
	thread_t tid;		/* thread having prio changed */
	register int newprio;	/* new scheduling priority */
{
	register int oldprio;
	register lwp_t *proc = (lwp_t *)(tid.thread_id);

	CLR_ERROR();
	if (proc == LWPNULL) {	/* self-setpri */
		tid.thread_id =  (caddr_t)__Curproc;
		proc = __Curproc;
		tid.thread_key = proc->lwp_lock;
	}
	LOCK();
	ERROR(((newprio > __MaxPrio) || (newprio < MINPRIO)), LE_INVALIDARG);
	ERROR((tid.thread_key != proc->lwp_lock), LE_NONEXIST);
	oldprio = proc->lwp_prio;
	if (oldprio == newprio) {
		UNLOCK();
		return (oldprio);
	}
	switch (proc->lwp_run) {
	  case RS_MONITOR:
		proc->lwp_prio = newprio;
		REMX_ITEM(&(((monitor_t *)(proc->lwp_blockedon))->mon_waiting),
		  proc);
		PRIO_QUEUE(&(((monitor_t *)(proc->lwp_blockedon))->mon_waiting),
		  proc, newprio);
		UNLOCK();
		break;
	  case RS_CONDVAR:
		proc->lwp_prio = newprio;
		REMX_ITEM(&(((condvar_t *)(proc->lwp_blockedon))->cv_waiting),
		  proc);
		PRIO_QUEUE(&(((condvar_t *)(proc->lwp_blockedon))->cv_waiting),
		  proc, newprio);
		UNLOCK();
		break;
	  case RS_UNBLOCKED:
		BLOCK(proc, RS_UNINIT, NOBODY);
		proc->lwp_prio = newprio;
		UNBLOCK(proc);
		if ((proc == __Curproc) && (newprio < oldprio)) {
			WAIT();
		} else {
			YIELD_CPU(newprio);
		}
		break;
	  case RS_ZOMBIE:
		TERROR(LE_NONEXIST);
	  default:
		UNLOCK();
		break;
	}
	return (oldprio);
}

/*
 * lwp_yield -- PRIMITIVE.
 * yield cpu to another lwp at the same priority.
 * The yielder stays on the run queue.
 * You can't yield to a higher prio since otherwise it would be already running.
 * You can't yield to a lower prio since you would still be running.
 * Thus, the priority must be the same.
 */
int
lwp_yield(tid)
	thread_t tid; 	/* thread to run next (THREADNULL if don't care) */
{
	register int prio = __Curproc->lwp_prio;
	register lwp_t *newproc = (lwp_t *)(tid.thread_id);
	register qheader_t *rq = &__RunQ[prio];
	register lwp_t *proc;

	/*
	 * No LWPINIT() here for speed and also nobody should be using
	 * yield unless something else was done first.
	 */
	CLR_ERROR();
	LOCK();
	LWPTRACE(tr_PROCESS, 2, ("About to yield %d %x\n", prio, (int)newproc));
	if ((newproc == LWPNULL) || (newproc == __Curproc)) {
		/*
		 * yield to anybody at same prio (including self):
		 * put current guy at end of his queue to let in next guy
		 */
		BACKX_QUEUE(rq);
	} else {	/* yield to specific guy */
		ERROR((tid.thread_key != newproc->lwp_lock), LE_NONEXIST);
		ERROR((newproc->lwp_run != RS_UNBLOCKED), LE_INVALIDARG);
		ERROR((newproc->lwp_prio != prio), LE_ILLPRIO);
		REM_ITEM((lwp_t *), rq, newproc, proc);
		ERROR((proc == LWPNULL), LE_NONEXIST);
		ASSERT(newproc == proc);
		FRONT_QUEUE(rq, newproc);
	}
	WAIT();
	return (0);
}

/*
 * lwp_sleep -- PRIMITIVE.
 * put the current process to sleep as specified in "timeout".
 */
int
lwp_sleep(timeout)
	struct timeval *timeout;	/* how long to sleep */
{
	int err;

	if (timeout == POLL) {
		return (0);
	}
	LWPINIT();
	CLR_ERROR();
	LOCK();
	if (timeout != INFINITY) {
		err = __setalarm(timeout, lwp_swakeup, (int)__Curproc);
		ERROR((err == -1), LE_INVALIDARG);
	}
	BLOCK(__Curproc, RS_SLEEP, NOBODY);
	INS_QUEUE(&__SleepQ, __Curproc);
	WAIT();
	return (0);
}

/*
 * lwp_suspend -- PRIMITIVE.
 * remove a lwp from runnable status.
 * If it was blocked, set a flag such that unblocking it
 * will cause it to be put in __SuspendQ instead of __RunQ (but don't alter
 * lwp_blockedon).
 */
int
lwp_suspend(tid)
	thread_t tid;
{
	register lwp_t *proc = (lwp_t *)(tid.thread_id);

	CLR_ERROR();
	if (proc == LWPNULL) {	/* self-suspend */
		tid.thread_id =  (caddr_t)__Curproc;
		proc = __Curproc;
		tid.thread_key = proc->lwp_lock;
	}
	LOCK();
	ERROR((tid.thread_key != proc->lwp_lock), LE_NONEXIST);
	if (proc->lwp_run == RS_UNBLOCKED) {
		BLOCK(proc, RS_SUSPEND, NOBODY);
		INS_QUEUE(&__SuspendQ, proc);
	}	/* else, it's already off the run queue onto some other queue */
	proc->lwp_suspended = TRUE;	/* don't let it run until resumed */
	if (proc == __Curproc) { /* on __SuspendQ, __Curproc is RS_UNBLOCKED */
		WAIT();
	} else {
		UNLOCK();
	}
	return (0);
}

/*
 * lwp_resume -- PRIMITIVE.
 * make process eligible to run from suspended state and
 * run it if its priority warrants it.
 */
int
lwp_resume(tid)
	thread_t tid;
{
	register lwp_t *proc = (lwp_t *)(tid.thread_id);

	CLR_ERROR();
	LOCK();
	ERROR((tid.thread_key != proc->lwp_lock), LE_NONEXIST);
	proc->lwp_suspended = FALSE;
	if (proc->lwp_run == RS_SUSPEND) { /* not waiting for a message, etc. */
		REMX_ITEM(&__SuspendQ, proc);
		UNBLOCK(proc);
		YIELD_CPU(proc->lwp_prio);
	} else {	/* just remove the need to suspend */
		UNLOCK();
	}
	return (0);
}

/*
 * lwp_geterr() -- PRIMITIVE.
 * return the error number associated with the current lwp.
 */
lwp_err_t
lwp_geterr()
{
	return (__Curproc->lwp_lwperrno);
}

/*
 * pod_setexit() -- PRIMITIVE.
 * set the exit status to be reported when the pod terminates
 */
void
pod_setexit(status)
	int status;	/* exit status for pod */
{
	LwpStatus = status;
}

/*
 * pod_getexit() -- PRIMITIVE.
 * return the current exit status
 */
int
pod_getexit()
{
	return (LwpStatus);
}

/*
 * lwp_join -- PRIMITIVE.
 * wait for the specified thread to die.
 */
int
lwp_join(tid)
	thread_t tid;	/* thread we will wait on */
{
	register lwp_t *proc = (lwp_t *)(tid.thread_id);

	CLR_ERROR();
	LOCK();
	ERROR((tid.thread_key != proc->lwp_lock), LE_NONEXIST);
	BLOCK(__Curproc, RS_JOIN, proc);
	INS_QUEUE(&proc->lwp_joinq, __Curproc);
	WAIT();
	return (0);
}

/*
 * lwp_ping() -- PRIMITIVE.
 * Return 0 (no error) if the specified thread exists, else -1.
 */
int
lwp_ping(tid)
	thread_t tid;
{
	register lwp_t *pid = (lwp_t *)(tid.thread_id);

	CLR_ERROR();
	LOCK();
	ERROR((pid->lwp_lock != tid.thread_key), LE_NONEXIST);
	UNLOCK();
	return (0);
}

/*
 * lwp_getstate -- PRIMITIVE.
 * return useful context of a specific lwp.
 */
int
lwp_getstate(tid, statvec)
	thread_t tid;
	register statvec_t *statvec;
{
	register lwp_t *pid = (lwp_t *)(tid.thread_id);

	CLR_ERROR();
	LOCK();
	ERROR((pid->lwp_lock != tid.thread_key), LE_NONEXIST);
	statvec->stat_prio = pid->lwp_prio;
	switch (pid->lwp_run) {
	  case RS_CONDVAR:
		statvec->stat_blocked.any_kind = CV_TYPE;
		statvec->c_id = pid->lwp_blockedon;
		statvec->c_key = pid->lwp_lock;
		break;
	  case RS_MONITOR:
		statvec->stat_blocked.any_kind = MON_TYPE;
		statvec->m_id = pid->lwp_blockedon;
		statvec->m_key = pid->lwp_lock;
		break;
	  case RS_SEND:
	  case RS_RECEIVE:
		statvec->stat_blocked.any_kind = LWP_TYPE;
		statvec->l_id = pid->lwp_blockedon;
		statvec->l_key = ((lwp_t *)(pid->lwp_blockedon))->lwp_lock;
		break;
	  default:
		statvec->stat_blocked.any_kind = NO_TYPE;
	}
	UNLOCK();
	return (0);
}

/*
 * initialize context caches and process info
 */
void
__init_lwp()
{
	register int i;

	__SlaveThreads = 0;
	__NActiveSlaves = 0;
	__Nrunnable = 0;
	LwpStatus = 0;		/* default: normal exit status */
	LwpType = __allocinit(sizeof (lwp_t), MAXLWPS, IFUNCNULL, FALSE);
	__alloc_guess(LwpType, 4);
	for (i = 0; i <= __MaxPrio; i++)
		INIT_QUEUE(&__RunQ[i]);
	INIT_QUEUE(&__SleepQ);
	INIT_QUEUE(&__SuspendQ);
	INIT_QUEUE(&__ZombieQ);
	__LwpRunCnt = 0;
}
