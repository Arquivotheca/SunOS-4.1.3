#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)textsw_dbx.c 1.1 92/07/30";
#endif
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Support routines for dbx's use of text subwindows.
 */

#include <suntool/primal.h>
#include <suntool/textsw_impl.h>
#include <pixrect/pixfont.h>

extern Ev_mark_object	 ev_add_glyph();
extern Ev_mark_object	 ev_add_glyph_on_line();
extern void		 ev_line_info();
extern void		 ev_remove_glyph();
extern Es_index		 ev_position_for_physical_line();

extern Textsw
textsw_first(any)
	Textsw		any;
{
	Textsw_view	view = VIEW_ABS_TO_REP(any);

	return(view ? VIEW_REP_TO_ABS(FOLIO_FOR_VIEW(view)->first_view)
		    : TEXTSW_NULL);
}

extern Textsw
textsw_next(previous)
	Textsw		previous;
{
	Textsw_view	view = VIEW_ABS_TO_REP(previous);

	return(view ? VIEW_REP_TO_ABS(view->next) : TEXTSW_NULL);
}

extern int
textsw_does_index_not_show(abstract, index, line_index)
	Textsw		abstract;
	Es_index	index;
	int		*line_index;	/* output only.
					/* if index does not show then
					/* not set. */
{
	Textsw_view	 view = VIEW_ABS_TO_REP(abstract);
	Rect		 rect;
	int		 dummy_line_index;
	
	if (!line_index)
	line_index = &dummy_line_index;
	switch (ev_xy_in_view(view->e_view, index, line_index, &rect)) {
	  case EV_XY_VISIBLE:
	    return(0);
	  case EV_XY_RIGHT:
	    return(0);
	  case EV_XY_BELOW:
	    return(1);
	  case EV_XY_ABOVE:
	    return(-1);
	  default:	/* should never happen */
	    return(-1);
	}
}

extern int
textsw_screen_line_count(abstract)
	Textsw		abstract;
{
	Textsw_view	view = VIEW_ABS_TO_REP(abstract);

	return(view ? view->e_view->line_table.last_plus_one-1 : 0);
}

extern int
textsw_screen_column_count(abstract)
	Textsw		 abstract;
{
	Textsw_view	 view = VIEW_ABS_TO_REP(abstract);
	PIXFONT		*font = (PIXFONT *)
				LINT_CAST(textsw_get(abstract, TEXTSW_FONT));

	return(view->e_view->rect.r_width / font->pf_char['m'].pc_adv.x);
}

/*   Following is obsolete; replace by:
 * textsw_set(abstract, TEXTSW_FIRST, pos, 0);
 */
extern void
textsw_set_start(abstract, pos)
	Textsw		abstract;
	Textsw_index	pos;
{
	Textsw_view	view = VIEW_ABS_TO_REP(abstract);

	ev_set_start(view->e_view, pos);
}

extern void
textsw_file_lines_visible(abstract, top, bottom)
	Textsw	 abstract;
	int	*top, *bottom;
{
	Textsw_view	view = VIEW_ABS_TO_REP(abstract);

	ev_line_info(view->e_view, top, bottom);
	*top -= 1;
	*bottom -= 1;
}

extern void
textsw_view_line_info(abstract, top, bottom)
	Textsw	 abstract;
	int	*top, *bottom;
{
	Textsw_view	view = VIEW_ABS_TO_REP(abstract);

	ev_line_info(view->e_view, top, bottom);
}

extern int
textsw_contains_line(abstract, line, rect)
	register Textsw	 abstract;
	register int	 line;
	register Rect	*rect;
{
	int		 lt_index;
	int		 top, bottom;
	Es_index	 first;
	Textsw_view	 view = VIEW_ABS_TO_REP(abstract);

	textsw_view_line_info(abstract, &top, &bottom);
	if (line < top || line > bottom)
	    return(-2);
	lt_index = ev_rect_for_ith_physical_line(
			view->e_view, line-top, &first, rect, TRUE);
	return(lt_index);
}

/* ARGSUSED */
extern int
textsw_nop_notify(abstract, attrs)
	Textsw		abstract;
	Attr_avlist	attrs;
{
	return(0);
}

