#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)textsw_selection.c 1.1 92/07/30";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * User interface to selection within text subwindows.
 */

#include <suntool/primal.h>
#include <suntool/textsw_impl.h>

extern Es_index		ev_line_start(), ev_resolve_xy();
extern void		ev_make_visible();
pkg_private Seln_rank	textsw_acquire_seln();

extern
textsw_normalize_view(abstract, pos)
	Textsw		abstract;
	Es_index	pos;
{
	Textsw_view	        view = VIEW_ABS_TO_REP(abstract);
	register Textsw_folio   folio = FOLIO_FOR_VIEW(view);
	int		        upper_context;
	
	upper_context = (int)LINT_CAST(
			    ev_get(folio->views, EV_CHAIN_UPPER_CONTEXT) );
	textsw_normalize_internal(VIEW_ABS_TO_REP(abstract), pos, pos, upper_context, 0,
				  TXTSW_NI_DEFAULT);
}

extern
textsw_possibly_normalize(abstract, pos)
	Textsw		abstract;
	Es_index	pos;
{
	Textsw_view	        view = VIEW_ABS_TO_REP(abstract);
	register Textsw_folio   folio = FOLIO_FOR_VIEW(view);
	int		        upper_context;
	
	upper_context = (int)LINT_CAST(
			    ev_get(folio->views, EV_CHAIN_UPPER_CONTEXT) );
			    
	textsw_normalize_internal(view, pos, pos, upper_context, 0,
				  TXTSW_NI_NOT_IF_IN_VIEW|TXTSW_NI_MARK);
}

extern
textsw_possibly_normalize_and_set_selection(
	abstract, first, last_plus_one, type)
	Textsw		abstract;
	Es_index	first, last_plus_one;
	unsigned	type;
{
	int		        upper_context;
	Textsw_view	        view = VIEW_ABS_TO_REP(abstract);
	register Textsw_folio   folio = FOLIO_FOR_VIEW(view);



	upper_context = (int)LINT_CAST(
			    ev_get(folio->views, EV_CHAIN_UPPER_CONTEXT) );

	textsw_normalize_internal(view, first, last_plus_one, upper_context, 0,
		TXTSW_NI_NOT_IF_IN_VIEW|TXTSW_NI_MARK|TXTSW_NI_SELECT(type));
}

pkg_private int
textsw_normalize_internal(
	view, first, last_plus_one, upper_context, lower_context, flags)
	register Textsw_view	view;
	Es_index		first, last_plus_one;
	int			upper_context, lower_context;
	register unsigned	flags;
{
	Rect			rect;
	Es_index		line_start, start;
	int			lines_above, lt_index, lines_below;
	int			normalize = TRUE;

	if (flags & TXTSW_NI_NOT_IF_IN_VIEW) {
	    switch (ev_xy_in_view(view->e_view, first, &lt_index, &rect)) {
	      case EV_XY_VISIBLE:
		normalize = FALSE;
		break;
	      case EV_XY_RIGHT:
		normalize = FALSE;
		break;
	      default:
		break;
	    }
	}
	if (normalize) {
	    if (flags & TXTSW_NI_MARK)
		textsw_set_scroll_mark(VIEW_REP_TO_ABS(view));
	    line_start = ev_line_start(view->e_view, first);
	    lines_below = textsw_screen_line_count(VIEW_REP_TO_ABS(view));
	    
	    if (flags & TXTSW_NI_AT_BOTTOM) {
		lines_above = (lines_below > lower_context) ?
		               (lines_below - (lower_context + 1)) : (lines_below - 1);		
	    } else
		lines_above = (upper_context < lines_below) ? upper_context : 0;
		
	    if (lines_above > 0) {
		ev_find_in_esh(view->folio->views->esh, "\n", 1,
			       line_start, (unsigned)lines_above+1,
			       EV_FIND_BACKWARD, &start, &line_start);
		if (start == ES_CANNOT_SET)
			line_start = 0;
	    }
	    /* Ensure no caret turds will leave behind */
	    textsw_take_down_caret(FOLIO_FOR_VIEW(view));

	    ev_set_start(view->e_view, line_start);

	    /* ensure line_start really is visible now */
	    lines_below -= (lines_above + 1);
	    ev_make_visible(view->e_view, first, lines_below, 0, 0);

	    textsw_update_scrollbars(FOLIO_FOR_VIEW(view), view);
	}
	if (EV_SEL_BASE_TYPE(flags)) {
	    textsw_set_selection(VIEW_REP_TO_ABS(view), first, last_plus_one,
				 EV_SEL_BASE_TYPE(flags));
	}
}

