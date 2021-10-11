#ifndef	lint
#ifdef sccs
static	char sccsid[] = "@(#)ndet_sig.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Ndet_sig.c - Implement signal specific calls that are shared among
 * NTFY_SYNC_SIGNAL and NTFY_ASYNC_SIGNAL.
 */

#include <sunwindow/ntfy.h>
#include <sunwindow/ndet.h>
#include <signal.h>

pkg_private int
ndet_check_mode(mode, type_ptr)
	Notify_signal_mode mode;
	NTFY_TYPE *type_ptr;
{
	NTFY_TYPE type;

	switch (mode) {
	case NOTIFY_SYNC: type = NTFY_SYNC_SIGNAL; break;
	case NOTIFY_ASYNC: type = NTFY_ASYNC_SIGNAL; break;
	default: ntfy_set_errno(NOTIFY_INVAL); return(-1);
	}
	if (type_ptr)
		*type_ptr = type;
	return(0);
}

pkg_private int
ndet_check_sig(sig)
	int sig;
{
	if (sig < 0 || sig >= NSIG) {
		ntfy_set_errno(NOTIFY_BAD_SIGNAL);
		return(-1);
	}
	return(0);
}

