#ifndef	lint
#ifdef sccs
static	char sccsid[] = "@(#)ndet_s_pri.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Ndet_s_pri.c - Implement the notify_set_prioritizer_func interface.
 */

#include <sunwindow/ntfy.h>
#include <sunwindow/ndet.h>
#include <sunwindow/ndis.h>	/* For ndis_default_prioritizer */

extern Notify_func
notify_set_prioritizer_func(nclient, func)
	Notify_client nclient;
	register Notify_func func;
{
	register Notify_func old_func = NOTIFY_FUNC_NULL;
	register NTFY_CLIENT *client;

	NTFY_BEGIN_CRITICAL;
	/* Find/create client that corresponds to nclient */
	if ((client = ntfy_new_nclient(&ndet_clients, nclient,
            &ndet_client_latest)) == NTFY_CLIENT_NULL)
		goto Done;
	/* Exchange functions */
	old_func = client->prioritizer;
	client->prioritizer = func;
	/* Use default if null */
	if (func == NOTIFY_FUNC_NULL)
		client->prioritizer = ndis_default_prioritizer;
Done:
	NTFY_END_CRITICAL;
	return(old_func);
}

