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
#include <lwp/message.h>
#include <lwp/process.h>
#include <lwp/schedule.h>
#include <lwp/asynch.h>
#include <lwp/monitor.h>
#include <lwp/condvar.h>
#include <lwp/alloc.h>
#ifndef lint
SCCSID(@(#)monitor.c 1.1 92/07/30 Copyr 1986 Sun Micro);
#endif lint

/*
 * PRIMITIVES contained herein:
 * mon_create(mid)
 * mon_destroy(mondesc)
 * mon_enter(mid)
 * mon_exit(monid)
 * mon_enumerate(vec, maxsize)
 * mon_waiters(mid, owner, vec, maxsize)
 * mon_cond_enter(mid)
 * mon_break(monid)
 */

qheader_t __MonitorQ;		/* list of all monitors in the system */
STATIC int MonpsType;		/* cookie for monitor info caches */
STATIC int MonType;		/* cookie for monitor descriptor caches */

/*
 * remove a particular monitor from __MonitorQ
 */
STATIC void
rem_mon(id)
	register monitor_t *id;
{
	register qheader_t *queue = &__MonitorQ;
	register __queue_t *q = queue->q_head;
	register __queue_t *prev = QNULL;

	while (q != QNULL) {
		if (((monps_t *)q)->monps_monid == id)
			break;
		prev = q;
		q = q->q_next;
	}
	if (q == QNULL)
		return;
	if (prev == QNULL) {
		queue->q_head = q->q_next;
	} else {
		prev->q_next = q->q_next;
	}
	if (queue->q_tail == q)
		queue->q_tail = prev;
	FREECHUNK(MonpsType, q);
}

/*
 * mon_create() -- PRIMITIVE.
 * Create a monitor and return handle for it.
 */
int
mon_create(monid)
	mon_t *monid;
{
	register monitor_t *tmp;
	register monps_t *ps;

	LWPINIT();
	CLR_ERROR();
	LOCK();
	GETCHUNK((monitor_t *), tmp, MonType);
	tmp->mon_owner = LWPNULL;
	tmp->mon_set = MONNULL;
	tmp->mon_nestlevel = 0;
	INIT_QUEUE(&tmp->mon_waiting);
	GETCHUNK((monps_t *), ps, MonpsType);
	ps->monps_monid = tmp;
	INS_QUEUE(&__MonitorQ, ps);
	tmp->mon_lock = UNIQUEID();
	if (monid != (mon_t *)0) {
		monid->monit_id = (caddr_t)tmp;
		monid->monit_key = tmp->mon_lock;
	}
	UNLOCK();
	return (0);
}

/*
 * mon_destroy() -- PRIMITIVE.
 * Destroy a monitor and any conditions
 * bound to it. If any threads are blocked on this monitor or
 * associated conditions, don't destroy and return an error.
 * Note that some conditions might get freed before the error is detected.
 */
int
mon_destroy(mon)
	mon_t mon;
{
	register monitor_t *mid = (monitor_t *)mon.monit_id;

	CLR_ERROR();
	LOCK();
	ERROR((mid->mon_lock != mon.monit_key), LE_NONEXIST);
	ERROR((!ISEMPTYQ(&mid->mon_waiting) || __cv_free(mid)), LE_INUSE);
	rem_mon(mid);	/* take out of __MonitorQ */
	mid->mon_lock = INVALIDLOCK;
	FREECHUNK(MonType, mid);
	UNLOCK();
	return (0);
}

/*
 * mon_enter() -- PRIMITIVE.
 * Enter a monitor. If it is not busy, just record the owner and
 * let him run. Otherwise, block this process by its priority on
 * the monitor queue.
 */
int
mon_enter(mon)
	mon_t mon;
{
	register monitor_t *mid = (monitor_t *)mon.monit_id;
	register lwp_t *proc = __Curproc;

	CLR_ERROR();
	LOCK();
	ERROR((mid->mon_lock != mon.monit_key), LE_NONEXIST);
	if (mid->mon_owner == proc) { /* owner reenters monitor */
		mid->mon_nestlevel++;
		UNLOCK();
		return (mid->mon_nestlevel);
	}

	/*
	 * We maintain, in the process context, the identity of the
	 * currently-held monitor. In the monitor, we point to the
	 * next-most-recently owned monitor (for nested calls).
	 * Note that the earlier monitor locks are held by their owner.
	 */
	if (mid->mon_owner == LWPNULL) {	/* monitor free */
		MON_SET(proc, mid);
		UNLOCK();
	} else {	/* monitor busy */
		BLOCK(proc, RS_MONITOR, mid);
		PRIO_QUEUE(&mid->mon_waiting, proc, (proc->lwp_prio));
		WAIT();
		if (IS_ERROR())
			return (-1);
	}
	return (0);
}

/*
 * mon_exit() -- PRIMITIVE.
 * Leave a monitor and let next guy (if any) in.
 */
int
mon_exit(mon)
	mon_t mon;
{
	register monitor_t *monid = (monitor_t *)mon.monit_id;
	register lwp_t *current = __Curproc;
	register lwp_t *waiter;

	CLR_ERROR();
	LOCK();
	ERROR((current->lwp_curmon != monid), LE_INVALIDARG);
	ERROR((monid->mon_lock != mon.monit_key), LE_NONEXIST);
	ASSERT(monid->mon_owner == __Curproc);
	if (monid->mon_nestlevel > 0) {
		monid->mon_nestlevel--;
		UNLOCK();
		return (monid->mon_nestlevel + 1);
	}
	MON_RESTORE(current, monid);

	/* release monitor and start next process running if present */
	if (ISEMPTYQ(&monid->mon_waiting)) {
		monid->mon_owner = LWPNULL;
		UNLOCK();
	} else {	/* make first guy runnable */
		REM_QUEUE((lwp_t *), &monid->mon_waiting, waiter);
		UNBLOCK(waiter);
		MON_SET(waiter, monid);
		YIELD_CPU(waiter->lwp_prio);
	}
	return (0);
}

/*
 * assist for monitor macro (needed because exc_on_exit only passes
 * simple types as the arg).
 */
void
mon_exit1(mon)
	mon_t *mon;
{
	(void) mon_exit(*mon);
}

/*
 * mon_enumerate() -- PRIMITIVE.
 * List all monitors in the system.
 * Return number of monitors but don't fill in more than "maxsize" of them.
 */
int
mon_enumerate(vec, maxsize)
	mon_t vec[];	/* array to be filled in with mids */
	int maxsize;	/* size of vec */
{
	register monps_t *mid;
	register int i = 0;

#define	ADDLIST(m) {								\
	if (i < maxsize) {							\
		vec[i].monit_id = (caddr_t)m->monps_monid;			\
		vec[i].monit_key = ((monitor_t *)(m->monps_monid))->mon_lock;	\
	}									\
	i++;									\
}

	CLR_ERROR();
	LOCK();
	for (mid = (monps_t *)FIRSTQ(&__MonitorQ); mid != MONPSNULL;
	  mid = mid->monps_next)
		ADDLIST(mid);
	UNLOCK();
	return (i);

#undef ADDLIST
}

/*
 * mon_waiters() -- PRIMITIVE.
 * List owner and and all processes waiting on a monitor.
 * Return number of waiters (but don't fill in more than maxsize of them).
 */
int
mon_waiters(mon, owner, vec, maxsize)
	mon_t mon;		/* monitor being inquired about */
	thread_t *owner;	/* process that owns this monitor */
	thread_t vec[];		/* array to be filled in with pids */
	int maxsize;		/* size of vec */
{
	monitor_t *mid = (monitor_t *)mon.monit_id;
	register __queue_t *pid;
	register int i;

#define	ADDLIST(p) {						\
	if (i < maxsize) {					\
		vec[i].thread_id = (caddr_t)p;			\
		vec[i].thread_key = ((lwp_t *)p)->lwp_lock;	\
	}							\
	i++;							\
}

	CLR_ERROR();
	LOCK();
	ERROR((mid->mon_lock != mon.monit_key), LE_NONEXIST);
	i = 0;
	for (pid = FIRSTQ(&mid->mon_waiting); pid != QNULL; pid = pid->q_next)
		ADDLIST(pid);
	owner->thread_id = (caddr_t)(mid->mon_owner);
        if (owner->thread_key != (int)LWPNULL)
                owner->thread_key = ((lwp_t *)(mid->mon_owner))->lwp_lock;
	UNLOCK();
	return (i);

#undef ADDLIST
}

