#ifndef	lint
#ifdef sccs
static	char sccsid[] = "@(#)nint_i_sig.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Nint_i_sig.c - Implement the notify_interpose_signal_func interface.
 */

#include <sunwindow/ntfy.h>
#include <sunwindow/ndet.h>
#include <sunwindow/nint.h>

extern Notify_error
notify_interpose_signal_func(nclient, func, signal, mode)
	Notify_client nclient;
	Notify_func func;
	int signal;
	Notify_signal_mode mode;
{
	NTFY_TYPE type;

	/*
	 * Check arguments & pre-allocate stack incase going to get
	 * asynchronous event before synchronous one.
	 */
	if (ndet_check_mode(mode, &type) || ndet_check_sig(signal) ||
	    (nint_alloc_stack() != NOTIFY_OK))
		return(notify_errno);
	return(nint_interpose_func(nclient, func, type, (NTFY_DATA)signal,
	    NTFY_USE_DATA));
}

