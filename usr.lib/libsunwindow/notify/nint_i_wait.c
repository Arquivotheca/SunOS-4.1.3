#ifndef	lint
#ifdef sccs
static	char sccsid[] = "@(#)nint_i_wait.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Nint_i_wait.c - Implement the notify_interpose_wait3_func interface.
 */

#include <sunwindow/ntfy.h>
#include <sunwindow/ndet.h>
#include <sunwindow/nint.h>

extern Notify_error
notify_interpose_wait3_func(nclient, func, pid)
	Notify_client nclient;
	Notify_func func;
	int pid;
{
	/* Check arguments */
	if (ndet_check_pid(pid))
		return(notify_errno);
	return(nint_interpose_func(nclient, func, NTFY_WAIT3,
	    (NTFY_DATA)pid, NTFY_USE_DATA));
}

