#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)ttysw_tsw.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Tool subwindow creation of ttysw
 */

#include <sys/types.h>
#include <sys/time.h>

#include <pixrect/pixrect.h>
#include <pixrect/pixfont.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/pixwin.h>
#include <suntool/icon.h>
#include <suntool/tool.h>
#include <suntool/ttysw.h>

struct	toolsw *
ttysw_createtoolsubwindow(tool, name, width, height)
	struct tool *tool;
	char *name;
	short width, height;
{
	struct toolsw *toolsw;
	extern ttysw_handlesigwinch(), ttysw_selected(), ttysw_done();

	/*
	 * Create subwindow
	 */
	toolsw = tool_createsubwindow(tool, name, width, height);
	/*
	 * Setup ttysw
	 */
	toolsw->ts_data = ttysw_init(toolsw->ts_windowfd);
	toolsw->ts_io.tio_handlesigwinch = ttysw_handlesigwinch;
	toolsw->ts_io.tio_selected = ttysw_selected;
	toolsw->ts_destroy = ttysw_done;
	return (toolsw);
}
