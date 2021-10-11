#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)ttytlsw_tio.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Flavor of ttysw that knows about tool windows and allows tty based
 *	programs to set/get data about the tool window (toolio based
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

extern	char * calloc();

struct	toolsw *
ttytlsw_createtoolsubwindow(tool, name, width, height)
	struct tool *tool;
	char *name;
	short width, height;
{
	struct toolsw *toolsw;
	struct ttytoolsubwindow *ttytlsw;
	int ttytlsw_done();

	ttytlsw = (struct ttytoolsubwindow *)
			LINT_CAST(calloc(1, sizeof (*ttytlsw)));
	if (ttytlsw == (struct ttytoolsubwindow *)0)
		return((struct toolsw *)0);
	/* Create ttysw subwindow */
	toolsw = ttysw_createtoolsubwindow(tool, name, width, height);
	/* Do std ttytlsw set up */
	ttytlsw->tool = tool;
	(void)ttytlsw_setup(ttytlsw, (struct ttysubwindow *)LINT_CAST(toolsw->ts_data));
	/* Do destroy set up */
	ttytlsw->cached_destroyop = toolsw->ts_destroy;
	toolsw->ts_destroy = ttytlsw_done;
	return (toolsw);
}

ttytlsw_done(ttysw_client)
	Ttysubwindow ttysw_client;
{
	struct ttytoolsubwindow *ttytlsw;
	int (*func)();
	Ttysw	*ttysw;

	ttysw = (Ttysw *)(LINT_CAST(ttysw_client));
	ttytlsw = (struct ttytoolsubwindow *)(LINT_CAST(ttysw->ttysw_client));
	func = ttytlsw->cached_destroyop;
	(void)ttytlsw_cleanup(ttysw);
	(*func)(ttysw);
}