/*
 * mon_cond_enter() -- PRIMITIVE.
 * Conditionally enter a monitor if not busy. Return an error
 * if it is.
 */
int
mon_cond_enter(mon)
	mon_t mon;
{
	register monitor_t *mid = (monitor_t *)mon.monit_id;
	register lwp_t *proc;

	CLR_ERROR();
	LOCK();
	ERROR((mid->mon_lock != mon.monit_key), LE_NONEXIST);
	proc = __Curproc;
	if (mid->mon_owner == proc) { /* owner reenters monitor */
		mid->mon_nestlevel++;
		UNLOCK();
		return (mid->mon_nestlevel);
	}
	if (mid->mon_owner == LWPNULL) {	/* monitor free */
		MON_SET(proc, mid);
		UNLOCK();
		return (0);
	} else {				/* monitor busy */
		TERROR(LE_INUSE);
	}
}

/*
 * mon_break() -- PRIMITIVE.
 * release the monitor "monid" to the next process (if any).
 */
int
mon_break(mon)
	mon_t mon;
{
	register monitor_t *monid = (monitor_t *)mon.monit_id;
	register lwp_t *waiter;
	register lwp_t *owner;

	CLR_ERROR();
	LOCK();
	ERROR((monid->mon_lock != mon.monit_key), LE_NONEXIST);
	owner = monid->mon_owner;
	ERROR((owner == LWPNULL), LE_NOTOWNED);
	MON_RESTORE(owner, monid);

	/* release monitor and start next process running if present */
	if (ISEMPTYQ(&monid->mon_waiting)) {
		monid->mon_owner = LWPNULL;
		UNLOCK();
	} else {	/* make first guy runnable */
		REM_QUEUE((lwp_t *), &monid->mon_waiting, waiter);
		UNBLOCK(waiter);
		MON_SET(waiter, monid);
		YIELD_CPU(waiter->lwp_prio);
	}
	return (0);
}

