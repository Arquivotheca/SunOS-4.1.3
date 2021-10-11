#ifndef	lint
#ifdef sccs
static	char sccsid[] = "@(#)ndet_veto.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Ndet_veto.c - Implementation of notify_veto_destroy.
 */

#include <sunwindow/ntfy.h>
#include <sunwindow/ndet.h>

/* ARGSUSED */
extern Notify_error
notify_veto_destroy(nclient)
	Notify_client nclient;
{
	NTFY_BEGIN_CRITICAL;
	ndet_flags |= NDET_VETOED;
	NTFY_END_CRITICAL;
	return(NOTIFY_OK);
}

