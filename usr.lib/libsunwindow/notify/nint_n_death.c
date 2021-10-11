#ifndef	lint
#ifdef sccs
static	char sccsid[] = "@(#)nint_n_death.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Nint_n_death.c - Implement the notify_next_destroy_func interface.
 */

#include <sunwindow/ntfy.h>
#include <sunwindow/ndet.h>
#include <sunwindow/nint.h>

extern Notify_value	
notify_next_destroy_func(nclient, status)
	Notify_client nclient;
	Destroy_status status;
{
	Notify_func func;

	/* Check arguments */
	if (ndet_check_status(status))
		return(NOTIFY_UNEXPECTED);
	if ((func = nint_next_callout(nclient, NTFY_DESTROY)) ==
	    NOTIFY_FUNC_NULL)
		return(NOTIFY_UNEXPECTED);
	return(func(nclient, status));
}