pkg_private Es_index
textsw_set_insert(folio, pos)
	register Textsw_folio	folio;
	register Es_index	pos;
{
	extern Es_index		ev_get_insert(), ev_set_insert();
	register Es_index	set_to;
	Es_index		boundary;

	if (TXTSW_IS_READ_ONLY(folio)) {
	    return(ev_get_insert(folio->views));
	}
	if (TXTSW_HAS_READ_ONLY_BOUNDARY(folio)) {
	    boundary = textsw_find_mark_internal(folio,
						 folio->read_only_boundary);
	    if (pos < boundary) {
		if AN_ERROR(boundary == ES_INFINITY) {
		} else
		    return(ev_get_insert(folio->views));
	    }
	}
	/* Ensure timer is set to fix caret display */
	textsw_take_down_caret(folio);
	set_to = ev_set_insert(folio->views, pos);
	ASSUME((pos == ES_INFINITY) || (pos == set_to));
	return(set_to);
}

extern caddr_t
textsw_checkpoint_undo(abstract, undo_mark)
	Textsw			abstract;
	caddr_t			undo_mark;
{
	register Textsw_folio	folio =
				FOLIO_FOR_VIEW(VIEW_ABS_TO_REP(abstract));
	caddr_t			current_mark;

	/* AGAIN/UNDO support */
	if (((int)undo_mark) < TEXTSW_INFINITY-1) {
	    current_mark = undo_mark;
	} else {
	    current_mark = es_get(folio->views->esh, ES_UNDO_MARK);
	}
	if (TXTSW_DO_UNDO(folio) &&
	    (((int)undo_mark) != TEXTSW_INFINITY)) {
	    if (current_mark != folio->undo[0]) {
		/* Make room for, and then record the current mark. */
		bcopy((char *)(&folio->undo[0]), (char *)(&folio->undo[1]),
		      (int)(folio->undo_count-1)*sizeof(folio->undo[0]));
		folio->undo[0] = current_mark;
	    }
	}
	return(current_mark);
}

extern
textsw_checkpoint_again(abstract)
	Textsw			abstract;
{
	register Textsw_folio	folio =
				FOLIO_FOR_VIEW(VIEW_ABS_TO_REP(abstract));

	/* AGAIN/UNDO support */
	if (!TXTSW_DO_AGAIN(folio))
	    return;
	if (folio->func_state & TXTSW_FUNC_AGAIN)
	    return;
	folio->again_first = ES_INFINITY;
	folio->again_last_plus_one = ES_INFINITY;
	folio->again_insert_length = 0;
	if (TXTSW_STRING_IS_NULL(&folio->again[0]))
	    return;
	if (folio->again_count > 1) {
	    /* Make room for this action sequence. */
	    textsw_free_again(folio,
			      &folio->again[folio->again_count-1]);
	    bcopy((char *)(&folio->again[0]), (char *)(&folio->again[1]),
		  (int)(folio->again_count-1)*sizeof(folio->again[0]));
	}
	folio->again[0] = null_string;
	folio->state &= ~(TXTSW_AGAIN_HAS_FIND|TXTSW_AGAIN_HAS_MATCH); 
}

extern
textsw_set_selection(abstract, first, last_plus_one, type)
	Textsw			abstract;
	Es_index		first, last_plus_one;
	unsigned		type;
{
	extern int		ev_set_selection();
	register Textsw_folio	folio =
				FOLIO_FOR_VIEW(VIEW_ABS_TO_REP(abstract));

	textsw_take_down_caret(folio);
	type &= EV_SEL_MASK;
	if ((first == ES_INFINITY) && (last_plus_one == ES_INFINITY)) {
	    ev_clear_selection(folio->views, type);
	    return;
	}
	ev_set_selection(folio->views, first, last_plus_one, type);
	(void) textsw_acquire_seln(folio, seln_rank_from_textsw_info(type));
	if (type & EV_SEL_PRIMARY) {
	    (void) textsw_checkpoint_undo(abstract,
					  (caddr_t)TEXTSW_INFINITY-1);
	}
}

