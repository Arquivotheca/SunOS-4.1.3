#ifndef	lint
#ifdef sccs
static	char sccsid[] = "@(#)ndet_death.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Ndet_death.c - Common routines between notify_post_destroy and notify_die.
 */

#include <sunwindow/ntfy.h>
#include <sunwindow/ndet.h>
#include <sunwindow/ndis.h>
#include <sunwindow/nint.h>

pkg_private int
ndet_check_status(status)
	Destroy_status status;
{
	if (!(status == DESTROY_PROCESS_DEATH || status == DESTROY_CHECKING ||
	    status == DESTROY_CLEANUP)) {
		ntfy_set_errno(NOTIFY_INVAL);
		return(-1);
	}
	return(0);
}

/*
 * Immediate destroy notification.
 */
pkg_private NTFY_ENUM
ndet_immediate_destroy(client, condition, context)
	NTFY_CLIENT *client;
	register NTFY_CONDITION *condition;
	NTFY_ENUM_DATA context;
{
	Destroy_status status = (Destroy_status)context;

	if (condition->type == NTFY_DESTROY) {
		Notify_func func = nint_push_callout(client, condition);

		/* Send destroy notification */
		ndet_flags &= ~NDET_VETOED;
		NTFY_END_CRITICAL;
		(void) func(client->nclient, status);
		NTFY_BEGIN_CRITICAL;
		nint_unprotected_pop_callout();
		/* Return NTFY_ENUM_TERM if checking and was told to veto */
		if (status == DESTROY_CHECKING && (ndet_flags & NDET_VETOED))
			return(NTFY_ENUM_TERM);
		else
			/*
			 * Since only one destroy per client can skip other
			 * conditions.
			 */
			return(NTFY_ENUM_SKIP);
	}
	return(NTFY_ENUM_NEXT);
}

