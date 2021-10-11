#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)ttytlsw_notify.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Flavor of ttysw that knows about tool windows and allows tty based
 *	programs to set/get data about the tool window (notifier based
 *	flavor).
 */


#include <stdio.h>
#include <sys/file.h>
#include <sys/signal.h>
#include <sunwindow/window_hs.h>
#include <sunwindow/win_lock.h>
#include <suntool/icon.h>
#include <suntool/icon_load.h>
#include <suntool/tool.h>
#include <suntool/wmgr.h>
#include <suntool/ttysw.h>
#include <suntool/ttysw_impl.h>
#include <suntool/ttytlsw_impl.h>

extern	char	* calloc();

caddr_t
ttytlsw_create(tool, name, width, height)
	struct tool *tool;
	char *name;
	short width, height;
{
	struct ttysubwindow *ttysw;
	struct ttytoolsubwindow *ttytlsw;
	Notify_value ttytlsw_destroy();
	Notify_func func;

	ttytlsw = (struct ttytoolsubwindow *) LINT_CAST(calloc(1, sizeof (*ttytlsw)));
	if (ttytlsw == (struct ttytoolsubwindow *)0)
		return(NULL);
	/* Create ttysw subwindow */
	ttysw = (struct ttysubwindow *)LINT_CAST(ttysw_create(tool, name, width, height));
	if (!ttysw)
		return(NULL);
	/* Do std ttytlsw set up */
	ttytlsw->tool = tool;
	(void)ttytlsw_setup(ttytlsw, ttysw);
	/* Do destroy set up */
	if ((func = notify_set_destroy_func((Notify_client)(LINT_CAST(ttysw)), ttytlsw_destroy)) ==
	    NOTIFY_FUNC_NULL)
		return(NULL);
	ttytlsw->cached_destroyop = (int (*)())func;
	return((caddr_t)ttysw);
}

Notify_value ttytlsw_destroy(ttysw_client, status)
	Ttysubwindow ttysw_client;
	Destroy_status status;
{
	struct ttytoolsubwindow *ttytlsw;
	Notify_func func;
	Ttysw	*ttysw;

	ttysw = (Ttysw *)(LINT_CAST(ttysw_client));
	ttytlsw = (struct ttytoolsubwindow *)(LINT_CAST(ttysw->ttysw_client));
	if (status != DESTROY_CHECKING) {
		func = (Notify_func)ttytlsw->cached_destroyop;
		(void)ttytlsw_cleanup(ttysw);
		return((*func)(ttysw, status));
	}
	return(NOTIFY_IGNORED);
}

