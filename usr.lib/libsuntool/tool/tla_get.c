#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)tla_get.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 *  tla_get.c - Routines to query the value of tool attributes.
 */

#include <sys/types.h>
#include <sys/file.h>
#include <sys/time.h>
#include <stdio.h>
#include <pixrect/pixrect.h>
#include <pixrect/pixfont.h>
#include <pixrect/pr_util.h>
#include <pixrect/memvar.h>
#include <sunwindow/notify.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/cms.h>
#include <sunwindow/pixwin.h>
#include <sunwindow/win_input.h>
#include <sunwindow/win_screen.h>
#include <sunwindow/win_struct.h>
#include <suntool/icon.h>
#include <suntool/tool.h>
#include <suntool/tool_impl.h>

/* 
 * WARNING:  This code is being superseded by frame_get in frame.c.
 */

char *
tool_get_attribute(tool, attr)
	register struct tool *tool;
	register int attr;
{
#define	GET_FLAG(mask) ((tool->tl_flags & (mask))? (char *)1: (char *)0)
	register char *v;
	extern char *tool_copy_attr();

	switch (attr) {
	case WIN_COLUMNS:	{
		struct rect	r;

		(void)tool_getnormalrect(tool, &r);
		v = (char *) tool_columnsfromwidth(tool, r.r_width);
		break;
	}
	case WIN_LINES:	{
		struct rect	r;

		(void)tool_getnormalrect(tool, &r);
		v = (char *) tool_linesfromheight(tool, r.r_height);
		break;
	}
	case WIN_WIDTH:
	case WIN_HEIGHT:
	case WIN_LEFT:
	case WIN_TOP:
	case WIN_ICON_LEFT:
	case WIN_ICON_TOP: {
		struct rect irect, rect;

		if (tool_is_iconic(tool)) {
			(void)win_getrect(tool->tl_windowfd, &irect);
			(void)win_getsavedrect(tool->tl_windowfd, &rect);
		} else {
			(void)win_getrect(tool->tl_windowfd, &rect);
			(void)win_getsavedrect(tool->tl_windowfd, &irect);
		}
		switch (attr) {
		case WIN_WIDTH: v = (char *) rect.r_width; break;
		case WIN_HEIGHT: v = (char *) rect.r_height; break;
		case WIN_LEFT: v = (char *) rect.r_left; break;
		case WIN_TOP: v = (char *) rect.r_top; break;
		case WIN_ICON_LEFT: v = (char *) irect.r_left; break;
		case WIN_ICON_TOP: v = (char *) irect.r_top; break;
		}
		break;
		}
	case WIN_ICONIC: v = (char *) tool_is_iconic(tool); break;
	case WIN_DEFAULT_CMS: v = GET_FLAG(TOOL_DEFAULT_CMS); break;
	case WIN_REPAINT_LOCK: v = GET_FLAG(TOOL_REPAINT_LOCK); break;
	case WIN_LAYOUT_LOCK: v = GET_FLAG(TOOL_LAYOUT_LOCK); break;
	case WIN_NAME_STRIPE: v = GET_FLAG(TOOL_NAMESTRIPE); break;
	case WIN_BOUNDARY_MGR: v = GET_FLAG(TOOL_BOUNDARYMGR); break;
	case WIN_LABEL: v = tool_copy_attr(attr, tool->tl_name); break;
	case WIN_FOREGROUND:
	case WIN_BACKGROUND: {
		struct singlecolor color;
		struct colormapseg cms;

		(void)pw_getcmsdata(tool->tl_pixwin, &cms, (int *)0);
		(void)pw_getcolormap(tool->tl_pixwin,
		    ((Win_attribute)attr == WIN_FOREGROUND)? cms.cms_size-1: 0, 1,
		    &color.red, &color.green, &color.blue);
		v = tool_copy_attr(attr, (char *)(LINT_CAST(&color)));
		break;
		}
	case WIN_ICON: v = tool_copy_attr(attr, (char *)(LINT_CAST(
		tool->tl_icon))); break;
	case WIN_ICON_LABEL:
		if (tool->tl_icon)
			v = tool_copy_attr(attr, (char *)(LINT_CAST(
				tool->tl_icon->ic_text)));
		else
			v = NULL;
		break;
	case WIN_ICON_IMAGE:
		if (tool->tl_icon)
			v = tool_copy_attr(attr, (char *)(LINT_CAST(
				tool->tl_icon->ic_mpr)));
		else
			v = NULL;
		break;
	case WIN_ICON_FONT:
		if (tool->tl_icon)
			v = tool_copy_attr(attr, (char *)(LINT_CAST(
				tool->tl_icon->ic_font)));
		else
			v = NULL;
		break;
	case WIN_ATTR_LIST:
		return((char *)-1);
	default:
		return((char *)-1);
	}
	return(v);
}

