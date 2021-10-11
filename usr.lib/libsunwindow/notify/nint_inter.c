#ifndef	lint
#ifdef sccs
static	char sccsid[] = "@(#)nint_inter.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Nint_inter.c - Implement the nint_interpose_func private interface.
 */
#include <sunwindow/ntfy.h>
#include <sunwindow/ndet.h>
#include <sunwindow/nint.h>
#include <sunwindow/notify.h>

pkg_private Notify_error	
nint_interpose_func(nclient, func, type, data, use_data)
	Notify_client nclient;
	Notify_func func;
	NTFY_TYPE type;
	caddr_t data;
	int use_data;
{
	NTFY_CLIENT *client;
	register NTFY_CONDITION *cond;
	register int i;
	
	NTFY_BEGIN_CRITICAL;
	/* Find client */
	if ((client = ntfy_find_nclient(ndet_clients, nclient,
	    &ndet_client_latest)) == NTFY_CLIENT_NULL) {
		ntfy_set_errno(NOTIFY_UNKNOWN_CLIENT);
		goto Error;
	}
	/* Find condition using data */
	if ((cond = ntfy_find_condition(client->conditions, type,
	    &(client->condition_latest), data, use_data)) ==
	    NTFY_CONDITION_NULL) {
		ntfy_set_errno(NOTIFY_NO_CONDITION);
		goto Error;
	}
	/* See if going to exceded max number of functions supported */
	if (cond->func_count+1 > NTFY_FUNCS_MAX) {
		ntfy_set_errno(NOTIFY_FUNC_LIMIT);
		goto Error;
	}
	/* Allocate function list if this is first interposition */
	if (cond->func_count == 1) {
		Notify_func func_save = cond->callout.function;
		Notify_func *functions;
		
		if ((functions = ntfy_alloc_functions()) == NTFY_FUNC_PTR_NULL)
			goto Error;
		functions[0] = func_save;
		cond->callout.functions = functions;
	}
	/* Slide other functions over */
	for (i = cond->func_count;i > 0;i--)
		cond->callout.functions[i] = cond->callout.functions[i-1];
	/* Add new function to beginning of function list */
	cond->callout.functions[0] = func;
	cond->func_count++;
	NTFY_END_CRITICAL;
	return(NOTIFY_OK);
Error:
	NTFY_END_CRITICAL;
	return(notify_errno);
}