/*
 * Release all monitor locks held by "corpse".
 * "corpse" iteslf has already been removed from any monitor queue
 * that it may have been blocked on. Thus, curmon points to the
 * top of the monitor stack for "corpse" and "corpse" is effectively
 * running (that is, the nugget is running corpse on its behalf).
 * Note that if "corpse" is forced to release monitors, invariants
 * may no longer hold.
 * It may be true that all present and future monitor invariants are
 * now wrong. At present, we just break the lock. It may be desirable
 * in the future to keep state about the error in the monitor to
 * indicate to all future calls that the monitor is possibly invalid.
 * Note that schedule() will be called after moncleanup returns
 * so there is no need to yield.
 */
void
__moncleanup(corpse)
	register lwp_t *corpse;
{
	register lwp_t *waiter;
	register monitor_t *monid;

	for (monid = corpse->lwp_curmon; monid != MONNULL;
	  monid = corpse->lwp_curmon) {

		/* MON_RESTORE resets corpse->lwp_curmon */
		MON_RESTORE(corpse, monid);

		/* release monitor */
		if (ISEMPTYQ(&monid->mon_waiting)) {
			monid->mon_owner = LWPNULL;
		} else {	/* make first guy runnable */
			REM_QUEUE((lwp_t *), &monid->mon_waiting, waiter);
			UNBLOCK(waiter);
			MON_SET(waiter, monid);
		}
	}
}

/* initialize monitor code */
void
__init_mon() 
{

	MonType = __allocinit(sizeof (monitor_t), MON_GUESS, IFUNCNULL, FALSE);
	MonpsType =
	  __allocinit(sizeof (monps_t), MONPS_GUESS, IFUNCNULL, FALSE);
	INIT_QUEUE(&__MonitorQ);
}
