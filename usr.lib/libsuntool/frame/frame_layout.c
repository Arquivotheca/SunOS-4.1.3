#ifndef lint
#ifdef sccs
static        char sccsid[] = "@(#)frame_layout.c 1.1 92/07/30 Copyright 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 *  frame_layout: Layout a frame and its subwindows
 */

/* ------------------------------------------------------------------------- */

#include <stdio.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/file.h>

#include <pixrect/pixrect.h>

#include <sunwindow/notify.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/pixwin.h>
#include <sunwindow/win_struct.h>
#include <sunwindow/cms.h>
#include <sunwindow/win_screen.h>
#include <sunwindow/win_input.h>
#include <sunwindow/win_ioctl.h>		/* For WMGR_ICONIC */
#include "sunwindow/sv_malloc.h"

#include <suntool/tool_struct.h>
#include <suntool/window.h>
#include <suntool/wmgr.h>

#include "suntool/tool_impl.h"
#include "suntool/frame_impl.h"

/* ------------------------------------------------------------------------- */

/* 
 * Public
 */

extern int			win_getrect(), win_getsavedrect();
extern int			win_setrect(), win_setsavedrect();
extern struct toolsw		*tool_lastsw(), *tool_addsw();
extern				window_getrelrect();


/* 
 * Package private
 */

Pkg_private			frame_layout();


/* 
 * Private
 */

#define tool_find(tool, obj)	tool_find_prev((Tool *)(LINT_CAST((tool))), \
					(obj), FALSE)
Private Toolsw			*tool_find_prev();
Private				expand_sw();
Private Toolsw			*frame_find();
Private struct list_node	*frame_find_node();
Private void			grant_extend_to_edge();

/* ------------------------------------------------------------------------- */



