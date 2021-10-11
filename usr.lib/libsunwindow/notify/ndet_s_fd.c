#ifndef	lint
#ifdef sccs
static	char sccsid[] = "@(#)ndet_s_fd.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Ndet_s_fd.c - Implement notify_set_*_func file descriptor specific calls
 * that are shared among NTFY_INPUT, NTFY_OUTPUT and NTFY_EXCEPTION.
 */

#include <sunwindow/ntfy.h>
#include <sunwindow/ndet.h>
#include <sunwindow/ndis.h>
#include <sunwindow/nint.h>

extern Notify_func
ndet_set_fd_func(nclient, func, fd, type)
	Notify_client nclient;
	Notify_func func;
	int fd;
	NTFY_TYPE type;
{
	Notify_func old_func = NOTIFY_FUNC_NULL;
	register NTFY_CLIENT *client;
	NTFY_CONDITION *condition;

	NTFY_BEGIN_CRITICAL;
	/* Check arguments */
	if (ndet_check_fd(fd))
		goto Done;
	/* Find/create client that corresponds to nclient */
	if ((client = ntfy_new_nclient(&ndet_clients, nclient,
            &ndet_client_latest)) == NTFY_CLIENT_NULL)
		goto Done;
	/* Find/create condition */
	if ((condition = ntfy_new_condition(&(client->conditions), type,
	    &(client->condition_latest), (NTFY_DATA)fd, NTFY_USE_DATA)) ==
	    NTFY_CONDITION_NULL)
		goto Done;
	/* Exchange functions */
	old_func = nint_set_func(condition, func);
	/* Remove condition if func is null */
	if (func == NOTIFY_FUNC_NULL) {
		ndis_flush_condition(nclient, type,
		    (NTFY_DATA)fd, NTFY_USE_DATA);
		ntfy_unset_condition(&ndet_clients, client, condition,
		    &ndet_client_latest, NTFY_NDET);
	}
	/*
	 * Have notifier check for fd activity next time around loop.
	 * Will notice and adjust signal handling (e.g., SIGIO & SIGURG)
	 * at this time.
	 */
	ndet_flags |= NDET_FD_CHANGE;
Done:
	NTFY_END_CRITICAL;
	return(old_func);
}

