#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)tool_create.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 * Tool_create: Tool/sw creation code.
 */
#include <stdio.h>
#include <sys/file.h>
#include <suntool/tool_hs.h>
#include <sunwindow/win_ioctl.h>
#include "sunwindow/sv_malloc.h"
#include <suntool/wmgr.h>
#include <suntool/tool_impl.h>

extern	struct pixfont *pf_sys;

struct	toolsw *tool_lastsw(), *tool_addsw();

struct	tool *
tool_create(name, flags, tool_rect, icon)
	char		*name;
	int		 flags;
	struct rect	*tool_rect;
	struct icon	*icon;
{
	caddr_t		 args[32],
			*pointer = args;

	*pointer++ = (caddr_t)WIN_LABEL;
	*pointer++ = (caddr_t)name;
	if (flags & TOOL_NAMESTRIPE)  {
		*pointer++ = (caddr_t)WIN_NAME_STRIPE;
		*pointer++ = (caddr_t) TRUE;
	}
	if (flags & TOOL_BOUNDARYMGR)  {
		*pointer++ = (caddr_t)WIN_BOUNDARY_MGR;
		*pointer++ = (caddr_t) TRUE;
	}
	if (tool_rect != (struct rect *) NULL)  {
		*pointer++ = (caddr_t)WIN_LEFT;
		*pointer++ = (caddr_t)tool_rect->r_left;
		*pointer++ = (caddr_t)WIN_TOP;
		*pointer++ = (caddr_t)tool_rect->r_top;
		*pointer++ = (caddr_t)WIN_WIDTH;
		*pointer++ = (caddr_t)tool_rect->r_width;
		*pointer++ = (caddr_t)WIN_HEIGHT;
		*pointer++ = (caddr_t)tool_rect->r_height;
	}
	if (icon != (struct icon *) NULL)  {
		*pointer++ = (caddr_t)WIN_ICON;
		*pointer++ = (caddr_t) icon;
	}
	*pointer++ = (caddr_t) 0;
	return tool_make(WIN_ATTR_LIST, args, 0);
}

struct	toolsw *
tool_createsubwindow(tool, name, width, height)
	struct	tool *tool;
	char	*name;
	short	width, height;
{
	int	swfd, link;
	struct	toolsw *swprevious, *sw;
	struct	rect rect;

/*  Create sub window */

	if ((swfd = win_getnewwindow())<0) {
		(void)fprintf(stderr, "couldn't get a new subwindow\n");
		perror(tool->tl_name);
		return(0);
	}

/*  Find the subwindow at end of list */

	swprevious = tool_lastsw(tool);

/*  Setup links */

	link = win_fdtonumber(tool->tl_windowfd);
	(void)win_setlink(swfd, WL_PARENT, link);
	if (swprevious) {
		link = win_fdtonumber(swprevious->ts_windowfd);
		(void)win_setlink(swfd, WL_OLDERSIB, link);
	}
	(void)tool_positionsw(tool, swprevious, width, height, &rect);
	(void)win_setrect(swfd, &rect);

/*  Install in tool and window tree */

	(void)win_insert(swfd);
	(void)tool_repaint_all_later(tool);
	sw = tool_addsw(tool, swfd, name, width, height);
	return(sw);
}

/*
 * Utilities
 */

tool_repaint_all_later(tool)
	struct	tool *tool;
{
	int flags = win_get_flags((char *)tool);

	(void) win_set_flags((char *)tool, (unsigned)(flags|PW_REPAINT_ALL));
}

struct	toolsw *
tool_lastsw(tool)
	struct	tool *tool;
{
	struct	toolsw *swlast, *sw;

/*  Find the subwindow at end of list */

	swlast = tool->tl_sw;
	for (sw = tool->tl_sw;sw;sw = sw->ts_next) {
		swlast = sw;
	}
	return(swlast);
}

struct	toolsw *
tool_addsw(tool, windowfd, name, width, height)
	struct	tool *tool;
	int	windowfd;
	char	*name;
	short	width, height;
{
	struct	toolsw
	    *swlast = tool_lastsw(tool),
	    *sw = (struct toolsw *) (LINT_CAST(sv_calloc(1, sizeof (struct toolsw))));

	sw->ts_windowfd = windowfd;
	sw->ts_name = name;
	sw->ts_width = width;
	sw->ts_height = height;
	sw->ts_priv = (caddr_t) (LINT_CAST(sv_calloc(1, sizeof(Toolsw_priv))));
	if (swlast)
		swlast->ts_next = sw;
	else
		tool->tl_sw = sw;
	return(sw);
}
