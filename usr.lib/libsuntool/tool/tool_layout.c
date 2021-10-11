#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)tool_layout.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Layout of subwindows in tool routines.
 *
 * NOTE: Clients of sunwindows are supposed to be able to provide there own
 *	 copy of this tool_layoutsubwindows.  This has two implications:
 *	 1) This tool_layoutsubwindows must be in its own module.
 *	 2) This tool_layoutsubwindows cannot use private implementation .h's.
 */

#include <sys/types.h>
#include <sunwindow/rect.h>
#include <suntool/tool.h>

/* some parts of the system cannot deal with
 * zero sized subwindows.  So, when we must
 * cut the size of a subwindow to less than 
 * this constant, we use this constant.
 * This will cause a loss of tool borders.
 */
#define	MIN_SW_SIZE	1
/*
 * Layout subwindows in tool.
 * Here we preserve extend-to-edgeness of the subwindows: if a
 * subwindow's width or height is extend-to-edge, the corresponding value
 * in the subwindows's rect is extended to the border of the tool.
 * If the subwindow crosses the edge of the tool, cut back the width/height.
 * If the subwindow is not as big as it should be (ts_width/height),
 * expand the subwindow to tile the tool.
 */
tool_layoutsubwindows(tool)
	struct	tool *tool;
{
	register Toolsw	*sw;
	Rect		rect, old_rect, rconstrain;
	register short	border_width = tool_borderwidth(tool);
	short  		swsp = tool_subwindowspacing(tool) / 2;
	register short	right, bottom, outer_right, outer_bottom, avs;
	register short	yoffset;
	short		hole, need_constraint;
	void tool_compute_constraint();

	if (tool->tl_flags & TOOL_LAYOUT_LOCK)
		return;
		
	/*
	 * Iconic adjustments
	 */
	(void)tool_getnormalrect(tool, &rect);
	yoffset = tool_sw_iconic_offset(tool);
	
	outer_right = rect.r_width;
	right = outer_right - border_width - 1;
	outer_bottom = rect.r_height;
	bottom = outer_bottom - border_width - 1;

	(void)win_lockdata(tool->tl_windowfd);
	
	for (sw = tool->tl_sw; sw; sw = sw->ts_next) {

	    (void)win_getrect(sw->ts_windowfd, &rect);
	    rect.r_top -= yoffset;
	    old_rect = rect;
	    need_constraint = TRUE;

	    /* adjust the width */
	    if (rect.r_left < outer_right) {
		avs = right - rect.r_left + 1;
		if (avs < MIN_SW_SIZE)
		    avs = MIN_SW_SIZE;
		if (sw->ts_width == TOOL_SWEXTENDTOEDGE)
		    /* preserve extend-to-edgeness */
		    rect.r_width = avs;
		else if (rect_right(&rect) >= right)
		    /* cut the width back to save the border */
		    rect.r_width = min(avs, sw->ts_width);
		else if (rect.r_width < sw->ts_width) {
		    /* try to expand the width to desired width */
		    tool_compute_constraint(tool, sw, &rconstrain);
		    need_constraint = FALSE;
		    rect_marginadjust(&rect, swsp);
		    hole = rect_right(&rconstrain) - rect_right(&rect);
		    if (hole > 0)
			rect.r_width += hole;
		    rect_marginadjust(&rect, -swsp);
		    if (rect.r_width > sw->ts_width)
			rect.r_width = sw->ts_width;
		}
	    }
		    
	    /* adjust the height */
	    if (rect.r_top < outer_bottom) {
		avs = bottom - rect.r_top + 1;
		if (avs < MIN_SW_SIZE)
		    avs = MIN_SW_SIZE;
		if (sw->ts_height == TOOL_SWEXTENDTOEDGE)
		    /* preserve extend-to-edgeness */
		    rect.r_height = avs;
		else if (rect_bottom(&rect) >= bottom)
		    /* cut the height back to save the border */
		    rect.r_height = min(avs, sw->ts_height);
		else if (rect.r_height < sw->ts_height) {
		    /* try to expand the height to desired height */
		    if (need_constraint) {
		        tool_compute_constraint(tool, sw, &rconstrain);
			rconstrain.r_top -= yoffset;
		    }
		    rect_marginadjust(&rect, swsp);
		    hole = rect_bottom(&rconstrain) - rect_bottom(&rect);
		    if (hole > 0)
			rect.r_height += hole;
		    rect_marginadjust(&rect, -swsp);
		    if (rect.r_height > sw->ts_height)
			rect.r_height = sw->ts_height;
		}
	    }
		    
	    if (!rect_equal(&rect, &old_rect)) {
		rect.r_top += yoffset;
		(void)win_setrect(sw->ts_windowfd, &rect);
	    }
	}

	(void)win_unlockdata(tool->tl_windowfd);
	return;
}
