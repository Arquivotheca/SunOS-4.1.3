#ifndef	lint
#ifdef sccs
static	char sccsid[] = "@(#)ntfy_debug.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif
 
/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Ntfy_debug.c - Debugging routines enabled by NTFY_DEBUG in ntfy.h
 */

#include <stdio.h>
#include <sunwindow/ntfy.h>

pkg_private_data int	ntfy_errno_no_print;
pkg_private_data int	ntfy_warning_print;

pkg_private_data int	ntfy_errno_abort;
pkg_private_data int	ntfy_errno_abort_init;

pkg_private void
ntfy_set_errno_debug(error)
	Notify_error error;
{
	notify_errno = error;
	if ((!ntfy_errno_no_print) && error != NOTIFY_OK)
		notify_perror("Notifier error");
	if (!ntfy_errno_abort_init) {
		extern char *getenv();
		char *str = getenv("NOTIFY_ERROR_ABORT");

		if (str && (str[0] == 'y' || str[0] == 'Y'))
			ntfy_errno_abort = 1;
		else
			ntfy_errno_abort = 0;
		ntfy_errno_abort_init = 1;
	}
	if (ntfy_errno_abort == 1 && error != NOTIFY_OK)
		abort();
}

pkg_private void
ntfy_set_warning_debug(error)
	Notify_error error;
{
	notify_errno = error;
	if (ntfy_warning_print && error != NOTIFY_OK)
		notify_perror("Notifier warning");
}

pkg_private void
ntfy_assert_debug(msg)
	char *msg;
{
	(void) fprintf(stderr, "Notifier assertion botched: %s\n", msg);
}

pkg_private void
ntfy_fatal_error(msg)
	char *msg;
{
	(void) fprintf(stderr, "Notifier fatal error: %s\n", msg);
}

