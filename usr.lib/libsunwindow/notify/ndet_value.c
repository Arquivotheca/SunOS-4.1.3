#ifndef	lint
#ifdef sccs
static	char sccsid[] = "@(#)ndet_value.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Ndet_value.c - Implement the notify_itimer_value interface.
 */

#include <sunwindow/ntfy.h>
#include <sunwindow/ndet.h>

extern Notify_error
notify_itimer_value(nclient, which, value)
	Notify_client nclient;
	int which;
	struct itimerval *value;
{
	register NTFY_CLIENT *client;
	NTFY_CONDITION *condition;
	NTFY_TYPE type;
	int n;

	NTFY_BEGIN_CRITICAL;
	/* Check arguments */
	if (value == NTFY_ITIMER_NULL) {
		ntfy_set_errno(NOTIFY_INVAL);
		goto Error;
	}
	/* Initialize return value to no itimer */
	*value = NOTIFY_NO_ITIMER;
	if (ndet_check_which(which, &type))
		goto Error;
	/* Find client that corresponds to nclient */
	if ((client = ntfy_find_nclient(ndet_clients, nclient,
            &ndet_client_latest)) == NTFY_CLIENT_NULL) {
		ntfy_set_warning(NOTIFY_UNKNOWN_CLIENT);
		goto Error;
	}
	/* Find condition */
	if ((condition = ntfy_find_condition(client->conditions, type,
	    &(client->condition_latest), NTFY_DATA_NULL, NTFY_IGNORE_DATA)) ==
	    NTFY_CONDITION_NULL) {
		ntfy_set_warning(NOTIFY_NO_CONDITION);
		goto Error;
	}
	/* Set value */
	value->it_interval = condition->data.ntfy_itimer->itimer.it_interval;
	if (type == NTFY_REAL_ITIMER) {
		struct	timeval tod;

		n = gettimeofday(&tod, (struct timezone *)0); /* SYSTEM CALL */
		ntfy_assert(n == 0, "Unexpected error: gettimeofday");
		value->it_value = ndet_real_min(condition->data.ntfy_itimer,
		    tod);
	} else {
		struct	itimerval process_itimer;

		n = getitimer(ITIMER_VIRTUAL, &process_itimer);
		ntfy_assert(n == 0, "Unexpected error: getitimer");
		value->it_value = ndet_virtual_min(condition->data.ntfy_itimer,
		    process_itimer.it_value);
		condition->data.ntfy_itimer->set_tv = process_itimer.it_value;
	}
	NTFY_END_CRITICAL;
	return(NOTIFY_OK);
Error:
	NTFY_END_CRITICAL;
	return(notify_errno);
}

