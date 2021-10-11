#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)tla_set.c 1.1     92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 *  tla_set.c - Set attributes of tool.
 */

#include <sys/types.h>
#include <sys/file.h>
#include <sys/time.h>
#include <stdio.h>
#include <varargs.h>
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

/* VARARGS1 */
int
tool_set_attributes(tool, va_alist)
	struct tool *tool;
	va_dcl
{
	va_list	valist;
	caddr_t avlist[ATTR_STANDARD_SIZE];

	va_start(valist);
	(void) attr_make(avlist,ATTR_STANDARD_SIZE,valist);
	va_end(valist);
	return(tool_set_attr(tool, avlist));
}

tool_set_attr(tool, avlist)
register struct tool *tool;
Attr_avlist avlist;
{
#define	SET_FLAG(bool, flags, mask) \
			if ((bool)) (flags) |= mask; \
			else (flags) &= ~mask;
	void wmgr_close(),wmgr_open();
	register char **args;
	register int attr;
	register char *v1;
	struct rect irect, irect_start, rect, rect_start;
	short flags, flags_start;
	char *label, *label_start;
	struct singlecolor bg_struct, fg_struct;
	struct singlecolor *bg = &bg_struct, *bg_start = &bg_struct;
	struct singlecolor *fg = &fg_struct, *fg_start = &fg_struct;
	struct colormapseg cms;
	struct icon *icon, *icon_start;
	char *icon_label, *icon_label_start;
	struct pixrect *icon_image, *icon_image_start;
	PIXFONT *icon_font, *icon_font_start;
	int repaint_icon = 0, auto_repaint = 0, repaint_name_stripe = 0,
	    repaint_tool_only = 0;
	enum height_state {virgin=0, internal=1, absolute=2};
	enum height_state height_state = virgin;
	int was_iconic = tool_is_iconic(tool);

	/*
	 * Flatten out attribute list.
	 */
	if (*avlist == NULL)
		return (0);
	if (avlist == NULL)
		return(-1);
	/*
	 * Remember initial state.
	 */
	if (was_iconic) {
		(void)win_getrect(tool->tl_windowfd, &irect);
		(void)win_getsavedrect(tool->tl_windowfd, &rect);
	} else {
		(void)win_getrect(tool->tl_windowfd, &rect);
		(void)win_getsavedrect(tool->tl_windowfd, &irect);
	}
	irect_start = irect;
	rect_start = rect;
	flags_start = tool->tl_flags;
	SET_FLAG(was_iconic, flags_start, TOOL_ICONIC);
	flags = flags_start;
	label_start = label = (char *)tool->tl_name;
	(void)pw_getcmsdata(tool->tl_pixwin, &cms, (int *)0);
	(void)pw_getcolormap(tool->tl_pixwin, 0, 1, &bg->red, &bg->green, &bg->blue);
	(void)pw_getcolormap(tool->tl_pixwin, 1, 1, &fg->red, &fg->green, &fg->blue);
	/* Note: TBD Need to have icon allocated in tool_create */
	icon_start = icon = tool->tl_icon;
	if (icon) {
		icon_label_start = icon_label = icon->ic_text;
		icon_image_start = icon_image = icon->ic_mpr;
		icon_font_start = icon_font = icon->ic_font;
	} else {
		icon_label_start = icon_label = NULL;
		icon_image_start = icon_image = (struct pixrect *)0;
		icon_font_start = icon_font = (struct pixfont *)0;
	}
	/*
	 * Determine ways changed
	 */
	for (args = avlist;*args;args = attr_next(args)) {
		/* Note: TBD check number of args expected here */
		attr = (int) args[0];
		v1 = args[1];
		switch (attr) {
		case WIN_COLUMNS:
			rect.r_width = tool_widthfromcolumns((int)(LINT_CAST(v1)));
			break;
		case WIN_LINES:
			rect.r_height = tool_heightfromlines((int)(LINT_CAST(v1)),
					    tool->tl_flags&TOOL_NAMESTRIPE);
			height_state = internal;
			break;
		case WIN_WIDTH:
			rect.r_width = (u_int)v1;
			break;
		case WIN_HEIGHT:
			rect.r_height = (u_int)v1;
			height_state = absolute;
			break;
		case WIN_LEFT: rect.r_left = (u_int)v1; break;
		case WIN_TOP: rect.r_top = (u_int)v1; break;
		case WIN_ICONIC:
			SET_FLAG(v1, flags, TOOL_ICONIC);
			break;
		case WIN_DEFAULT_CMS:
			SET_FLAG(v1, flags, TOOL_DEFAULT_CMS);
			break;
		case WIN_REPAINT_LOCK:
			SET_FLAG(v1, flags, TOOL_REPAINT_LOCK);
			break;
		case WIN_LAYOUT_LOCK:
			SET_FLAG(v1, flags, TOOL_LAYOUT_LOCK);
			break;
		case WIN_NAME_STRIPE:
			SET_FLAG(v1, flags, TOOL_NAMESTRIPE);
			break;
		case WIN_BOUNDARY_MGR:
			SET_FLAG(v1, flags, TOOL_BOUNDARYMGR);
			break;
		case WIN_LABEL:
			label = tool_copy_attr(attr, v1);
			break;
		case WIN_FOREGROUND:
			fg = (struct singlecolor *) 
				(LINT_CAST(tool_copy_attr(attr, v1)));
			break;
		case WIN_BACKGROUND:
			bg = (struct singlecolor *) 
				(LINT_CAST(tool_copy_attr(attr, v1)));
			break;
		case WIN_ICON:
			icon = (struct icon *) 
				(LINT_CAST(tool_copy_attr(attr, v1)));
			break;
		case WIN_ICON_LEFT: irect.r_left = (u_int)v1; break;
		case WIN_ICON_TOP: irect.r_top = (u_int)v1; break;
		case WIN_ICON_LABEL:
			icon_label = tool_copy_attr(attr, v1);
			break;
		case WIN_ICON_IMAGE:
			icon_image = (struct pixrect *)
				(LINT_CAST(tool_copy_attr(attr, v1)));
			break;
		case WIN_ICON_FONT:
			icon_font = (PIXFONT *) 
				(LINT_CAST(tool_copy_attr(attr, v1)));
			break;
		case WIN_ATTR_LIST:
			/* Free all storage ?? */
			goto Error;
		}
	}
	/*
	 * Apply changes.
	 */
	/* New icon? */
	if (icon != icon_start) {
		(void)tool_free_attr((int)(LINT_CAST(WIN_ICON)), 
				(char *)(LINT_CAST(icon_start)));
		tool->tl_icon = icon;
		irect.r_width = icon->ic_width;
		irect.r_height = icon->ic_height;
		repaint_icon = 1;
	}
	/* New icon contents (font, image, text)? */
	if (tool->tl_icon) {
		if (icon_label != icon_label_start) {
			(void)tool_free_attr((int)(LINT_CAST(WIN_ICON_LABEL)), 
				(char *)(LINT_CAST(tool->tl_icon->ic_text)));
			tool->tl_icon->ic_text = icon_label;
			repaint_icon = 1;
		}
		if (icon_font != icon_font_start) {
			(void)tool_free_attr((int)(LINT_CAST(WIN_ICON_FONT)), 
				(char *)(LINT_CAST(tool->tl_icon->ic_font)));
			tool->tl_icon->ic_font = icon_font;
			repaint_icon = 1;
		}
		if (icon_image != icon_image_start) {
			(void)tool_free_attr((int)(LINT_CAST(WIN_ICON_IMAGE)), 
				(char *)(LINT_CAST(tool->tl_icon->ic_mpr)));
			tool->tl_icon->ic_mpr    = icon_image;
			tool->tl_icon->ic_width =
			tool->tl_icon->ic_gfxrect.r_width =
						icon_image->pr_size.x;
			tool->tl_icon->ic_height =
			tool->tl_icon->ic_gfxrect.r_height =
						icon_image->pr_size.y;
			repaint_icon = 1;
		}
	}
	/* New rects? */
	if (((flags & TOOL_NAMESTRIPE) != (flags_start & TOOL_NAMESTRIPE)) &&
	    height_state != absolute) {
		register int	n;
		struct	toolsw *sw;
		Rect swrect;
		
		n = tool_headerheight(TRUE) - TOOL_BORDERWIDTH;
		if (!(flags & TOOL_NAMESTRIPE)) n = -n;
		rect.r_height += n;

		/* Relocate sw's (tool_layout doesn't do this anymore) */
		for (sw = tool->tl_sw; sw; sw = sw->ts_next) {
		    (void)win_getrect(sw->ts_windowfd, &swrect);
		    swrect.r_top += n;
		    (void)win_setrect(sw->ts_windowfd, &swrect);
		}
	    }
	if (!rect_equal(&rect, &rect_start)) {
		if (was_iconic)
			(void)win_setsavedrect(tool->tl_windowfd, &rect);
		else {
			(void)win_setrect(tool->tl_windowfd, &rect);
			auto_repaint = 1;
		}
	}
	if (!rect_equal(&irect, &irect_start)) {
		if (was_iconic) {
			(void)win_setrect(tool->tl_windowfd, &irect);
			auto_repaint = 1;
		} else {
			(void)win_setsavedrect(tool->tl_windowfd, &irect);
		}
	}
	/* New colors? */
	(void)tool_setgroundcolor(tool,
	    (fg != fg_start)? fg: (struct singlecolor *)0,
	    (bg != bg_start)? bg: (struct singlecolor *)0,
	    flags & TOOL_DEFAULT_CMS);
	if ((fg != fg_start || bg != bg_start) &&
	    (tool->tl_pixwin->pw_pixrect->pr_depth == 1))
		repaint_tool_only = 1;
	if (fg != fg_start)
		(void)tool_free_attr((int)(LINT_CAST(WIN_FOREGROUND)), 
				(char *)(LINT_CAST(fg)));
	if (bg != bg_start)
		(void)tool_free_attr((int)(LINT_CAST(WIN_BACKGROUND)), 
				(char *)(LINT_CAST(bg)));
	/* New flags? */
	if (((flags & TOOL_ICONIC) && !was_iconic) ||
	    ((~flags & TOOL_ICONIC) && was_iconic)) {
		char name[WIN_NAMESIZE];
		int rootfd;

		(void)win_numbertoname(win_getlink(tool->tl_windowfd, WL_PARENT),
		    name);
		if ((rootfd = open(name, O_RDONLY, 0)) < 0) {
			/* Note: TBD print message? */
			goto Error;
		}
		if ((flags & TOOL_ICONIC) && !was_iconic) {
			wmgr_close(tool->tl_windowfd, rootfd);
			auto_repaint = 1;
		} else {
			wmgr_open(tool->tl_windowfd, rootfd);
			auto_repaint = 1;
		}
		(void)close(rootfd);
	}
	/* TOOL_ICONIC will get changed as state changes in handle sigwinch */
	tool->tl_flags = (flags & ~TOOL_ICONIC) | (tool->tl_flags & TOOL_ICONIC);
	/* New label? */
	if (label != label_start) {
		(void)tool_free_attr((int)(LINT_CAST(WIN_LABEL)), 
				(char *)(LINT_CAST(label_start)));
		(char *)tool->tl_name = label;
		repaint_name_stripe = 1;
	}
	/* Layout subwindows if needed */
	if (((flags & TOOL_LAYOUT_LOCK) != (flags_start & TOOL_LAYOUT_LOCK)) &&
	    ((~(tool->tl_flags)) & TOOL_LAYOUT_LOCK))
		(void)tool_layoutsubwindows(tool);
	/* Repaint if needed */
	if ((~(tool->tl_flags) & TOOL_REPAINT_LOCK) && !auto_repaint) {
		if (was_iconic && repaint_icon)
			(void)tool_displayicon(tool);
		else if (!was_iconic && repaint_name_stripe)
			(void)tool_displaynamestripe(tool);
		else if (!was_iconic && repaint_tool_only)
			(void)tool_displaytoolonly(tool);
		else if ((flags & TOOL_REPAINT_LOCK) !=
		    (flags_start & TOOL_REPAINT_LOCK))
			(void)tool_display(tool);
	}
	return(0);
Error:
	return(-1);
}

