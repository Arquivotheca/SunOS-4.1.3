#ifndef	lint
#ifdef sccs
static	char sccsid[] = "@(#)nint_set.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Nint_set.c - Implement the nint_set_func private interface.
 */

#include <sunwindow/ntfy.h>
#include <sunwindow/ndet.h>
#include <sunwindow/nint.h>

pkg_private Notify_func
nint_set_func(cond, new_func)
	register NTFY_CONDITION *cond;
	Notify_func new_func;
{
	Notify_func old_func;

	if (cond->func_count > 1) {
		old_func = cond->callout.functions[cond->func_count-1];
		cond->callout.functions[cond->func_count-1] = new_func;
	} else {
		old_func = cond->callout.function;
		cond->callout.function = new_func;
		cond->func_count = 1;
	}
	return(old_func);
}

