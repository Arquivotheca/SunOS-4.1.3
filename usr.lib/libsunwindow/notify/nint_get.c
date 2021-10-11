#ifndef	lint
#ifdef sccs
static	char sccsid[] = "@(#)nint_get.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Nint_get.c - Implement the nint_get_func private interface.
 */

#include <sunwindow/ntfy.h>
#include <sunwindow/ndet.h>
#include <sunwindow/nint.h>

pkg_private Notify_func
nint_get_func(cond)
	register NTFY_CONDITION *cond;
{
	Notify_func func;

	if (cond->func_count > 1)
		func = cond->callout.functions[cond->func_count-1];
	else
		func = cond->callout.function;
	return(func);
}

