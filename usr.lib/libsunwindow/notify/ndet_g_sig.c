#ifndef	lint
#ifdef sccs
static	char sccsid[] = "@(#)ndet_g_sig.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Ndet_g_sig.c - Implement the notify_get_signal_func interface.
 */

#include <sunwindow/ntfy.h>
#include <sunwindow/ndet.h>

extern Notify_func
notify_get_signal_func(nclient, signal, mode)
	Notify_client nclient;
	int signal;
	Notify_signal_mode mode;
{
	NTFY_TYPE type;

	/* Check arguments */
	if (ndet_check_mode(mode, &type))
		return(NOTIFY_FUNC_NULL);
	if (ndet_check_sig(signal))
		return(NOTIFY_FUNC_NULL);
	return(ndet_get_func(nclient, type, (NTFY_DATA)signal, NTFY_USE_DATA));
}