/*ARGSUSED*/
/*VARARGS3*/
frame_layout(frame, win, op, d1, d2, d3, d4, d5)
	register Window frame, win;
	Window_layout_op op;
{   
    Rect rect, *rectp;
    int link;
    register int fd;
    char *malloc();
    struct list_node *node, *nnode;
    Window_type win_type = (Window_type)window_get(win, WIN_TYPE);
    int is_frame = win_type == FRAME_TYPE;
    int subframe = is_frame && frame != NULL;
    Tool *tool;
    Toolsw *tsw;
   
    frame = frame ? frame : win;
    tool = (Tool *)(LINT_CAST(frame));

   /* FIXME: TOOL_BORDERWIDTH should be a field of the frame */

    switch (op) {
	
      case WIN_CREATE:
	tsw = tool_lastsw((struct tool *)(LINT_CAST(frame)));
	fd = window_fd(win);
    
	link = win_fdtonumber(window_fd(frame)); /*  Setup links */
	(void)win_setlink(fd, WL_PARENT, link);
	if (tsw) {
	    link = win_fdtonumber(tsw->ts_windowfd);
	    (void)win_setlink(fd, WL_OLDERSIB, link);
	}
	(void)tool_positionsw((struct tool *)(LINT_CAST(frame)), tsw,
			TOOL_SWEXTENDTOEDGE, TOOL_SWEXTENDTOEDGE, &rect);
	(void)wmgr_setnormalrect(fd, &rect);
	break;
	
	
      case WIN_DESTROY:
	node = &((struct toolplus *)(LINT_CAST(frame)))->frame_list;
	nnode = NULL;
	for (; node->next; node = node->next)
	    if (node->next->client == (caddr_t)win) {
		nnode = node->next;
		node->next = node->next->next;
		break;
	    }
	if (!nnode) break; /* This shouldn''t happen but incompatible window
			    * types may call this twice. */

	/* depends on tsw being NULL if the subwindow has been destroyed */
	if (!is_frame && !(tool->tl_flags&TOOL_DESTROY)) {
	    tsw = nnode->tsw ? nnode->tsw : tool_find(tool, nnode->client);
	    if (tsw) (void)tool_destroysubwindow_inserted((struct tool *)(LINT_CAST(
	    	frame)), tsw, !nnode->tsw);
	}
	if (nnode) free((char *)(LINT_CAST(nnode)));
	break;

      case WIN_INSTALL:
	fd = window_fd(win);
	tsw = NULL;

	if (!is_frame) {
	    if (!window_get(win, WIN_COMPATIBILITY)) {
		tsw = (Toolsw *)tool_addsw((struct tool *)(LINT_CAST(frame)), 
				fd, (char *)(LINT_CAST(d1)), TOOL_SWEXTENDTOEDGE,
				TOOL_SWEXTENDTOEDGE);
		tsw->ts_data = win;		/* Remember client handle */
	    } else {
		tsw = tool_find(frame, win);	/* Find toolsw */
	    }
	}

	node = (struct list_node *)(LINT_CAST
		(sv_malloc(sizeof(struct list_node))));
	node->next = 0, node->client = win;
	nnode = &((struct toolplus *)(LINT_CAST(frame)))->frame_list;
	    
	for (; nnode->next; nnode = nnode->next) ; /* skip to the end */
	nnode->next = node;		
	node->tsw = 0;
	if (!window_get(win, WIN_SHOW) && !is_frame)
            frame_remove_from_tool(frame, win);

	if (window_get(win, WIN_SHOW) && !window_get(win, WIN_COMPATIBILITY))
	    (void)win_insert(fd);
	
	(void)tool_repaint_all_later((struct tool *)(LINT_CAST(frame)));
	break;
      
      case WIN_INSERT:
	fd = window_fd(win);
	(void)win_insert(fd);
	if (is_frame) break;
       /* 
        * Reinsert subwindow in toolsw linked list.
        */
	node = frame_find_node((struct toolplus *)(LINT_CAST(frame)), win);
	tsw = tool->tl_sw;
	if (tsw) {		/* Insert at the end of the list */
	    while (tsw->ts_next) tsw = tsw->ts_next;
	    tsw->ts_next = node->tsw;
	} else {
	    tool->tl_sw = node->tsw;
	}
	tsw = node->tsw;
	tsw->ts_next = node->tsw = NULL;

	if (tsw->ts_width == -1 || tsw->ts_height == -1) {
	    (void)win_getrect(fd, &rect);
	    expand_sw((Tool *)(LINT_CAST(frame)), tsw, &rect);
	    (void)win_setrect(fd, &rect);
	}

	(void)tool_repaint_all_later((struct tool *)(LINT_CAST(frame)));
	break;
      
      case WIN_REMOVE:
	(void)win_remove(window_fd(win));
	if (is_frame) break;
       /*
        * Remove subwindow from toolsw linked list to avoid confusing
        * the layout routines (tool boundary mgr, repaint, etc.)
        */
        frame_remove_from_tool(frame, win);

	(void)tool_repaint_all_later((struct tool *)(LINT_CAST(frame)));
	break;
	
      case WIN_LAYOUT:
	*(int *)d1 = TRUE;
	break;

    }
  
    /* Ignore 2nd info call */
    if (subframe && frame == win) return TRUE;
    
    switch (op) {
      
      case WIN_ADJUSTED:
	if (is_frame) break;

	(void)tool_repaint_all_later((struct tool *)(LINT_CAST(frame)));
	break;

      case WIN_ADJUST_BELOW:
	window_getrelrect(window_fd(win), window_fd((Window)d1), &rect);
	d1 = rect.r_top + rect.r_height + TOOL_BORDERWIDTH;

	if (!is_frame)
	    d1 -= tool_headerheight(tool->tl_flags & TOOL_NAMESTRIPE)
		+ tool_sw_iconic_offset(tool);
	else if (subframe)
	    (void)wmgr_getnormalrect(window_fd(frame), &rect), d1 -= rect.r_top;

	goto case_win_adjust_y;

      case WIN_ADJUST_HEIGHT:
	tsw = frame_find((struct toolplus *)(LINT_CAST(frame)), win);
	fd = window_fd(win);
	(void)wmgr_getnormalrect(fd, &rect);
	if (d1 == WIN_EXTEND_TO_EDGE && tsw) {
	    tsw->ts_height = (int)d1;
	    expand_sw((Tool *)(LINT_CAST(frame)), tsw, &rect);
	} else {
	    rect.r_height = d1;
	}
	(void)wmgr_setnormalrect(fd, &rect);
	if (tsw)
	    tsw->ts_height = (int)d1;
	else
	    grant_extend_to_edge((Tool *)(LINT_CAST(win)), FALSE);
	break;
      
      case WIN_ADJUST_RECT:
	tsw = frame_find((struct toolplus *)(LINT_CAST(frame)), win);
	fd = window_fd(win);

	if (!is_frame && tsw) {
	    ((Rect *)d1)->r_left += TOOL_BORDERWIDTH;
	    ((Rect *)d1)->r_top +=
		tool_headerheight(tool->tl_flags & TOOL_NAMESTRIPE)
		    + tool_sw_iconic_offset(tool);
	    tsw->ts_width =  ((Rect *)d1)->r_width;
	    tsw->ts_height = ((Rect *)d1)->r_height;
	    if (tsw->ts_width == WIN_EXTEND_TO_EDGE ||
		tsw->ts_height == WIN_EXTEND_TO_EDGE)
		expand_sw((Tool *)(LINT_CAST(frame)), tsw, (Rect *)d1);	    
	} else if (subframe) {
	    (void)wmgr_getnormalrect(window_fd(frame), &rect);
	    ((Rect *)d1)->r_left += rect.r_left;
	    ((Rect *)d1)->r_top += rect.r_top;
	}

	(void)wmgr_setnormalrect(fd, (Rect *)d1);
	break;
      
      case WIN_ADJUST_RIGHT_OF:
	window_getrelrect(window_fd(win), window_fd((Window)d1), &rect);
	d1 = rect.r_left + rect.r_width + TOOL_BORDERWIDTH;

	if (!is_frame) d1 -= TOOL_BORDERWIDTH;
	else if (subframe)
	    (void)wmgr_getnormalrect(window_fd(frame), &rect), d1 -= rect.r_left;
	
	goto case_win_adjust_x;

      case WIN_ADJUST_WIDTH:
	tsw = frame_find((struct toolplus *)(LINT_CAST(frame)), win);
	fd = window_fd(win);
	(void)wmgr_getnormalrect(fd, &rect);
	if (d1 == WIN_EXTEND_TO_EDGE && tsw) {
	    tsw->ts_width = (int)d1;
	    expand_sw((Tool *)(LINT_CAST(frame)), tsw, &rect);
	} else {
	    rect.r_width = d1;
	}

	(void)wmgr_setnormalrect(fd, &rect);

	if (tsw) tsw->ts_width = (int)d1;
	else grant_extend_to_edge((Tool *)(LINT_CAST(win)), TRUE);
	break;
      
      case WIN_ADJUST_X:
      case_win_adjust_x:
/*	if (d1 < 0) break;*/ /* Ignore negative offsets */
	tsw = frame_find((struct toolplus *)(LINT_CAST(frame)), win);
	fd = window_fd(win);

	if (!is_frame) d1 += TOOL_BORDERWIDTH;
	else if (subframe)
	    (void)wmgr_getnormalrect(window_fd(frame), &rect), d1 += rect.r_left;
	if (d1 < 0) break; /* Ignore negative offsets */

	(void)wmgr_getnormalrect(fd, &rect);
	rect.r_left = d1;

	if (tsw && (tsw->ts_width == -1 || tsw->ts_height == -1))
	    expand_sw((Tool *)(LINT_CAST(frame)), tsw, &rect);

	(void)wmgr_setnormalrect(fd, &rect);
	break;

      case WIN_ADJUST_Y:
      case_win_adjust_y:
/*	if (d1 < 0) break;*/ /* Ignore negative offsets */
	tsw = frame_find((struct toolplus *)(LINT_CAST(frame)), win);
	fd = window_fd(win);

	if (!is_frame)
	    d1 += tool_headerheight(tool->tl_flags & TOOL_NAMESTRIPE)
		+ tool_sw_iconic_offset(tool);
	else if (subframe)
	    (void)wmgr_getnormalrect(window_fd(frame), &rect), d1 += rect.r_top;
	if (d1 < 0) break; /* Ignore negative offsets */

	(void)wmgr_getnormalrect(fd, &rect);
	rect.r_top = d1;

	if (tsw && (tsw->ts_width == -1 || tsw->ts_height == -1))
	    expand_sw((Tool *)(LINT_CAST(frame)), tsw, &rect);

	(void)wmgr_setnormalrect(fd, &rect);
	break;

      case WIN_GET_X:
	/* Use d5 as a temp, a bit of a hack */
	(void)wmgr_getnormalrect(window_fd(win), &rect);
	d5 = rect.r_left - (is_frame ? 0 : TOOL_BORDERWIDTH);
	if (subframe) {
	    (void)wmgr_getnormalrect(window_fd(window_get(win, WIN_OWNER)), &rect);
	    d5 -= rect.r_left;
	}    
	*(int *)d1 = d5;
	break;
	
      case WIN_GET_Y:
	(void)wmgr_getnormalrect(window_fd(win), &rect);
	d5 = rect.r_top - (is_frame ? 0
			   : tool_headerheight(tool->tl_flags & TOOL_NAMESTRIPE)
			     + tool_sw_iconic_offset(tool));
	if (subframe) {
	    (void)wmgr_getnormalrect(window_fd(window_get(win, WIN_OWNER)), &rect);
	    d5 -= rect.r_top;
	}	    
	*(int *)d1 = d5;
	break;

      case WIN_GET_WIDTH:
	(void)wmgr_getnormalrect(window_fd(win), &rect);
	*(int *)d1 = rect.r_width;
	break;
	
      case WIN_GET_HEIGHT:
	(void)wmgr_getnormalrect(window_fd(win), &rect);
	*(int *)d1 = rect.r_height;
	break;
	
      case WIN_GET_RECT:
	rectp = (Rect *)d1;		/* Setup pointer arg */
	(void)wmgr_getnormalrect(window_fd(win), rectp);
	if (!is_frame) {
	    rectp->r_left -= TOOL_BORDERWIDTH;
	    rectp->r_top -= tool_headerheight(tool->tl_flags & TOOL_NAMESTRIPE)
		+ tool_sw_iconic_offset(tool);
	} else if (subframe) {
	    (void)wmgr_getnormalrect(window_fd(frame), &rect);
	    rectp->r_left -= rect.r_left;
	    rectp->r_top -= rect.r_top;
	}
	break;

      case WIN_CREATE: /* Ignore these -- already handled */
      case WIN_DESTROY:
      case WIN_INSTALL:
      case WIN_INSERT:
      case WIN_REMOVE:
      case WIN_LAYOUT:
	break;

      default:
	(void)fprintf(stderr,
   "frame_layout(internal error): frame layout option (%d) not recognized.\n",
		op);
	return FALSE;
	
    }
    return TRUE;
}


