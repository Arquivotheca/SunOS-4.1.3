#ifndef	lint
#ifdef sccs
static	char sccsid[] = "@(#)ndet_s_except.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Ndet_s_except.c - Implement notify_set_exception_func call.
 */

#include <sunwindow/ntfy.h>
#include <sunwindow/ndet.h>

extern Notify_func
notify_set_exception_func(nclient, func, fd)
	Notify_client nclient;
	Notify_func func;
	int fd;
{
	return(ndet_set_fd_func(nclient, func, fd, NTFY_EXCEPTION));
}

