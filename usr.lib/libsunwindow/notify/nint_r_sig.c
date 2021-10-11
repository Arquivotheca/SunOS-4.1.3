#ifndef	lint
#ifdef sccs
static	char sccsid[] = "@(#)nint_r_sig.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Nint_r_sig.c - Implement the notify_remove_signal_func interface.
 */

#include <sunwindow/ntfy.h>
#include <sunwindow/ndet.h>
#include <sunwindow/nint.h>

extern Notify_error
notify_remove_signal_func(nclient, func, signal, mode)
	Notify_client nclient;
	Notify_func func;
	int signal;
	Notify_signal_mode mode;
{
	NTFY_TYPE type;

	/* Check arguments */
	if (ndet_check_mode(mode, &type))
		return(notify_errno);
	if (ndet_check_sig(signal))
		return(notify_errno);
	return(nint_remove_func(nclient, func, type, (NTFY_DATA)signal,
	    NTFY_USE_DATA));
}

