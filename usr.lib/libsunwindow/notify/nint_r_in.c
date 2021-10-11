#ifndef	lint
#ifdef sccs
static	char sccsid[] = "@(#)nint_r_in.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Nint_r_in.c - Implement the notify_remove_input_func interface.
 */

#include <sunwindow/ntfy.h>
#include <sunwindow/nint.h>

extern Notify_error	
notify_remove_input_func(nclient, func, fd)
	Notify_client nclient;
	Notify_func func;
	int fd;
{
	return(nint_remove_fd_func(nclient, func, NTFY_INPUT, fd));
}