pkg_private Textsw_view
textsw_view_for_entity_view(folio, e_view)
	Textsw_folio		folio;
	Ev_handle		e_view;
/* It is okay for the caller to pass EV_NULL for e_view. */
{
	register Textsw_view	textsw_view;

	FORALL_TEXT_VIEWS(folio, textsw_view) {
	    if (textsw_view->e_view == e_view)
		return(textsw_view);
	}
	return((Textsw_view)0);
}

static int
update_selection(view, ie)
	Textsw_view		 view;
	register Event		*ie;
{
	register Textsw_folio	 folio = FOLIO_FOR_VIEW(view);
	register unsigned	 sel_type;
	register Es_index	 position;
	Es_index		 first, last_plus_one, ro_bdry;

	position = ev_resolve_xy(view->e_view, ie->ie_locx, ie->ie_locy);
	if (position == ES_INFINITY)
	    return;
	sel_type = textsw_determine_selection_type(folio, TRUE);
	if (position == es_get_length(folio->views->esh)) {
	    /* Check for and handle case of empty textsw. */
	    if (position == 0) {
		last_plus_one = 0;
		goto Do_Update;
	    }
	    position--;		/* ev_span croaks if passed EOS */
	}
	if (folio->track_state & TXTSW_TRACK_POINT) {
	    if (folio->span_level == EI_SPAN_CHAR) {
		last_plus_one = position+1;
	    } else {
		ev_span(folio->views, position, &first, &last_plus_one,
			(int)folio->span_level);
		ASSERT(first != ES_CANNOT_SET);
		position = first;
	    }
	} else {
	    unsigned	save_span_level = folio->span_level;
	    /*
	     *	Adjust
	     */
	    if (event_action(ie) != LOC_MOVE) {
		(void) ev_get_selection(folio->views, &first, &last_plus_one,
					sel_type);
		if (first >= last_plus_one) {
		    /*
		     * Treat Adjust without prior selection as if it was
		     *   Point then Adjust.
		     */
		    first = position;
		    last_plus_one = position+1;
		}
		if ((position == first) || (position+1 == last_plus_one)) {
		    folio->track_state |= TXTSW_TRACK_ADJUST_END;
		} else
		    folio->track_state &= ~TXTSW_TRACK_ADJUST_END;
		folio->adjust_pivot = (
		    position < (last_plus_one+first)/2 ? last_plus_one: first);
	    }
	    if (folio->track_state & TXTSW_TRACK_ADJUST_END)
		/* Adjust-from-end is char-adjust */
		folio->span_level = EI_SPAN_CHAR;
	    if ((folio->state & TXTSW_ADJUST_IS_PD) &&
		(sel_type & EV_SEL_PRIMARY)) {
		if (folio->state & TXTSW_CONTROL_DOWN)
		    sel_type &= ~EV_SEL_PD_PRIMARY;
		else
		    sel_type |= EV_SEL_PD_PRIMARY;
	    }
	    if (position < folio->adjust_pivot) {
		/* There is nothing to the left of position == 0! */
		if (position > 0) {
		    /* Note:
		     * span left only finds the left portion of what
		     * the entire span would be.
		     * The position between lines is ambiguous: is it
		     * at the end of the previous line or the beginning
		     * of the next?
		     * EI_SPAN_WORD treats it as being at the end of
		     * the inter-word space terminated by newline.
		     * EI_SPAN_LINE treats it as being at the start of
		     * the following line.
		     */
		    switch(folio->span_level) {
		    case EI_SPAN_CHAR:
			break;
		    case EI_SPAN_WORD:
			ev_span(folio->views, position+1,
			    &first, &last_plus_one,
			    (int)folio->span_level | EI_SPAN_LEFT_ONLY);
			position = first;
			ASSERT(position != ES_CANNOT_SET);
			break;
		    case EI_SPAN_LINE:
			ev_span(folio->views, position,
			    &first, &last_plus_one,
			    (int)folio->span_level | EI_SPAN_LEFT_ONLY);
			position = first;
			ASSERT(position != ES_CANNOT_SET);
			break;
	            }
		}
		last_plus_one = folio->adjust_pivot;
	    } else {
		if (folio->span_level == EI_SPAN_CHAR) {
		    last_plus_one = position+1;
		} else {
		    ev_span(folio->views, position, &first, &last_plus_one,
			(int)folio->span_level | EI_SPAN_RIGHT_ONLY);
		    ASSERT(first != ES_CANNOT_SET);
		}
		position = folio->adjust_pivot;
	    }
	    folio->span_level = save_span_level;
	}

Do_Update:
	if (sel_type & (EV_SEL_PD_PRIMARY|EV_SEL_PD_SECONDARY)) {
	    ro_bdry = TXTSW_IS_READ_ONLY(folio) ? last_plus_one
		      : textsw_read_only_boundary_is_at(folio);
	    if (last_plus_one <= ro_bdry) {
		sel_type &= ~(EV_SEL_PD_PRIMARY|EV_SEL_PD_SECONDARY);
	    }
	}
	textsw_set_selection(VIEW_REP_TO_ABS(view),
			     position, last_plus_one, sel_type);
	if (sel_type & EV_SEL_PRIMARY) {
	    textsw_checkpoint_again(VIEW_REP_TO_ABS(view));
	}
	ASSERT(allock());
}