/*
 * Remove win from tool
 */
Private
frame_remove_from_tool(frame, win)
	register Window frame, win;
{
    struct list_node *node;
    Toolsw *tsw;
    Tool *tool;

    tool = (Tool *)(LINT_CAST(frame));
    tsw = tool_find_prev((Tool *)(LINT_CAST(frame)), win, TRUE);
    node = frame_find_node((struct toolplus *)(LINT_CAST(frame)), win);
    if (tsw) {
	node->tsw = tsw->ts_next;
	tsw->ts_next = tsw->ts_next->ts_next;
	if (((Toolsw_priv *)(LINT_CAST(node->tsw->ts_priv)))->have_kbd_focus) {
	    ((Toolsw_priv *)(LINT_CAST(node->tsw->ts_priv)))->have_kbd_focus = FALSE;
	    (void)win_set_kbd_focus(node->tsw->ts_windowfd, WIN_NULLLINK);
	}
    } else {
	node->tsw = tool->tl_sw;
	if (tool->tl_sw) tool->tl_sw = tool->tl_sw->ts_next;
    }
}


Private Toolsw *
tool_find_prev(tool, client, prev)
	Tool *tool;
	caddr_t	client;
	int/*bool*/ prev;
{   
    Toolsw *sw, *previous_sw = NULL;

    for (sw = tool->tl_sw;sw;sw = sw->ts_next) {
	if (sw->ts_data == client) break;
	previous_sw = sw;
    }	    
    return (prev && sw) ? previous_sw : sw;
}


