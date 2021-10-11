#ifndef	lint
#ifdef sccs
static	char sccsid[] = "@(#)ndet_g_pri.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Ndet_g_pri.c - Implement the notify_get_prioritizer_func interface.
 */

#include <sunwindow/ntfy.h>
#include <sunwindow/ndet.h>

extern Notify_func
notify_get_prioritizer_func(nclient)
	Notify_client nclient;
{
	register NTFY_CLIENT *client;
	register Notify_func func = NOTIFY_FUNC_NULL;

	NTFY_BEGIN_CRITICAL;
	/* Find client that corresponds to nclient */
	if ((client = ntfy_find_nclient(ndet_clients, nclient,
            &ndet_client_latest)) == NTFY_CLIENT_NULL) {
		ntfy_set_errno(NOTIFY_UNKNOWN_CLIENT);
		goto Done;
	}
	/* Get function */
	func = client->prioritizer;
Done:
	NTFY_END_CRITICAL;
	return(func);
}