/*ARGSUSED*/
tool_cmsname(tool, name)
	struct tool *tool;
	char *name;
{
	extern char *sprintf();
	static tool_cms_id;
	
	(void)sprintf(name, "tool%ld-%ld", getpid(), ++tool_cms_id);
}

tool_setgroundcolor(tool, foreground, background, makedefault)
	struct tool *tool;
	register struct singlecolor *foreground;
	register struct singlecolor *background;
	int makedefault;
{
	char cmsname[CMS_NAMESIZE];
	u_char red[256], green[256], blue[256];
	struct pixwin *pw = tool->tl_pixwin;
	struct colormapseg cms;
	struct cms_map map;

	(void)pw_getcmsdata(pw, &cms, (int *)0);
	if (cms.cms_size < 1 || cms.cms_size > 256)
		return(-1);
	(void)pw_getcolormap(pw, 0, cms.cms_size, red, green, blue);
	if (foreground) {
		red[cms.cms_size-1] = foreground->red;
		green[cms.cms_size-1] = foreground->green;
		blue[cms.cms_size-1] = foreground->blue;
	}
	if (background) {
		red[0] = background->red;
		green[0] = background->green;
		blue[0] = background->blue;
	}
	if (foreground || background) {
		if (strncmp("tool", pw->pw_cmsname, 4) != 0) {
			(void)tool_cmsname(tool, cmsname);
			(void)pw_setcmsname(pw, cmsname);
		}
		(void)pw_putcolormap(pw, 0, cms.cms_size, red, green, blue);
	}
	if (makedefault) {
		(void)pw_getcmsname(pw, &cms.cms_name[0]);
		map.cm_red = red;
		map.cm_green = green;
		map.cm_blue = blue;
		(void)pw_setdefaultcms(&cms, &map);
	}
	return(0);
}
