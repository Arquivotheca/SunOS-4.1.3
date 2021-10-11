#ifndef	lint
#ifdef sccs
static	char sccsid[] = "@(#)ndet_g_in.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Ndet_g_in.c - Implement notify_get_input_func call.
 */

#include <sunwindow/ntfy.h>
#include <sunwindow/ndet.h>

extern Notify_func
notify_get_input_func(nclient, fd)
	Notify_client nclient;
	int fd;
{
	return(ndet_get_fd_func(nclient, fd, NTFY_INPUT));
}

