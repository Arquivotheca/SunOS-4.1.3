#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)tool_pos.c 1.1 92/07/30";
#endif
#endif

/*
 * Copyright (c) 1983, 1984, 1988 by Sun Microsystems, Inc.
 */

/*
 * Layout of subwindows in tool routines.
 */

#include <sys/types.h>
#include <sys/file.h>
#include <sys/time.h>
#include <pixrect/pixrect.h>
#include <pixrect/pixfont.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/pixwin.h>
#include <sunwindow/win_input.h>
#include <sunwindow/win_struct.h>
#include <sunwindow/win_cursor.h>
#include <sunwindow/cms.h>
#include <sunwindow/win_screen.h>
#include <sunwindow/win_ioctl.h>
#include <suntool/icon.h>
#include <suntool/tool.h>
#include <suntool/wmgr.h>

static int	tool_swsizecompute();

extern	struct pixfont *pf_sys;

/*
 * Position subwindow in tool.
 */
tool_positionsw(tool, swprevious, width, height, rect)
	struct	tool *tool;
	struct	toolsw *swprevious;
	int	width, height;
	struct	rect *rect;
{
	int	yoffset;
	struct	rect toolrect;

	/*
	 * Iconic adjustments
	 */
	(void)tool_getnormalrect(tool, &toolrect);
	yoffset = tool_sw_iconic_offset(tool);
	
	/*
	 * Set position: set up new subwindow (nsw) relative to previous
	 *	subwindow (psw) in the list.
	 *	If psw.width is default then assume that it extends to right
	 *	edge and start nsw below it and flush left.
	 *	Otherwise, put to the right of it.
	 */
	if (swprevious) {
		(void)win_getrect(swprevious->ts_windowfd, rect);
		rect->r_top -= yoffset;
		if (swprevious->ts_width==TOOL_SWEXTENDTOEDGE) {
			rect->r_top = rect->r_top+rect->r_height+
				tool_subwindowspacing(tool);
			rect->r_left = tool_borderwidth(tool);
		} else {
			rect->r_left = rect->r_left+rect->r_width+
				tool_subwindowspacing(tool);
		}
	} else {
		rect->r_top =
			tool_headerheight(tool->tl_flags & TOOL_NAMESTRIPE);
		rect->r_left = tool_borderwidth(tool);
	}
	/*
	 * Compute width & height
	 */
	rect->r_width = tool_swsizecompute(width,
	    toolrect.r_width-rect->r_left-tool_borderwidth(tool));
	rect->r_height = tool_swsizecompute(height,
	    toolrect.r_height-rect->r_top-tool_borderwidth(tool));
	rect->r_top += yoffset;
}

int
tool_headerheight(namestripe)
	int	namestripe;
{
	if (namestripe)  {
		return pf_sys->pf_defaultsize.y + 2;
	} else {
		return TOOL_BORDERWIDTH;
	}
}

short
tool_stripeheight(tool)
	struct	tool *tool;
{
	if (tool->tl_flags&TOOL_NAMESTRIPE)
		return(pf_sys->pf_defaultsize.y);
	else
		return(tool_borderwidth(tool));
}

/*ARGSUSED*/
short
tool_borderwidth(tool)
	struct	tool *tool;
{
	return(TOOL_BORDERWIDTH);
}

short
tool_subwindowspacing(tool)
	struct	tool *tool;
{
	return(tool_borderwidth(tool));
}

/*
 * Set size: set up new subwindow (nsw) size in available space (avs). 
 *	If avs is <=0 then set nsw to be requested amount.
 *	If avs is > 0 then set nsw = min(requested amount, avs).
 *	If nsw = default then use avs unless avs = 0 inwhich case
 *	choose some small constant.  We don't want 0 size.
 */
static int
tool_swsizecompute(request, avs)
	int	request, avs;
{
	int	nsw;

	if (request==TOOL_SWEXTENDTOEDGE) {
		if (avs<=0)
			nsw = 16;
		else
			nsw = avs;
	} else
		nsw = request;
	if (avs>0)
		nsw = min(nsw, avs);
	return(nsw);
}

