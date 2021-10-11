#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)wmgr_attic.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * 	wmgr_attic.c:	out-of-date & apparently unused wmgr routines
 */

#include <stdio.h>
#include <errno.h>
#include <sys/file.h>
#include <sunwindow/window_hs.h>
#include <sunwindow/win_ioctl.h>
#include <suntool/wmgr.h>
#include <suntool/fullscreen.h>

 
wmgr_getnormalrect(windowfd, rectp)
	int windowfd;
	struct rect *rectp;
{
	if (wmgr_iswindowopen(windowfd))
		(void)win_getrect(windowfd, rectp);
	else
		(void)win_getsavedrect(windowfd, rectp);
}

wmgr_setnormalrect(windowfd, rectp)
	int windowfd;
	struct rect *rectp;
{
	if (wmgr_iswindowopen(windowfd))
		(void)win_setrect(windowfd, rectp);
	else
		(void)win_setsavedrect(windowfd, rectp);
}




/*
 * Store the globally needed values of where the next tool/icon rect should
 * be positioned
 */
/*ARGSUSED*/
wmgr_setrectalloc(rootfd, tool_left, tool_top, icon_left, icon_top)
	int	rootfd;
	short	tool_left, tool_top, icon_left, icon_top;
{
	struct	rect ri, ro;

	if (!wmgr_get_placeholders(&ri, &ro)) {
		(void)fprintf(stderr, "Placement values not stored.\n");
		return;
	}
	ro.r_left = tool_left;
	ro.r_top = tool_top;
	ri.r_left = icon_left;
	ri.r_top = icon_top;
	(void)wmgr_set_placeholders(&ri, &ro);
}

/*ARGSUSED*/
wmgr_getrectalloc(rootfd, tool_left, tool_top, icon_left, icon_top)
	int	rootfd;
	short	*tool_left, *tool_top, *icon_left, *icon_top;
{
	struct	rect ri, ro;

	if (!wmgr_get_placeholders(&ri, &ro)) {
		(void)fprintf(stderr, "Placement values not stored.\n");
		return;
	}
	*tool_left = ro.r_left;
	*tool_top = ro.r_top;
	*icon_left = ri.r_left;
	*icon_top = ri.r_top;
}
