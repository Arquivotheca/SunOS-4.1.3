#ifndef	lint
#ifdef sccs
static	char sccsid[] = "@(#)ndet_fd.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Ndet_fd.c - Implement file descriptor specific calls that are shared among
 * NTFY_INPUT, NTFY_OUTPUT and NTFY_EXCEPTION.
 */

#include <sunwindow/ntfy.h>
#include <sunwindow/ndet.h>

/* performance: global cache of getdtablesize() */
extern int dtablesize_cache;
#define GETDTABLESIZE() \
	(dtablesize_cache?dtablesize_cache:(dtablesize_cache=getdtablesize()))

pkg_private int
ndet_check_fd(fd)
	int fd;
{
	if (fd < 0 || fd >= GETDTABLESIZE()) {
		ntfy_set_errno(NOTIFY_BADF);
		return(-1);
	}
	return(0);
}

