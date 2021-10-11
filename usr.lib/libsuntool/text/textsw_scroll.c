#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)textsw_scroll.c 1.1 92/07/30";
#endif
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Liason routines between textsw and scrollbar
 */

#include <suntool/primal.h>
#include <suntool/textsw_impl.h>

extern void
textsw_set_scroll_mark(abstract)
	Textsw			abstract;
{
	register Textsw_view	view = VIEW_ABS_TO_REP(abstract);
	Scrollbar		sb = SCROLLBAR_FOR_VIEW(view);

	if (sb) {
	    scrollbar_set(sb,
			  SCROLL_MARK, view->e_view->line_table.seq[0],
			  0);
	}
}

pkg_private void
textsw_scroll(sb)
	Scrollbar		sb;
{
	Es_index		first, last_plus_one, new_start;
	Scroll_motion		motion;
	int			offset;
	register Textsw_view	view;

	view = (Textsw_view)LINT_CAST(scrollbar_get(sb, SCROLL_OBJECT));
	motion = (Scroll_motion)scrollbar_get(sb, SCROLL_REQUEST_MOTION);
	if (motion == SCROLL_ABSOLUTE) {
	    ev_view_range(view->e_view, &first, &last_plus_one);
	    new_start = (Es_index)scrollbar_get(sb, SCROLL_VIEW_START);
	    textsw_normalize_view(VIEW_REP_TO_ABS(view), new_start);
	} else {
	    register int	line;
	    offset = (int)LINT_CAST(scrollbar_get(sb, SCROLL_REQUEST_OFFSET));
	    line = ev_line_for_y(view->e_view, view->rect.r_top+offset);
	    if (line == 0)
		line++;		/* Always make some progress */
	    if (motion == SCROLL_BACKWARD)
		line = -line;
	    new_start = ev_scroll_lines(view->e_view, line, FALSE);
	}
	ev_view_range(view->e_view, &first, &last_plus_one);
	scrollbar_set(sb,
		      SCROLL_VIEW_START, first,
		      SCROLL_VIEW_LENGTH, last_plus_one-first,
		      0);
	scrollbar_paint(sb);
}

pkg_private void
textsw_update_scrollbars(folio, only_view)
	register Textsw_folio	folio;
	register Textsw_view	only_view;
{
	register Textsw_view	view;
	register Scrollbar	sb;
	Es_index		first, last_plus_one;

	FORALL_TEXT_VIEWS(folio, view) {
	    if (only_view && (view != only_view))
		continue;
	    if ((sb = SCROLLBAR_FOR_VIEW(view)) == 0)
		continue;
	    ev_view_range(view->e_view, &first, &last_plus_one);
	    scrollbar_set(sb,
			  SCROLL_VIEW_START, first,
			  SCROLL_VIEW_LENGTH, last_plus_one-first,
			  SCROLL_OBJECT_LENGTH, es_get_length(
						    folio->views->esh),
			  SCROLL_LINE_HEIGHT, ei_line_height(
						    folio->views->eih),
			  0);
	    scrollbar_paint(sb);
	}
}

