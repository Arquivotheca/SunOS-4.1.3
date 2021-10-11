#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)tla_storage.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 *  tla_storage.c - Basic free, copy, count tool attribute operations.
 */

#include <stdio.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/time.h>
#include <pixrect/pixrect.h>
#include <pixrect/pixfont.h>
#include <pixrect/pr_util.h>
#include <pixrect/memvar.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/cms.h>
#include <sunwindow/pixwin.h>
#include <sunwindow/win_input.h>
#include <sunwindow/win_screen.h>
#include <sunwindow/win_struct.h>
#include "sunwindow/sv_malloc.h"
#include <suntool/icon.h>
#include <suntool/tool.h>

int	tool_debug_attr = 1;

tool_free_attribute(attr, v)
	int attr;
	char *v;
{
	(void)tool_free_attr(attr, v);
}

int
tool_card_attr(attr)
	int attr;
{
	switch (attr) {
	case WIN_COLUMNS:
	case WIN_LINES:
	case WIN_WIDTH:
	case WIN_HEIGHT:
	case WIN_LEFT:
	case WIN_TOP:
	case WIN_ICON_LEFT:
	case WIN_ICON_TOP:
	case WIN_ICONIC:
	case WIN_DEFAULT_CMS:
	case WIN_REPAINT_LOCK:
	case WIN_LAYOUT_LOCK:
	case WIN_NAME_STRIPE:
	case WIN_BOUNDARY_MGR:
	case WIN_LABEL:
	case WIN_FOREGROUND:
	case WIN_BACKGROUND:
	case WIN_ICON_LABEL:
	case WIN_ICON:
	case WIN_ICON_IMAGE:
	case WIN_ICON_FONT:
		return(1);
	case 0:
		return(0);
	default:
		return(-1);
	}
}

tool_free_attr(attr, v)
	int attr;
	register char *v;
{
	switch (attr) {
	case WIN_COLUMNS:
	case WIN_LINES:
	case WIN_WIDTH:
	case WIN_HEIGHT:
	case WIN_LEFT:
	case WIN_TOP:
	case WIN_ICON_LEFT:
	case WIN_ICON_TOP:
	case WIN_ICONIC:
	case WIN_DEFAULT_CMS:
	case WIN_REPAINT_LOCK:
	case WIN_LAYOUT_LOCK:
	case WIN_NAME_STRIPE:
	case WIN_BOUNDARY_MGR:
		break;
	case WIN_LABEL:
	case WIN_FOREGROUND:
	case WIN_BACKGROUND:
	case WIN_ICON_LABEL:
		if (v)
			free(v);
		break;
	case WIN_ICON: {
		struct icon *icon = (struct icon *)(LINT_CAST(v));

		if (icon) {
			(void)tool_free_attr((int)(LINT_CAST(WIN_ICON_IMAGE)), 
					(char *)(LINT_CAST(icon->ic_background)));
			(void)tool_free_attr((int)(LINT_CAST(WIN_ICON_IMAGE)), 
					(char *)(LINT_CAST(icon->ic_mpr)));
			(void)tool_free_attr((int)(LINT_CAST(WIN_ICON_FONT)), 
					(char *)(LINT_CAST(icon->ic_font)));
			(void)tool_free_attr((int)(LINT_CAST(WIN_ICON_LABEL)), 
					(char *)(LINT_CAST(icon->ic_text)));
			free(v);
		}
		break;
		}
	case WIN_ICON_IMAGE:
		if (v)
			(void)pr_destroy((struct pixrect *)(LINT_CAST(v)));
		break;
	case WIN_ICON_FONT:
		/*
		 * Note: Treating WIN_ICON_FONT's value as static until get
		 * reasonable font manager that handles reference counting.
		 * If kept changing icon font using the interface (assuming
		 * that old one freed) then could waste space in a BIG way.
		 */
		break;
	default:
		if (tool_debug_attr)
			(void)fprintf(stderr, "tool_free_attr: unknown attr %ld\n",
			    attr);
	}
}

char *
tool_copy_attr(attr, v)
	int attr;
	register char *v;
{
	register char *vr = NULL;
	extern char *malloc();

	switch (attr) {
	case WIN_COLUMNS:
	case WIN_LINES:
	case WIN_WIDTH:
	case WIN_HEIGHT:
	case WIN_LEFT:
	case WIN_TOP:
	case WIN_ICON_LEFT:
	case WIN_ICON_TOP:
	case WIN_ICONIC:
	case WIN_DEFAULT_CMS:
	case WIN_REPAINT_LOCK:
	case WIN_LAYOUT_LOCK:
	case WIN_NAME_STRIPE:
	case WIN_BOUNDARY_MGR:
		vr = v;
		break;
	case WIN_LABEL:
	case WIN_ICON_LABEL:
		if (v) {
			vr = sv_malloc((unsigned)(LINT_CAST(strlen(v)+1)));
			(void)strcpy(vr, v);
		}
		break;
	case WIN_FOREGROUND:
	case WIN_BACKGROUND:
		if (v) {
			vr = sv_malloc(sizeof(struct singlecolor));
			bcopy(v, vr, sizeof(struct singlecolor));
		}
		break;
	case WIN_ICON: {
		struct icon *icon = (struct icon *)(LINT_CAST(v)), *newicon;

		if (icon) {
			vr = sv_malloc(sizeof(struct icon));
			newicon = (struct icon *)(LINT_CAST(vr));
			bcopy(v, vr, sizeof(struct icon));
			newicon->ic_background = (struct pixrect *)(LINT_CAST(
			    tool_copy_attr((int)(LINT_CAST(WIN_ICON_IMAGE)), 
			    		(char *)(LINT_CAST(icon->ic_background)))));
			newicon->ic_mpr = (struct pixrect *)(LINT_CAST(
			    tool_copy_attr((int)(LINT_CAST(WIN_ICON_IMAGE)), 
			    		(char *)(LINT_CAST(icon->ic_mpr)))));
			newicon->ic_font = (PIXFONT *)(LINT_CAST(
			    tool_copy_attr((int)(LINT_CAST(WIN_ICON_FONT)), 
			    		(char *)(LINT_CAST(icon->ic_font)))));
			newicon->ic_text = 
			    tool_copy_attr((int)(LINT_CAST(WIN_ICON_LABEL)), 
			    		(char *)(LINT_CAST(icon->ic_text)));
		}
		break;
		}
	case WIN_ICON_IMAGE: {
		struct pixrect *mpr = (struct pixrect *)(LINT_CAST(v)), *nmpr;

		if (mpr) {
			nmpr = mem_create(mpr->pr_width, mpr->pr_height,
			    mpr->pr_depth);
			vr = (char *)(LINT_CAST(nmpr));
			((struct mpr_data *)(LINT_CAST(nmpr->pr_data)))->md_flags =
			  ((struct mpr_data *)(LINT_CAST(mpr->pr_data)))->md_flags;
			(void)pr_rop(nmpr, 0, 0, mpr->pr_width, mpr->pr_height,
			    PIX_SRC, mpr, 0, 0);
		}
		break;
		}
	case WIN_ICON_FONT:
		/*
		 * Note: Treating WIN_ICON_FONT's value as static until get
		 * reasonable font manager that handles reference counting.
		 * If kept changing icon font using the interface (assuming
		 * that old one freed) then could waste space in a BIG way.
		 */
		vr = v;
		break;
	default:
		if (tool_debug_attr)
			(void)fprintf(stderr, "tool_free_attr: unknown attr %ld\n",
			    attr);
	}
	return(vr);
}

