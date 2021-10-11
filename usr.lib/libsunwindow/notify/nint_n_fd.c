#ifndef	lint
#ifdef sccs
static	char sccsid[] = "@(#)nint_n_fd.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Nint_n_fd.c - Implement the nint_next_fd_func interface.
 */

#include <sunwindow/ntfy.h>
#include <sunwindow/ndet.h>
#include <sunwindow/nint.h>

pkg_private Notify_value	
nint_next_fd_func(nclient, type, fd)
	Notify_client nclient;
	NTFY_TYPE type;
	int fd;
{
	Notify_func func;

	/* Check arguments */
	if (ndet_check_fd(fd))
		return(NOTIFY_UNEXPECTED);
	if ((func = nint_next_callout(nclient, type)) == NOTIFY_FUNC_NULL)
		return(NOTIFY_UNEXPECTED);
	return(func(nclient, fd));
}