pkg_private
start_selection_tracking(view, ie)
	Textsw_view		 view;
	register Event		*ie;
{
	register Textsw_folio	 folio = FOLIO_FOR_VIEW(view);

	textsw_flush_caches(view, TFC_STD);
	switch (event_action(ie)) {
	  case TXTSW_ADJUST:
	    folio->track_state |= TXTSW_TRACK_ADJUST;
	    folio->last_click_x = ie->ie_locx;
	    folio->last_click_y = ie->ie_locy;
	    break;
	  case TXTSW_POINT: {
	    int		delta;		/* in millisecs */
	    folio->track_state |= TXTSW_TRACK_POINT;
	    delta = (ie->ie_time.tv_sec - folio->last_point.tv_sec) * 1000;
	    delta += ie->ie_time.tv_usec / 1000;
	    delta -= folio->last_point.tv_usec / 1000;
	    if (delta >= folio->multi_click_timeout) {
		folio->span_level = EI_SPAN_CHAR;
	    } else {
		int	delta_x = ie->ie_locx - folio->last_click_x;
		int	delta_y = ie->ie_locy - folio->last_click_y;
		if (delta_x < 0)
		    delta_x = -delta_x;
		if (delta_y < 0)
		    delta_y = -delta_y;
		if (delta_x > folio->multi_click_space ||
		    delta_y > folio->multi_click_space) {
		    folio->span_level = EI_SPAN_CHAR;
		} else {
		    switch (folio->span_level) {
		      case EI_SPAN_CHAR:
			folio->span_level = EI_SPAN_WORD;
			break;
		      case EI_SPAN_WORD:
			folio->span_level = EI_SPAN_LINE;
			break;
		      case EI_SPAN_LINE:
			folio->span_level = EI_SPAN_DOCUMENT;
			break;
		      case EI_SPAN_DOCUMENT:
			folio->span_level = EI_SPAN_CHAR;
			break;
		      default: folio->span_level = EI_SPAN_CHAR;
		    }
		}
	    }
	    folio->last_click_x = ie->ie_locx;
	    folio->last_click_y = ie->ie_locy;
	    break;
	  }
	  default: LINT_IGNORE(ASSUME(0));
	}
	if ((folio->state & TXTSW_CONTROL_DOWN) &&
	    !TXTSW_IS_READ_ONLY(folio))
	    folio->state |= TXTSW_PENDING_DELETE;
	update_selection(view, ie);
}

