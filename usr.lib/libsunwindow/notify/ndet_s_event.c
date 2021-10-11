#ifndef	lint
#ifdef sccs
static	char sccsid[] = "@(#)ndet_s_event.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Ndet_s_event.c - Implement the notify_set_event_func interface.
 */

#include <sunwindow/ntfy.h>
#include <sunwindow/ndet.h>
#include <sunwindow/ndis.h>
#include <sunwindow/nint.h>

extern Notify_func
notify_set_event_func(nclient, func, when)
	Notify_client nclient;
	Notify_func func;
	Notify_event_type when;
{
	Notify_func old_func = NOTIFY_FUNC_NULL;
	register NTFY_CLIENT *client;
	NTFY_CONDITION *condition;
	NTFY_TYPE type;

	NTFY_BEGIN_CRITICAL;
	/* Check arguments */
	if (ndet_check_when(when, &type))
		goto Done;
	/* Find/create client that corresponds to nclient */
	if ((client = ntfy_new_nclient(&ndet_clients, nclient,
            &ndet_client_latest)) == NTFY_CLIENT_NULL)
		goto Done;
	/* Find/create condition */
	if ((condition = ntfy_new_condition(&(client->conditions), type,
	    &(client->condition_latest), NTFY_DATA_NULL, NTFY_IGNORE_DATA)) ==
	    NTFY_CONDITION_NULL)
		goto Done;
	/* Exchange functions */
	old_func = nint_set_func(condition, func);
	/* Remove condition if func is null */
	if (func == NOTIFY_FUNC_NULL) {
		ndis_flush_condition(nclient, type,
		    NTFY_DATA_NULL, NTFY_IGNORE_DATA);
		ntfy_unset_condition(&ndet_clients, client, condition,
		    &ndet_client_latest, NTFY_NDET);
	}
	/*
	 * Since notifier doesn't detect client events, don't need to
	 * add *CHANGE flag to ndet_flags.
	 */
Done:
	NTFY_END_CRITICAL;
	return(old_func);
}