extern Textsw_index
textsw_index_for_file_line(abstract, line)
	Textsw		abstract;
	int		line;
{
	Textsw_view	view = VIEW_ABS_TO_REP(abstract);
	Textsw_folio	folio = FOLIO_FOR_VIEW(view);
	Es_index	result;

	result = ev_position_for_physical_line(folio->views, line, 0);
	return((Textsw_index)result);
}

/* Following is for compatibility with old client code. */
extern Textsw_index
textsw_position_for_physical_line(abstract, physical_line)
	Textsw		abstract;
	int		physical_line;		/* Note: 1-origin, not 0! */
{
	return(textsw_index_for_file_line(abstract, physical_line-1));
}

extern void
textsw_scroll_lines(abstract, count)
	Textsw		abstract;
	int		count;
{
	Textsw_view	view = VIEW_ABS_TO_REP(abstract);

	(void) ev_scroll_lines(view->e_view, count, FALSE);
}

extern Textsw_mark
textsw_add_glyph(abstract, pos, pr, op, offset_x, offset_y, flags)
	Textsw			 abstract;
	Textsw_index		 pos;
	Pixrect			*pr;
	int			 op;
	int			 offset_x, offset_y;
{
	Textsw_view		 view = VIEW_ABS_TO_REP(abstract);
	Textsw_folio		 folio = FOLIO_FOR_VIEW(view);
	Es_index		 line_start;
	Ev_mark_object		 mark;

	if (flags & TEXTSW_GLYPH_DISPLAY)
	    textsw_take_down_caret(folio);

	/* BUG ALERT!  True for only current client (filemerge), but wrong
	 * in general.
	 */
	line_start = pos;

	/* Assume that TEXTSW_ flags == EV_ flags */
	mark = ev_add_glyph(folio->views, line_start, pos, pr, op,
			    offset_x, offset_y);
	return((Textsw_mark)mark);
}

extern Textsw_mark
textsw_add_glyph_on_line(abstract, line, pr, op, offset_x, offset_y, flags)
	Textsw		 abstract;
	int		 line;		/* Note: 1-origin, not 0! */
	struct pixrect	*pr;
	int		 op;
	int		 offset_x, offset_y;
	int		 flags;
{
	Textsw_view	 view = VIEW_ABS_TO_REP(abstract);
	Textsw_folio	 folio = FOLIO_FOR_VIEW(view);
	Ev_mark_object	 mark;

	if (flags & TEXTSW_GLYPH_DISPLAY)
	    textsw_take_down_caret(folio);

	/* Assume that TEXTSW_ flags == EV_ flags */
	mark = ev_add_glyph_on_line(folio->views, line-1, pr,
				    op, offset_x, offset_y, flags);
	return((Textsw_mark)mark);
}

extern void
textsw_remove_glyph(abstract, mark, flags)
	Textsw		 abstract;
	Textsw_mark	 mark;
	int		 flags;
{
	Textsw_view	 view = VIEW_ABS_TO_REP(abstract);
	long unsigned	*dummy_for_compiler = (long unsigned *)&mark;

	textsw_take_down_caret(FOLIO_FOR_VIEW(view));

	ev_remove_glyph(FOLIO_FOR_VIEW(view)->views,
		        *(Ev_mark)dummy_for_compiler, (unsigned)flags);
}

extern void
textsw_set_glyph_pr(abstract, mark, pr)
	Textsw		 abstract;
	Textsw_mark	 mark;
	struct pixrect	*pr;
{
	Textsw_view	 view = VIEW_ABS_TO_REP(abstract);
	long unsigned	*dummy_for_compiler = (long unsigned *)&mark;

	textsw_take_down_caret(FOLIO_FOR_VIEW(view));

	ev_set_glyph_pr(FOLIO_FOR_VIEW(view)->views,
		        *(Ev_mark)dummy_for_compiler, pr);
}

extern Textsw_index
textsw_start_of_display_line(abstract, pos)
	Textsw		 abstract;
	Textsw_index	 pos;
{
	register Textsw_view	 view = VIEW_ABS_TO_REP(abstract);
	extern Es_index		 ev_display_line_start();

	return ((Textsw_index)ev_display_line_start(view->e_view, pos));
}