static Es_index
do_balance_beam(view, x, y, first, last_plus_one)
	Textsw_view		view;
	int			x, y;
	register Es_index	first, last_plus_one;
{
	register Ev_handle	e_view;
	Es_index		new_insert;
	register Textsw_folio	folio = FOLIO_FOR_VIEW(view);
	register int		delta;
	int			lt_index;
	struct rect		rect;

	if (folio->track_state & TXTSW_TRACK_ADJUST) {
	    return((first == folio->adjust_pivot) ? last_plus_one : first);
	}
	new_insert = last_plus_one;
	e_view = view->e_view;
	/*
	 * When the user has multi-clicked up to select enough text to
	 *   occupy multiple physical display lines, linearize the
	 *   distance to the endpoints, then do the balance beam.
	 */
	if (ev_xy_in_view(e_view, first, &lt_index, &rect)
	    != EV_XY_VISIBLE)
	    goto Did_balance;
	delta = x - rect.r_left;
	delta += ((y - rect.r_top) / rect.r_height) *
		 e_view->rect.r_width;
	switch (ev_xy_in_view(e_view, last_plus_one, &lt_index, &rect)) {
	  case EV_XY_BELOW:
	    /*
	     * SPECIAL CASE: if last_plus_one was at the start of
	     * the last line in the line table, it is still EV_XY_BELOW,
	     * and we should still treat it like EV_XY_VISIBLE,
	     * with rect.r_left == e_view->rect.r_left.
	     */
	    if (last_plus_one != ft_position_for_index(
	    			   e_view->line_table,
	    			   e_view->line_table.last_plus_one - 1)) {
	        new_insert = first;
	        goto Did_balance;
	    } else {
	        rect.r_left = e_view->rect.r_left;
	    }
	    /* FALL THROUGH */
	  case EV_XY_VISIBLE:
	    if (e_view->rect.r_left == rect.r_left) {
		/* last_plus_one must be a new_line, so back up */
		if (ev_xy_in_view(e_view, last_plus_one-1, &lt_index, &rect)
		    != EV_XY_VISIBLE) {
		    new_insert = first;
		    goto Did_balance;
		}
		if ((x >= rect.r_left) && (y >= rect.r_top) && (y <= rect_bottom(&rect))) {
		    /*
		     * SPECIAL CASE: Selecting in the white-space past the end
		     *   of a line puts the insertion point at the end of that
		     *   line, rather than the beginning of the next line.
		     *
		     * SPECIAL SPECIAL CASE: Selecting in the white-space past
		     * the end of a wrapped line (due to wide margins
		     * or word wrapping) puts the insertion point at the
		     * beginning of the next line.
		     */ 
		    if (ev_considered_for_line(e_view, lt_index) >=
		        ev_index_for_line(e_view, lt_index+1)) 
		        new_insert = last_plus_one;
		    else
		        new_insert = last_plus_one-1;
		    goto Did_balance;
		}
	    }
	    break;
	  default:
	    new_insert = first;
	    goto Did_balance;
	}
	if (y < rect.r_top)
	    rect.r_left += (((rect.r_top - y) / rect.r_height) + 1) *
			    e_view->rect.r_width;
	if (delta < rect.r_left - x)
	    new_insert = first;
Did_balance:
	return(new_insert);
}

static
done_tracking(view, x, y)
	Textsw_view		view;
	register int		x, y;
{
	register Textsw_folio	folio = FOLIO_FOR_VIEW(view);

	if (((folio->track_state & TXTSW_TRACK_SECONDARY) == 0) ||
	    (folio->func_state & TXTSW_FUNC_PUT)) {
	    Es_index		first, last_plus_one, new_insert;

	    (void) ev_get_selection(folio->views, &first, &last_plus_one,
			(unsigned)((folio->func_state & TXTSW_FUNC_PUT)
				? EV_SEL_SECONDARY : EV_SEL_PRIMARY));
	    new_insert = do_balance_beam(view, x, y, first, last_plus_one);
	    if (new_insert != ES_INFINITY) {
		first = textsw_set_insert(folio, new_insert);
		ASSUME(first != ES_CANNOT_SET);
	    }
	}
	folio->track_state &= ~(TXTSW_TRACK_ADJUST
			  | TXTSW_TRACK_ADJUST_END
			  | TXTSW_TRACK_POINT);
	/* Reset pending-delete state (although we may be in the middle of a
	 * multi-click, there is no way to know that and we must reset now in
	 * case user has ADJUST_IS_PENDING_DELETE or had CONTROL_DOWN).
	 */
	if ((folio->func_state & TXTSW_FUNC_DELETE) == 0)
	    folio->state &= ~TXTSW_PENDING_DELETE;
}

