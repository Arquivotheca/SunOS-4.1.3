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
#include <lwp/queue.h>
#include <lwp/cntxt.h>
#include <lwp/lwperror.h>
#include <lwp/message.h>
#include <lwp/process.h>
#include <lwp/schedule.h>
#include <lwp/monitor.h>
#include <lwp/condvar.h>
#include <lwp/alloc.h>
#ifndef lint
SCCSID(@(#)condvar.c 1.1 92/07/30 Copyr 1986 Sun Micro);
#endif lint

/*
 * PRIMITIVES contained herein:
 * cv_create(cv, mid)
 * cv_destroy(cvdesc)
 * cv_wait(cvdesc)
 * cv_notify(cvdesc)
 * cv_broadcast(cvdesc)
 * cv_enumerate(vec, maxsize)
 * cv_waiters (cid, vec, maxsize)
 */

qheader_t __CondvarQ;	/* list of all condition variables in the system */
STATIC int CvType;	/* cookie for condition variable caches */
STATIC int CvpsType;	/* cookie for condvar info caches */

/*
 * remove a particular condition from __CondvarQ
 */
STATIC void
rem_cv(id)
	register condvar_t *id;
{
	register qheader_t *queue = &__CondvarQ;
	register __queue_t *q = queue->q_head;
	register __queue_t *prev = QNULL;

	while (q != QNULL) {
		if (((cvps_t *)q)->cvps_cvid == id)
			break;
		prev = q;
		q = q->q_next;
	}
	if (q == QNULL)	/* not found */
		return;
	if (prev == QNULL) {	/* first guy in queue */
		queue->q_head = q->q_next;
	} else {
		prev->q_next = q->q_next;
	}
	if (queue->q_tail == q)
		queue->q_tail = prev;
	FREECHUNK(CvpsType, q);
}

/*
 * cv_create() -- PRIMITIVE.
 * Create a condition variable.
 */
int
cv_create(cv, mon)
	cv_t *cv;
	mon_t mon;
{
	register condvar_t *tmp;
	register cvps_t *ps;
	register monitor_t *mid = (monitor_t *)mon.monit_id;

	LWPINIT();
	CLR_ERROR();
	LOCK();
	GETCHUNK((condvar_t *), tmp, CvType);
	INIT_QUEUE(&tmp->cv_waiting);
	GETCHUNK((cvps_t *), ps, CvpsType);
	ps->cvps_cvid = tmp;
	INS_QUEUE(&__CondvarQ, ps);
	tmp->cv_mon = mid;
	tmp->cv_lock = UNIQUEID();
	if (cv != (cv_t *)0) {
		cv->cond_id = (caddr_t)tmp;
		cv->cond_key = tmp->cv_lock;
	}
	UNLOCK();
	return (0);
}

/*
 * cv_destroy() -- PRIMITIVE.
 * Destroy a condition variable.
 * Return an error if there are still processes waiting
 * on the condition variable.
 */
int
cv_destroy(cv)
	cv_t cv;	/* condition to destroy */
{
	register condvar_t *cvdesc = (condvar_t *)cv.cond_id;
	int s;

	CLR_ERROR();
	LOCK();
	ERROR((!ISEMPTYQ(&cvdesc->cv_waiting)), LE_INUSE);
	ERROR((cvdesc->cv_lock != cv.cond_key), LE_NONEXIST);
	rem_cv(cvdesc);
	cvdesc->cv_lock = INVALIDLOCK;
	FREECHUNK(CvType, cvdesc);
	UNLOCK();
	return (0);
}

/* 
 * cv_wait() -- PRIMITIVE.
 * Wait (block) for notification by a cv_notify or cv_broadcast on the
 * specified condition variable.
 */
int
cv_wait(cv)
	cv_t cv;
{
	register condvar_t *cvdesc = (condvar_t *)cv.cond_id;
	register lwp_t *proc = __Curproc;
	register monitor_t *monid;
	register lwp_t *waiter;

	CLR_ERROR();
	LOCK();
	ERROR((cvdesc->cv_lock != cv.cond_key), LE_NONEXIST);

	monid = proc->lwp_curmon;
	ERROR((monid != cvdesc->cv_mon), LE_NOTOWNED);

	/* save the current level of monitor "reentrancy" */
	proc->lwp_monlevel = monid->mon_nestlevel;
	MON_RESTORE(proc, monid);
	BLOCK(proc, RS_CONDVAR, cvdesc);

	/*
	 * give up the monitor.
	 * priority only matters for notify, broadcast implicitly prioritizes
	 */
	PRIO_QUEUE(&cvdesc->cv_waiting, proc, proc->lwp_prio);
	if (ISEMPTYQ(&monid->mon_waiting)) {
		monid->mon_owner = LWPNULL;				
	} else { /* make first guy runnable */			
		REM_QUEUE((lwp_t *), &monid->mon_waiting, waiter);
		UNBLOCK(waiter);
		MON_SET(waiter, monid);
	}								
	/* now, wait for someone to notify us */
	WAIT();

	/*
	 * When we are notified, we will be put
	 * back on the monitor queue if we waited within
	 * a monitor and mon_exit will schedule us in.
	 * Don't come back here and attempt to reacquire monitor with
	 * mon_enter. Example: the monitor that did the notify
	 * may leave and reenter the monitor. Since this guy is not
	 * queued yet on monitor queue, he starves.
	 */
	if (IS_ERROR())
		return (-1);
	return (0);
}

/*
 * cv_notify() -- PRIMITIVE.
 * Wake up the first thread blocked on this condition variable.
 * This thread will reacquire its monitor before continuing.
 */
int
cv_notify(cv)
	cv_t cv;	/* condition to notify */
{
	register condvar_t *cvdesc = (condvar_t *)cv.cond_id;
	register lwp_t *proc;
	register qheader_t *que;
	register monitor_t *mid = cvdesc->cv_mon;

	CLR_ERROR();
	LOCK();
	ERROR((cvdesc->cv_lock != cv.cond_key), LE_NONEXIST);

	ASSERT(mid != MONNULL);	/* can't wait unless have mon */
	ERROR((mid != __Curproc->lwp_curmon), LE_NOTOWNED);

	que = &cvdesc->cv_waiting;
	if (ISEMPTYQ(que)) { /* not an error; nobody was waiting */
		UNLOCK();
		return (0);
	}
	REM_QUEUE((lwp_t *), que, proc);
	PRIO_QUEUE(&mid->mon_waiting, proc, proc->lwp_prio);
	proc->lwp_run = RS_MONITOR;
	proc->lwp_blockedon = (caddr_t )mid;
	UNLOCK();
	return (0);
}

/*
 * cv_send() -- PRIMITIVE.
 * Wake up the specified thread blocked on the condition.
 * This thread will reacquire its monitor before continuing.
 */
int
cv_send(cv, thread)
	cv_t cv;		/* condition to notify */
	thread_t thread;	/* particular thread to notify */
{
	lwp_t *tid = (lwp_t *)thread.thread_id;
	condvar_t *cvdesc = (condvar_t *)cv.cond_id;
	register lwp_t *proc;
	register monitor_t *mid = cvdesc->cv_mon;
	register qheader_t *que;
	lwp_t *prev;

	CLR_ERROR();
	LOCK();
	ERROR((cvdesc->cv_lock != cv.cond_key), LE_NONEXIST);
	ASSERT(mid != MONNULL);	/* can't wait unless have mon */
	ERROR((mid != __Curproc->lwp_curmon), LE_NOTOWNED);
	que = &cvdesc->cv_waiting;
	if (ISEMPTYQ(que)) { /* not an error; nobody was waiting */
		UNLOCK();
		return (0);
	}
	proc = (lwp_t *)que->q_head;
	prev = LWPNULL;
	while (proc != LWPNULL) {
		if (proc == tid)
			break;
		prev = proc;
		proc = proc->lwp_next;
	}
	ERROR((proc == LWPNULL), LE_NOWAIT);
	if (prev == LWPNULL) {	/* first guy in queue */
		que->q_head = (__queue_t *)(proc->lwp_next);
	} else {
		prev->lwp_next = proc->lwp_next;
	}
	if (que->q_tail == (__queue_t *)proc)
		que->q_tail = (__queue_t *)prev;
	PRIO_QUEUE(&mid->mon_waiting, proc, proc->lwp_prio);
	proc->lwp_run = RS_MONITOR;
	proc->lwp_blockedon = (caddr_t )mid;
	UNLOCK();
	return (0);
}

/*
 * cv_broadcast() -- PRIMITIVE.
 * Wake up ALL processes blocked on this condition variable.
 * Each of these processes will reacquire their monitor locks before continuing.
 */
int
cv_broadcast(cv)
	cv_t cv;	/* condition to notify */
{
	register condvar_t *cvdesc = (condvar_t *)cv.cond_id;
	register lwp_t *proc;
	register qheader_t *que;
	register monitor_t *mid = cvdesc->cv_mon;

	LOCK();
	CLR_ERROR();
	ERROR((cvdesc->cv_lock != cv.cond_key), LE_NONEXIST);

	ASSERT(mid != MONNULL);
	ERROR((mid != __Curproc->lwp_curmon), LE_NOTOWNED);

	que = &cvdesc->cv_waiting;
	while (!ISEMPTYQ(que)) {
		REM_QUEUE((lwp_t *), que, proc);
		PRIO_QUEUE(&mid->mon_waiting, proc, proc->lwp_prio);
		proc->lwp_run = RS_MONITOR;
		proc->lwp_blockedon = (caddr_t )mid;

	}
	UNLOCK();
	return (0);
}

/*
 * cv_enumerate() -- PRIMITIVE.
 * List all condition variables in the system.
 * Return number of condition variables
 * but don't fill in more than "maxsize" of them.
 */
int
cv_enumerate(vec, maxsize)
	cv_t vec[];	/* array to be filled in with cids */
	int maxsize;	/* size of vec */
{
	register cvps_t *cid;
	register int i = 0;

#define	ADDLIST(c) {								\
	if (i < maxsize) {							\
		vec[i].cond_id = (caddr_t)c->cvps_cvid;				\
		vec[i].cond_key = ((condvar_t *)(c->cvps_cvid))->cv_lock;	\
	}									\
	i++;									\
}

	LWPINIT();
	CLR_ERROR();
	LOCK();
	for (cid = (cvps_t *)FIRSTQ(&__CondvarQ); cid != CVPSNULL;
	  cid = cid->cvps_next)
		ADDLIST(cid);
	UNLOCK();
	return (i);

#undef ADDLIST
}

/*
 * remove any conditions owned by the specified monitor (which is
 * being destroyed). If any conditions are still occupied by
 * threads, return TRUE, else FALSE.
 */
bool_t
__cv_free(mid)
	register monitor_t *mid;
{
	register cvps_t *cid;
	register condvar_t *cvdesc;

	cid = (cvps_t *)FIRSTQ(&__CondvarQ);
	while (cid != CVPSNULL) {
		cvdesc = cid->cvps_cvid;
		if (cvdesc->cv_mon == mid) {
			if (!ISEMPTYQ(&cvdesc->cv_waiting)) {
				return (TRUE);
			} else {

				/* careful, rem_cv has side effects! */
				cid = cid->cvps_next;
				rem_cv(cvdesc);
				FREECHUNK(CvType, cvdesc);
			}
		}
	}
	return (FALSE);
}

/*
 * cv_waiters() -- PRIMITIVE.
 * List all processes waiting on a condition variable.
 * Return number of waiting processes
 * (but don't fill in more than maxsize of them).
 */
int
cv_waiters (cv, vec, maxsize)
	cv_t cv;	/* condition variable being inquired about */
	thread_t vec[];	/* array to be filled in with pids */
	int maxsize;	/* size of vec */
{
	condvar_t *cid = (condvar_t *)cv.cond_id;
	register __queue_t *pid;
	register int i;

#define	ADDLIST(p) {						\
	if (i < maxsize) {					\
		vec[i].thread_id = (caddr_t)p;			\
		vec[i].thread_key = ((lwp_t *)p)->lwp_lock;	\
	}							\
	i++;							\
}

	LWPINIT();
	CLR_ERROR();
	LOCK();
	ERROR((cid->cv_lock != cv.cond_key), LE_NONEXIST);
	i = 0;
	for (pid = FIRSTQ(&cid->cv_waiting); pid != QNULL; pid = pid->q_next)
		ADDLIST(pid);
	UNLOCK();
	return (i);

#undef ADDLIST
}

/* initialize condition variable code */
void
__init_cv() {

	CvType =
	  __allocinit(sizeof (condvar_t), CONDVAR_GUESS, IFUNCNULL, FALSE);
	__alloc_guess(CvType, 4);
	CvpsType = __allocinit(sizeof (cvps_t), CVPS_GUESS, IFUNCNULL, 
	   FALSE);
	__alloc_guess(CvpsType, 4);
	INIT_QUEUE(&__CondvarQ);
}