/*
 * Convert secondary window rect into primary window space
 * SHOULD MOVE THIS TO THE win OR window DIRECTORY
 */
extern
window_getrelrect(pfd, sfd, srectp)
	int pfd, sfd;
	register Rect *srectp;
{
    Rect prect, *prectp = &prect, rect;
    int pwn, swn;
    char wname[WIN_NAMESIZE];
    
    (void)win_getrect(sfd, srectp);
    prect = rect_null;
    
    if (pfd == 0) {
	swn = win_getlink(sfd, WL_PARENT);
	while (swn != 0 && (swn != WIN_NULLLINK)) {
	    if (swn != WIN_NULLLINK) {
		(void)win_numbertoname(swn, wname);
		sfd = open(wname, O_RDONLY, 0);
		(void)win_getrect(sfd, &rect);
		srectp->r_top += rect.r_top, srectp->r_left += rect.r_left;
		swn = win_getlink(sfd, WL_PARENT);
		(void)close(sfd);
	    }
	}
    } else {
	pwn = win_getlink(pfd, WL_PARENT);
	swn = win_getlink(sfd, WL_PARENT);
	while (pwn != swn && (pwn != WIN_NULLLINK || swn != WIN_NULLLINK)) {
	    if (pwn != WIN_NULLLINK) {
		(void)win_numbertoname(pwn, wname);
		pfd = open(wname, O_RDONLY, 0);
		(void)win_getrect(pfd, &rect);
		prectp->r_top += rect.r_top, prectp->r_left += rect.r_left;
		pwn = win_getlink(pfd, WL_PARENT);
		(void)close(pfd);
	    }
	    if (swn != WIN_NULLLINK) {
		(void)win_numbertoname(swn, wname);
		sfd = open(wname, O_RDONLY, 0);
		(void)win_getrect(sfd, &rect);
		srectp->r_top += rect.r_top, srectp->r_left += rect.r_left;
		swn = win_getlink(sfd, WL_PARENT);
		(void)close(sfd);
	    }
	}
	srectp->r_top -= prectp->r_top, srectp->r_left -= prectp->r_left;
    }
}


