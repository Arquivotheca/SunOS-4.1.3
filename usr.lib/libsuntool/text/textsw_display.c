#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)textsw_display.c 1.1 92/07/30";
#endif
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Initialization and finalization of text subwindows.
 */

#include <suntool/primal.h>
#include <suntool/textsw_impl.h>
#include <sunwindow/win_notify.h>

extern void
textsw_display(abstract)
	Textsw			abstract;
{
	register Textsw_view	view = VIEW_ABS_TO_REP(abstract);
	Textsw_folio		textsw = FOLIO_FOR_VIEW(view);

	textsw->state |= TXTSW_DISPLAYED;
	FORALL_TEXT_VIEWS(textsw, view) {
	    textsw_display_view(VIEW_REP_TO_ABS(view), &view->rect);
	}
}

extern void
textsw_display_view(abstract, rect)
	Textsw			 abstract;
	register Rect		*rect;
{
	register Textsw_view	 view = VIEW_ABS_TO_REP(abstract);

	scrollbar_paint_clear(SCROLLBAR_FOR_VIEW(view));
	textsw_display_view_margins(view, rect);
	if (rect == 0) {
	    rect = &view->rect;
	} else if (!rect_intersectsrect(rect, &view->rect)) {
	    return;
	}
	
	ev_display_in_rect(view->e_view, rect);
	textsw_update_scrollbars(FOLIO_FOR_VIEW(view), view);

}

pkg_private void
textsw_display_view_margins(view, rect)
	register Textsw_view	 view;
	struct rect		*rect;
{
	struct rect		 margin;

	margin = view->e_view->rect;
	margin.r_left -= (
	    margin.r_width = (int)ev_get(view->e_view, EV_LEFT_MARGIN));
	if (rect == 0 || rect_intersectsrect(rect, &margin)) {
	    (void) pw_writebackground(PIXWIN_FOR_VIEW(view),
				      margin.r_left, margin.r_top,
				      margin.r_width, margin.r_height,
				      PIX_SRC);
	}
	margin.r_left = rect_right(&view->e_view->rect) + 1;
	margin.r_width = (int)ev_get(view->e_view, EV_RIGHT_MARGIN);
	if (rect == 0 || rect_intersectsrect(rect, &margin)) {
	    (void) pw_writebackground(PIXWIN_FOR_VIEW(view),
				      margin.r_left, margin.r_top, 
				      margin.r_width, margin.r_height,
				      PIX_SRC);
	}
}

pkg_private void
textsw_repaint(view)
	register Textsw_view	 view;
{     	

/*	
 *	This was used for eliminating unnecessary repaint on upper split
 	if (!(view->state & TXTSW_VIEW_DISPLAYED)) {
		view->state |= TXTSW_VIEW_DISPLAYED;
		view->state |= TXTSW_UPDATE_SCROLLBAR;
	} 
*/	 
	
	FOLIO_FOR_VIEW(view)->state |= TXTSW_DISPLAYED;      

	(void) win_set_flags((char *)view,
				(unsigned) win_get_flags((char *)view) &
				(~PW_REPAINT_ALL));
	textsw_display_view(VIEW_REP_TO_ABS(view), &view->rect);
}

pkg_private void
textsw_resize(view)
	register Textsw_view	view;
{
	register int		delta_height;
	Rect			old_rect;
	Rect			new_rect, *sb_rect;
	Scrollbar		sb = SCROLLBAR_FOR_VIEW(view);
	    

	old_rect = view->rect;
	(void) win_getsize(view->window_fd, &view->rect);
	delta_height = view->rect.r_height - old_rect.r_height;
	/* Adjust to account for the change in the window size */
	if (sb) {
		sb_rect = (Rect *)LINT_CAST(scrollbar_get(sb, SCROLL_RECT));
		if (sb_rect) {
		    new_rect = *sb_rect;
		    new_rect.r_height += delta_height;
		    scrollbar_set(sb, SCROLL_RECT, &new_rect, 0);
		}
	}
	new_rect = view->e_view->rect;
	new_rect.r_height += delta_height;
	new_rect.r_width += view->rect.r_width - old_rect.r_width;
	(void) ev_set(view->e_view, EV_RECT, &new_rect, 0);

}

extern Textsw_expand_status
textsw_expand(abstract,
	      start, stop_plus_one, out_buf, out_buf_len, total_chars)
	Textsw			 abstract;
	Es_index		 start;	/* Entity to start expanding at */
	Es_index		 stop_plus_one; /* 1st ent not expanded */
	char			*out_buf;
	int			 out_buf_len;
	int			*total_chars;
/* Expand the contents of the textsw from first to stop_plus_one into
 * the set of characters used to paint them, placing the expanded
 * text into out_buf, returning the number of character placed into
 * out_buf in total_chars, and returning status.
 */
{
	register Textsw_view	 view = VIEW_ABS_TO_REP(abstract);
	Ev_expand_status	 status;

	status = ev_expand(view->e_view,
	    start, stop_plus_one, out_buf, out_buf_len, total_chars);
	switch(status) {
	  case EV_EXPAND_OKAY:
	    return (TEXTSW_EXPAND_OK);
	  case EV_EXPAND_FULL_BUF:
	    return (TEXTSW_EXPAND_FULL_BUF);
	  case EV_EXPAND_OTHER_ERROR:
	  default:
	    return (TEXTSW_EXPAND_OTHER_ERROR);
	}
}
