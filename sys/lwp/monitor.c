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
SCCSID(@(#) monitor.c 1.1 92/07/30 Copyr 1987 Sun Micro);
#endif lint

extern int runthreads;

/*
 * PRIMITIVES contained herein:
 * mon_create(mid)
 *
 * In the kernel the purpose of the code is to supply a data structure so
 * that a condition variable may still be associated with a monitor. 
 */

qheader_t __MonitorQ;		/* list of all monitors in the system */
STATIC int MonpsType;		/* cookie for monitor info caches */
STATIC int MonType;		/* cookie for monitor descriptor caches */

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
	return (0);
}

void
__init_mon ()
{
};