pkg_private int
track_selection(view, ie)
	Textsw_view		 view;
	register Event		*ie;
{
	int			 do_done_tracking = 0;
	register Textsw_folio	 folio = FOLIO_FOR_VIEW(view);

	if (win_inputnegevent(ie)) {
	    switch (event_action(ie)) {
	      case TXTSW_POINT: {
		folio->last_point = ie->ie_time;
		break;
	      }
	      case TXTSW_ADJUST: {
		folio->last_adjust = ie->ie_time;
		break;
	      }
	      default:
		if ((folio->track_state & TXTSW_TRACK_SECONDARY) ||
		    (folio->func_state & TXTSW_FUNC_ALL)) {
		    done_tracking(view, ie->ie_locx, ie->ie_locy);
		    return(FALSE);	/* Don't ignore: FUNC up */
		} else {
		    return(TRUE);	/* Ignore: other key up */
		}
	    }
	    done_tracking(view, ie->ie_locx, ie->ie_locy);
	} else {
	    switch (event_action(ie)) {
	      case SCROLL_EXIT:
		/*
		 * if we were tracking before entering the scrollbar
		 * and the equivalent mouse button is up on exiting the
		 * scrollbar then quit tracking.
		 */
		if (folio->track_state & TXTSW_TRACK_ADJUST) {
		    do_done_tracking = (win_get_vuid_value(
					WIN_FD_FOR_VIEW(view),
					TXTSW_ADJUST) == 0);
		} else if (folio->track_state & TXTSW_TRACK_POINT) {
		    do_done_tracking = (win_get_vuid_value(
					WIN_FD_FOR_VIEW(view),
					TXTSW_POINT) == 0);
		}
		if (do_done_tracking) {
		    /* use x, y from SCROLL_ENTER */
		    done_tracking(view,
		    	folio->last_click_x, folio->last_click_y);
		}
		break;
	      case LOC_MOVE:
		update_selection(view, ie);
		break;
	      case LOC_WINEXIT:
		done_tracking(view, ie->ie_locx, ie->ie_locy);
		textsw_may_win_exit(folio);
		break;
	      default:;	/* ignore it */
	    }
	}
	return(TRUE);
}

/*	===============================================================
 *
 *	Utilities to simplify dealing with selections.
 *
 *	===============================================================
 */

pkg_private int
textsw_get_selection_as_int(folio, type, default_value)
	register Textsw_folio	folio;
	unsigned		type;
	int			default_value;

{
	Textsw_selection_object	selection;
	int			result;
	char			buf[20];

	textsw_init_selection_object(folio, &selection, buf, sizeof(buf), 0);
	result = textsw_func_selection_internal(
			folio, &selection, type, TFS_FILL_ALWAYS);
	if (TFS_IS_ERROR(result)) {
	    result = default_value;
	} else {
	    buf[selection.buf_len] = '\0';
	    result = atoi(buf);
	}
	return(result);
}

pkg_private int
textsw_get_selection_as_string(folio, type, buf, buf_max_len)
	register Textsw_folio	 folio;
	unsigned		 type;
	char			*buf;

{
	Textsw_selection_object	 selection;
	int			 result;

	textsw_init_selection_object(folio, &selection, buf, buf_max_len, 0);
	result = textsw_func_selection_internal(
			folio, &selection, type, TFS_FILL_ALWAYS);
	if (TFS_IS_ERROR(result) || (selection.buf_len == 0)) {
	    result = 0;
	} else {
	    buf[selection.buf_len] = '\0';
	    result = selection.buf_len+1;
	}
	return(result);
}

