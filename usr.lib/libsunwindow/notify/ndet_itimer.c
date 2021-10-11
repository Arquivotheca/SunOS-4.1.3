#ifndef	lint
#ifdef sccs
static	char sccsid[] = "@(#)ndet_itimer.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Ndet_itimer.c - Implement itimer specific calls that are shared among
 * NTFY_REAL_ITIMER and NTFY_VIRTUAL_ITIMER.
 */

#include <sunwindow/ntfy.h>
#include <sunwindow/ndet.h>
#include <sunwindow/ndis.h>	/* For ndis_enqueue */
#include <sunwindow/nint.h>	/* For nint_get_func */

extern	struct	itimerval NOTIFY_NO_ITIMER;
extern	struct	itimerval NOTIFY_POLLING_ITIMER;

/*
 * General utilities:
 */

pkg_private int
ndet_check_which(which, type_ptr)
	int which;
	NTFY_TYPE *type_ptr;
{
	NTFY_TYPE type;

	switch (which) {
	case ITIMER_REAL: type = NTFY_REAL_ITIMER; break;
	case ITIMER_VIRTUAL: type = NTFY_VIRTUAL_ITIMER; break;
	default: ntfy_set_errno(NOTIFY_BAD_ITIMER); return(-1);
	}
	if (type_ptr)
		*type_ptr = type;
	return(0);
}

/* Stolen (in most part) from /sys/sys/kern_time.c */
pkg_private int
ndet_check_tv(tv)
	struct timeval *tv;
{
	if (tv->tv_sec < 0 || tv->tv_sec > 100000000 ||
	    tv->tv_usec < 0 || tv->tv_usec >= 1000000) {
		ntfy_set_errno(NOTIFY_INVAL);
		return (-1);
	}
	return(0);
}

/* atv-btv */
pkg_private struct timeval
ndet_tv_subt(atv, btv)
	struct	timeval atv, btv;
{
	if ((atv.tv_usec < btv.tv_usec) && atv.tv_sec) {
		atv.tv_usec += 1000000;
		atv.tv_sec--;
	}
	if (atv.tv_usec > btv.tv_usec)
		atv.tv_usec -= btv.tv_usec;
	else
		atv.tv_usec = 0;
	if (atv.tv_sec > btv.tv_sec)
		atv.tv_sec -= btv.tv_sec;
	else {
		if  (atv.tv_sec < btv.tv_sec)
			atv.tv_usec = 0;
		atv.tv_sec = 0;
	}
	/* Prevent getting polling constant for a result */
	if (ndet_tv_equal(atv, NOTIFY_POLLING_ITIMER.it_value))
		timerclear(&atv);
	return(atv);
}

/* min(atv, btv) */
pkg_private struct timeval
ndet_tv_min(atv, btv)
	struct	timeval atv, btv;
{
	struct	timeval res;

	res = ndet_tv_subt(atv, btv);
	if (timerisset(&res))
		/* Means that btv is < atv */
		return(btv);
	else
		/* Means that atv is <= btv */
		return(atv);
}

/*
 * Figure the interval that has transpired since this real interval timer
 * has been set.  The difference between how much time the timer wants
 * to wait and how long it has waited is the amount of time left to wait.
 * The time left to wait is returned.
 */
pkg_private struct timeval
ndet_real_min(ntfy_itimer, current_tv)
	NTFY_ITIMER *ntfy_itimer;
	struct timeval current_tv;
{
	struct timeval time_dif;

	time_dif = ndet_tv_subt(current_tv, ntfy_itimer->set_tv);
	return(ndet_tv_subt(ntfy_itimer->itimer.it_value, time_dif));
}


/*
 * Update the interval until expiration by subtracting the amount of
 * time on the process interval timer (current_tv) from the
 * value of the process interval timer when it was last looked at by
 * this client (ntfy_itimer->set_tv).
 *
 * Return the amount of time that this virtual interval timer has to go
 * before expiration (ntfy_itimer->itimer.it_value).  When the overall
 * minimum time until expiration is computed, ntfy_itimer will update
 * ntfy_itimer->set_tv with this value.
 */
pkg_private struct timeval
ndet_virtual_min(ntfy_itimer, current_tv)
	NTFY_ITIMER *ntfy_itimer;
	struct timeval current_tv;
{
	struct timeval time_dif;

	time_dif = ndet_tv_subt(ntfy_itimer->set_tv, current_tv);
	ntfy_itimer->itimer.it_value = ndet_tv_subt(
	    ntfy_itimer->itimer.it_value, time_dif);
	return(ntfy_itimer->itimer.it_value);
}

/*
 * Dispatch notification, reset itimer value,
 * remove if nothing to wait for and returns -1 else 0.
 */
pkg_private int
ndet_itimer_expired(client, condition)
	NTFY_CLIENT *client;
	NTFY_CONDITION *condition;
{
	register NTFY_ITIMER *ntfy_itimer = condition->data.ntfy_itimer;
	NTFY_CLIENT client_tmp;
	NTFY_CONDITION condition_tmp;
	int rc = 0;
	Notify_func functions[NTFY_FUNCS_MAX];

	/*
	 * Remember contents of following for call to ndis_enqueue because
	 * call to notify_set_itimer_func can invalidate.
	 */
	client_tmp = *client;
	condition_tmp = *condition;
        if (condition->func_count > 1) {
                condition_tmp.callout.functions = functions;
                bcopy((caddr_t)condition->callout.functions,
		    (caddr_t)condition_tmp.callout.functions,
		    sizeof(NTFY_NODE));
        } else
                condition_tmp.callout.function = condition->callout.function;
	/* Reset itimer value from interval */
	ntfy_itimer->itimer.it_value = ntfy_itimer->itimer.it_interval;
	/* Remove condition if nothing to wait for */
	if (!timerisset(&(ntfy_itimer->itimer.it_value))) {
		Notify_func func, myfunc = nint_get_func(condition);

		/*
		 * Enumerator (which is calling this) is designed to survive
		 * having condition disappear in middle of enumeration.
		 */
		func = notify_set_itimer_func(client->nclient, NOTIFY_FUNC_NULL,
		    (condition->type == NTFY_REAL_ITIMER)? ITIMER_REAL:
		      ITIMER_VIRTUAL,
		    &NOTIFY_NO_ITIMER, (struct itimerval *)0);
		ntfy_assert(func == myfunc, "Error when removing itimer");
		rc = -1;
	}
	/* Send notification */
	if (ndis_enqueue(&client_tmp, &condition_tmp) != NOTIFY_OK)
		ntfy_fatal_error("Error when enq condition");
	return (rc);
}

pkg_private void
ndet_reset_itimer_set_tv(condition)
	NTFY_CONDITION *condition;
{
	int n;

	if (condition->type == NTFY_REAL_ITIMER) {

		n = gettimeofday(&condition->data.ntfy_itimer->set_tv, (struct timezone *)0);
		    /* SYSTEM CALL */
		ntfy_assert(n == 0, "Unexpected error: gettimeofday");
	} else {
		struct	itimerval process_itimer;

		n = getitimer(ITIMER_VIRTUAL, &process_itimer); /*SYSTEM CALL*/
		ntfy_assert(n == 0, "Unexpected error: getitimer");
		condition->data.ntfy_itimer->set_tv = process_itimer.it_value;
		
	}
}