Private
expand_sw(tool, sw, rectp)
	Tool *tool;
	Toolsw *sw;
	Rect *rectp;
{   
    Rect rect;
    
    (void)wmgr_getnormalrect(tool->tl_windowfd, &rect);

    if (sw->ts_width == TOOL_SWEXTENDTOEDGE)
	rectp->r_width = rect.r_width - TOOL_BORDERWIDTH - rectp->r_left;
    if (sw->ts_height == TOOL_SWEXTENDTOEDGE)
	rectp->r_height = rect.r_height - TOOL_BORDERWIDTH - 
	    (rectp->r_top - tool_sw_iconic_offset(tool));
    /* don't allow the subwindow to be less than 1 x 1 */
    if (rectp->r_width < 1)
        rectp->r_width = 1;
    if (rectp->r_height < 1)
        rectp->r_height = 1;
}


Private Toolsw *
frame_find(frame, client)
	struct toolplus *frame;
	caddr_t	client;
{   
    Toolsw *tsw;
    struct list_node *node;

    tsw = tool_find(frame, client);
    if (!tsw) {				      /* Maybe its a hidden subwindow */
	node = frame_find_node(frame, client); /* Intended to fail for frames */
	tsw = node ? node->tsw : NULL;
    }
    return tsw;
}


Private struct list_node *
frame_find_node(frame, client)
	struct toolplus *frame;
	caddr_t	client;
{   
    struct list_node *node;

    for (node = frame->frame_list.next; node; node = node->next) {
	if (node->client == client) return node;
    }	    
    return NULL;
}


/* make subwindows that border the frame
 * be extend-to-edge.
 */
Private void
grant_extend_to_edge(tool, to_right)
Tool		*tool;
register int	to_right;
{
    register Toolsw	*sw;
    register int	limit;
    Rect		rect;

    (void)tool_getnormalrect(tool, &rect);
    
    if (to_right)
	limit = rect.r_width - TOOL_BORDERWIDTH - 1;
    else
	limit = 
	    tool_sw_iconic_offset(tool) + rect.r_height - TOOL_BORDERWIDTH - 1;

    for (sw = tool->tl_sw; sw; sw = sw->ts_next) {
	(void)win_getrect(sw->ts_windowfd, &rect);
	if (to_right) {
	   if (rect_right(&rect) == limit)
	       sw->ts_width = TOOL_SWEXTENDTOEDGE;
	} else if (rect_bottom(&rect) == limit)
	       sw->ts_height = TOOL_SWEXTENDTOEDGE;
    }	    
}
