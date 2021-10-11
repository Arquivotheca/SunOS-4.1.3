/* Copyright (C) 1986 Sun Microsystems Inc. */

#include <lwp/common.h>
#include <lwp/queue.h>
#include <machlwp/machdep.h>
#include <lwp/lwperror.h>
#include <lwp/process.h>
#include <lwp/schedule.h>
#include <lwp/monitor.h>
#include <lwp/condvar.h>
#include <lwp/alloc.h>
#ifndef lint
SCCSID(@(#) condvar.c 1.1 92/07/30 Copyr 1987 Sun Micro);
#endif lint

/*
 * PRIMITIVES contained herein:
 * cv_create(cv, mid)
 * cv_destroy(cvdesc)
 * cv_wait(cvdesc)
 * cv_notify(cvdesc)
 * cv_broadcast(cvdesc)
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

	s = splclock();
	CLR_ERROR();
	ERROR((!ISEMPTYQ(&cvdesc->cv_waiting)), LE_INUSE);
	ERROR((cvdesc->cv_lock != cv.cond_key), LE_NONEXIST);
	rem_cv(cvdesc);
	cvdesc->cv_lock = INVALIDLOCK;
	FREECHUNK(CvType, cvdesc);
	(void) splx(s);
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
	extern int schedpri;

	CLR_ERROR();
	ERROR((cvdesc->cv_lock != cv.cond_key), LE_NONEXIST);

	(void) splclock();

	BLOCK(proc, RS_CONDVAR, cvdesc);

	/*
	 * give up the monitor.
	 * priority only matters for notify, broadcast implicitly prioritizes
	 */
	PRIO_QUEUE(&cvdesc->cv_waiting, proc, proc->lwp_prio);

	(void) splx(schedpri);

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
	int oldps;

	CLR_ERROR();
	ERROR((cvdesc->cv_lock != cv.cond_key), LE_NONEXIST);

	oldps = splclock();

	que = &cvdesc->cv_waiting;
	if (ISEMPTYQ(que)) { /* not an error; nobody was waiting */
		(void) splx(oldps);
		return (0);
	}
	REM_QUEUE((lwp_t *), que, proc);

	UNBLOCK(proc);
	(void) splx(oldps);
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
	int oldps;

	CLR_ERROR();
	ERROR((cvdesc->cv_lock != cv.cond_key), LE_NONEXIST);

	oldps = splclock();

	que = &cvdesc->cv_waiting;
	while (!ISEMPTYQ(que)) {
		REM_QUEUE((lwp_t *), que, proc);
		UNBLOCK(proc);
	}
	(void) splx(oldps);
	return (0);
}

/* initialize condition variable code */
void
__init_cv() {

	CvType =
	    __allocinit(sizeof (condvar_t), CONDVAR_GUESS, IFUNCNULL, FALSE);
	CvpsType = __allocinit(sizeof (cvps_t), CVPS_GUESS, IFUNCNULL,
	     FALSE);
	INIT_QUEUE(&__CondvarQ);
}
