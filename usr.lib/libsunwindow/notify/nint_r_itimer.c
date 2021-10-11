#ifndef	lint
#ifdef sccs
static	char sccsid[] = "@(#)nint_r_itimer.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Nint_r_itimer.c - Implement the notify_remove_itimer_func interface.
 */

#include <sunwindow/ntfy.h>
#include <sunwindow/ndet.h>
#include <sunwindow/nint.h>

extern Notify_error
notify_remove_itimer_func(nclient, func, which)
	Notify_client nclient;
	Notify_func func;
	int which;
{
	NTFY_TYPE type;

	/* Check arguments */
	if (ndet_check_which(which, &type))
		return(notify_errno);
	return(nint_remove_func(nclient, func, type, NTFY_DATA_NULL,
	    NTFY_IGNORE_DATA));
}
