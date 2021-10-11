#ifndef lint
#ifdef sccs
static        char sccsid[] = "@(#)frame_get.c 1.1 92/07/30";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 *  frame_get: Gets frame attributes
 */

/* ------------------------------------------------------------------------- */

#include <stdio.h>

#include <sys/types.h>

#include <pixrect/pixrect.h>

#include <sunwindow/notify.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/pixwin.h>
#include <sunwindow/cms.h>
#include <sunwindow/win_screen.h>

#include <suntool/tool_struct.h>
#include <suntool/window.h>
#include "suntool/tool_impl.h"
#include "suntool/frame_impl.h"

/* ------------------------------------------------------------------------- */

/* 
 * Public
 */

extern int			win_getrect(), win_getsavedrect();
extern char			*tool_get_attribute();

/* 
 * Package private
 */

Pkg_private caddr_t		frame_get();


/* 
 * Private
 */


/* ------------------------------------------------------------------------- */


/* ARGSUSED */
/* VARARGS2 */
caddr_t
frame_get(win, attr, d1, d2, d3, d4, d5)
	Window win;
	Frame_attribute attr;
{
    register caddr_t v = NULL;
    register Tool *tool = (Tool *)(LINT_CAST(win));
    register Toolsw *tsw;
    struct toolplus *toolx;
    struct list_node *node;
    Rect rect, rbound;
    
    switch (attr) {
	
      case WIN_FIT_HEIGHT:
	rbound = rect_null;
	for (tsw = tool->tl_sw; tsw; tsw = tsw->ts_next) {
	    (void)win_getrect(tsw->ts_windowfd, &rect); 
	    if (tsw->ts_height != WIN_EXTEND_TO_EDGE) {
		rect.r_height += TOOL_BORDERWIDTH;
	    } else {
		rect.r_height = 16 + TOOL_BORDERWIDTH;
	    }
	    rbound = rect_bounding(&rbound, &rect);
	}
	if (!rbound.r_height)
	    (void)wmgr_getnormalrect(tool->tl_windowfd, &rbound);
	else
	    rbound.r_height +=
		tool_headerheight(tool->tl_flags & TOOL_NAMESTRIPE);
	v = (caddr_t)rbound.r_height;
	break;
	
      case WIN_FIT_WIDTH:
	rbound = rect_null;
	for (tsw = tool->tl_sw; tsw; tsw = tsw->ts_next) {
	    (void)win_getrect(tsw->ts_windowfd, &rect);
	    if (tsw->ts_width != WIN_EXTEND_TO_EDGE) {
		rect.r_width += TOOL_BORDERWIDTH;
	    } else {
		rect.r_width = 16 + TOOL_BORDERWIDTH;
	    }
	    rbound = rect_bounding(&rbound, &rect);
	}
	if (!rbound.r_width)
	    (void)wmgr_getnormalrect(tool->tl_windowfd, &rbound);
	else
	    rbound.r_width += TOOL_BORDERWIDTH;
	v = (caddr_t)rbound.r_width;
	break;
	
      case WIN_SHOW_UPDATES:
	v = (caddr_t)!(int)tool_get_attribute(tool, (int)(LINT_CAST(
		FRAME_REPAINT_LOCK)));
	break;
      
      case FRAME_BORDER_STYLE:
	v = (caddr_t)FRAME_DOUBLE;
	break;
	    
      case FRAME_CLOSED_RECT:
	{   
	    static Rect rect_local;
	    
	    if (window_get(win, WIN_OWNER)) {		/* Is a subframe */
		v = NULL;
		break;
	    }
	    
	    if (tool_is_iconic(tool))
		(void)win_getrect(tool->tl_windowfd, &rect_local);
	    else
		(void)win_getsavedrect(tool->tl_windowfd, &rect_local);
	    v = (caddr_t)&rect_local;
	}
	break;

      case FRAME_CURRENT_RECT:
	{   
	    static Rect rect_local;
	    
	    if (window_get(win, WIN_OWNER)) {		/* Is a subframe */
		v = window_get((Window)(LINT_CAST(tool)), WIN_RECT);
		break;
	    }
	    
	    (void)win_getrect(tool->tl_windowfd, &rect_local);
	    v = (caddr_t)&rect_local;
	}
	break;

      case FRAME_DEFAULT_DONE_PROC:
      case_frame_default_done_proc:
	v = (caddr_t)(LINT_CAST(
		((struct toolplus *)(LINT_CAST(win)))->default_done_proc));
	break;
      
      case FRAME_DONE_PROC:
	v = (caddr_t)(LINT_CAST(
		((struct toolplus *)(LINT_CAST(win)))->done_proc));
	if (!v) goto case_frame_default_done_proc;
	break;
	
      case FRAME_PROPS_ACTION_PROC:
	v = (caddr_t)(LINT_CAST(tool->props_proc));
	break;
	
      case FRAME_PROPS_ACTIVE:
        v = (caddr_t)(LINT_CAST(tool->props_active));
	break;

      case FRAME_EMBOLDEN_LABEL:
	v = (caddr_t)((tool->tl_flags&TOOL_EMBOLDEN_LABEL) != 0);
	break;	
	
      case FRAME_FOREGROUND_COLOR:
      case FRAME_BACKGROUND_COLOR:
	{   
	    static struct singlecolor color;
	    struct colormapseg cms;

	    (void)pw_getcmsdata(tool->tl_pixwin, &cms, (int *)0);
	    (void)pw_getcolormap(tool->tl_pixwin,
			   ((Frame_attribute)attr == FRAME_FOREGROUND_COLOR)
			   ? cms.cms_size-1: 0, 1,
			   &color.red, &color.green, &color.blue);
	    v = (caddr_t)&color;
	}
	break;

      case FRAME_ICON:
	v = (caddr_t)tool->tl_icon;
	break;
	
      case FRAME_LABEL:
	v = (caddr_t)tool->tl_name;
	break;
	
      case FRAME_NO_CONFIRM:
	v = (caddr_t)((tool->tl_flags & TOOL_NO_CONFIRM) != 0);
	break;

      case FRAME_NTH_WINDOW:
	toolx = (struct toolplus *)tool;
	node = toolx->frame_list.next;
	while (node && d1 > 0) {
	    node = node->next;
	    d1--;
	}
	if (node)
	    v = (caddr_t)node->client;
	break;

      case FRAME_NTH_SUBWINDOW: {
        int	n;
        
	toolx = (struct toolplus *)tool;
	for (node = toolx->frame_list.next, n = 0; node; node = node->next) {
	    if ((Window_type)window_get(node->client, WIN_TYPE) != FRAME_TYPE)
		if (n++ == d1)
		    break;
	}	    
	if (node) 
	    v = (caddr_t)node->client;
	break;
      }
      
      case FRAME_NTH_SUBFRAME: {
        int	n;
        
	toolx = (struct toolplus *)tool;
	for (node = toolx->frame_list.next, n = 0; node; node = node->next) {
	    if ((Window_type)window_get(node->client, WIN_TYPE) == FRAME_TYPE)
		if (n++ == d1)
		    break;
	}	    
	if (node) 
	    v = (caddr_t)node->client;
	break;
      }

      case FRAME_OPEN_RECT:
	{   
	    static Rect rect_local;
	    
	    if (window_get(win, WIN_OWNER)) {	/* Is a subframe */
		v = window_get((Window)(LINT_CAST(tool)), WIN_RECT);
		break;
	    }
	    
	    if (tool_is_iconic(tool))
		(void)win_getsavedrect(tool->tl_windowfd, &rect_local);
	    else
		(void)win_getrect(tool->tl_windowfd, &rect_local);
	    v = (caddr_t)&rect_local;
	}
	break;

      case FRAME_NULL_ATTR:
	break;
	
      case FRAME_CLOSED_X:			/* WIN_ICON_LEFT */
      case FRAME_CLOSED_Y:			/* WIN_ICON_RIGHT */
	if (window_get(win, WIN_OWNER)) break;	/* Is a subframe */

      case FRAME_CLOSED: 			/* WIN_ICONIC: */
	v = (caddr_t)tool_is_iconic(tool);
	break;

      case FRAME_INHERIT_COLORS:		/* WIN_DEFAULT_CMS */
      case FRAME_SHOW_LABEL:			/* WIN_NAME_STRIPE: */
      case FRAME_SUBWINDOWS_ADJUSTABLE:		/* WIN_BOUNDARY_MGR: */
	v = (caddr_t)tool_get_attribute(tool, (int)(LINT_CAST(attr)));
	break;
      case FRAME_SHADOW:
         toolx = (struct toolplus *)tool;
         v = (caddr_t)toolx->shadow;
         break;
      case FRAME_SHOW_SHADOW:
         toolx = (struct toolplus *)tool;
         v = (caddr_t)toolx->has_active_shadow;
         break;         
      case HELP_DATA:
         toolx = (struct toolplus *)tool;
         v = (caddr_t)toolx->help_data;
         break;         
      default:
	if (ATTR_PKG_FRAME == ATTR_PKG(attr))
	    (void)fprintf(stderr, "frame_get: Frame attribute not allowed.\n%s\n",
		    attr_sprint((char *)NULL, (unsigned)(LINT_CAST(attr))));
	break;

    }
    return v;
}
