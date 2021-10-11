#ifndef	lint
#ifdef sccs
static	char sccsid[] = "@(#)nint_n_in.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Nint_n_in.c - Implement the notify_next_input_func interface.
 */

#include <sunwindow/ntfy.h>
#include <sunwindow/ndet.h>
#include <sunwindow/nint.h>

extern Notify_value
notify_next_input_func(nclient, fd)
	Notify_client nclient;
	int fd;
{
	return(nint_next_fd_func(nclient, NTFY_INPUT, fd));
}

