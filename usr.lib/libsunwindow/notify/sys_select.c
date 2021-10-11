#ifndef	lint
#ifdef sccs
static	char sccsid[] = "@(#)sys_select.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif
 
/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Sys_select.c - Real system call to select.
 */

#include <syscall.h>
#include <sunwindow/ntfy.h>	/* For ntfy_assert */
#include <errno.h>		/* For debugging */
#include <stdio.h>		/* For debugging */

extern	errno;

pkg_private int
notify_select(nfds, readfds, writefds, exceptfds, timeout)
	int nfds, *readfds, *writefds, *exceptfds;
	struct timeval *timeout;
{
	nfds = syscall(SYS_select, nfds, readfds, writefds, exceptfds, timeout);
	ntfy_assert(!(nfds == 0 && timeout == (struct timeval *)0 &&
	    *readfds == 0 && *writefds == 0 && *exceptfds == 0),
	    "SYS_select returned when no stimuli");
	return(nfds);
}

