#ifndef	lint
#ifdef sccs
static	char sccsid[] = "@(#)ndet_get_func.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Ndet_get_func.c - Implement the ndet_get_func private interface.
 */

#include <sunwindow/ntfy.h>
#include <sunwindow/ndet.h>
#include <sunwindow/nint.h>

pkg_private Notify_func
ndet_get_func(nclient, type, data, use_data)
	Notify_client nclient;
	NTFY_TYPE type;
	NTFY_DATA data;
	int use_data;
{
	register Notify_func func = NOTIFY_FUNC_NULL;
	register NTFY_CLIENT *client;
	NTFY_CONDITION *condition;

	NTFY_BEGIN_CRITICAL;
	/* Find client */
	if ((client = ntfy_find_nclient(ndet_clients, nclient,
            &ndet_client_latest)) == NTFY_CLIENT_NULL) {
		ntfy_set_warning(NOTIFY_UNKNOWN_CLIENT);
		goto Done;
	}
	/* Find condition */
	if ((condition = ntfy_find_condition(client->conditions, type,
	    &(client->condition_latest), data, use_data)) ==
	    NTFY_CONDITION_NULL) {
		ntfy_set_warning(NOTIFY_NO_CONDITION);
		goto Done;
	}
	/* Get function */
	func = nint_get_func(condition);
Done:
	NTFY_END_CRITICAL;
	return(func);
}

