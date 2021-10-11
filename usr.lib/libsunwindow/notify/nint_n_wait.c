#ifndef	lint
#ifdef sccs
static	char sccsid[] = "@(#)nint_n_wait.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Nint_n_wait.c - Implement the notify_next_wait3_func interface.
 */

#include <sunwindow/ntfy.h>
#include <sunwindow/ndet.h>
#include <sunwindow/nint.h>

extern Notify_value	
notify_next_wait3_func(nclient, pid, status, rusage)
	Notify_client nclient;
	int pid;
	union wait *status;
	struct rusage *rusage;
{
	Notify_func func;

	/* Don't check pid because may be exiting */
	if ((func = nint_next_callout(nclient, NTFY_WAIT3)) == NOTIFY_FUNC_NULL)
		return(NOTIFY_UNEXPECTED);
	return(func(nclient, pid, status, rusage));
}

