#ifndef	lint
#ifdef sccs
static	char sccsid[] = "@(#)ndet_g_fd.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Ndet_g_fd.c - Implement notify_get_*_func file descriptor specific calls
 * that are shared among NTFY_INPUT, NTFY_OUTPUT and NTFY_EXCEPTION.
 */

#include <sunwindow/ntfy.h>
#include <sunwindow/ndet.h>

extern Notify_func
ndet_get_fd_func(nclient, fd, type)
	Notify_client nclient;
	int fd;
	NTFY_TYPE type;
{
	/* Check arguments */
	if (ndet_check_fd(fd))
		return(NOTIFY_FUNC_NULL);
	return(ndet_get_func(nclient, type, (NTFY_DATA)fd, NTFY_USE_DATA));
}

