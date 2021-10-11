#ifndef	lint
#ifdef sccs
static	char sccsid[] = "@(#)ndet_do_dis.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Ndet_do_dis.c - In order for "background" dispatching to be done,
 *		   notify_do_dispatch must be called.  Background dispatching
 *		   is that dispatching which is done when a read or select is
 *		   called before calling notify_start.
 */

#include <sunwindow/ntfy.h>
#include <sunwindow/ndet.h>

extern Notify_error
notify_do_dispatch()
{
	ndet_flags |= NDET_DISPATCH;
	return (NOTIFY_OK);
}

