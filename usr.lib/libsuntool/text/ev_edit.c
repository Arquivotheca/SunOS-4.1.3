#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)ev_edit.c 1.1 92/07/30";
#endif
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Initialization and finalization of entity views.
 */

#include <suntool/primal.h>

#include <suntool/ev_impl.h>
#include <sys/time.h>
#include <pixrect/pr_util.h>
#include <pixrect/memvar.h>
#include <pixrect/pixfont.h>
#include <suntool/tool.h>

extern void			ev_notify();
extern void			ev_clear_from_margins();
extern void			ev_check_insert_visibility();
extern int			ev_clear_rect();
extern struct rect		ev_rect_for_line();

unsigned
ev_span(views, position, first, last_plus_one, group_spec)
	register Ev_chain	 views;
	Es_index		 position,
				*first,
				*last_plus_one;
	int			 group_spec;
{
	struct ei_span_result		span_result;
	char				buf[EV_BUFSIZE];
	struct es_buf_object		esbuf;
	esbuf.esh = views->esh;
	esbuf.buf = buf;
	esbuf.sizeof_buf = sizeof(buf);
	esbuf.first = esbuf.last_plus_one = 0;
	span_result = ei_span_of_group(
	    views->eih, &esbuf, group_spec, position);
	*first = span_result.first;
	*last_plus_one = span_result.last_plus_one;
	return(span_result.flags);
}

Es_index
ev_line_start(view, position)
	register Ev_handle	view;
	register Es_index	position;
{
	Es_index			dummy, result;
	register Ev_impl_line_seq	seq = (Ev_impl_line_seq)
						view->line_table.seq;
	if (position >= seq[0].pos) {
	    /* Optimization: First try the view's line_table */
	    int		lt_index = ft_bounding_index(
					&view->line_table, position);
	    if (lt_index < view->line_table.last_plus_one - 1)
		return(seq[lt_index].pos);
	    /* -1 is required to avoid mapping all large positions to the
	     *   hidden extra line entry.
	     */
	}
	ev_span(view->view_chain, position, &result, &dummy,
		EI_SPAN_LINE | EI_SPAN_LEFT_ONLY);
	if (result == ES_CANNOT_SET) {
	    result = position;
	}
	return(result);
}

Es_index
ev_get_insert(views)
	Ev_chain	 views;
{
	return((EV_CHAIN_PRIVATE(views))->insert_pos);
}

Es_index
ev_set_insert(views, position)
	Ev_chain	 views;
	Es_index	 position;
{
	Es_handle		esh = views->esh;
	Ev_chain_pd_handle	private = EV_CHAIN_PRIVATE(views);
	Es_index		result;

	result = es_set_position(esh, position);
	if (result != ES_CANNOT_SET) {
		private->insert_pos = result;
	}
	return(result);
}

extern void
ev_update_lt_after_edit(table, before_edit, delta)
	register Ev_line_table	*table;
	Es_index		 before_edit;
	register long int	 delta;
{
/*
 * Modifies the entries in table as follows:
 *   delta > 0 => (before_edit..EOS) incremented by delta
 *   delta < 0 => (before_edit+delta..before_edit] mapped to before_edit+delta,
 *		  (before_edit..EOS) decremented by ABS(delta).
 */
	register		 lt_index;
	Ev_impl_line_seq	 line_seq = (Ev_impl_line_seq) table->seq;

	if (delta == 0)
	    return;
	if (delta > 0) {
	    /*
	     * Only entries greater than before_edit are affected.  In
	     *   particular, entries at the original insertion position do not
	     *   extend.
	     */
	    if (before_edit < line_seq[0].pos) {
		ft_add_delta(*table, 0, delta);
	    } else {
		lt_index = ft_bounding_index(table, before_edit);
		if (lt_index < table->last_plus_one)
		    ft_add_delta(*table, lt_index+1, delta);
	    }
	} else {
	    /*
	     * Entries in the deleted range are set to the range's beginning.
	     * Entries greater than before_edit are simply decremented.
	     */
	    ft_set_esi_span(
		*table, before_edit+delta+1, before_edit,
		before_edit+delta, 0);
	    /* For now, let the compiler do all the cross jumping */
	    if (before_edit-1 < line_seq[0].pos) {
		ft_add_delta(*table, 0, delta);
	    } else {
		lt_index = ft_bounding_index(table, before_edit-1);
		if (lt_index < table->last_plus_one)
		     ft_add_delta(*table, lt_index+1, delta);
	    }
	}
}

/* Meaning of returned ei_span_result.flags>>16 is:
 *	0 success
 *	1 illegal edit_action
 *	2 unable to set insert
 */
extern struct ei_span_result
ev_span_for_edit(views, edit_action)
	Ev_chain	 views;
	int		 edit_action;
{
	Ev_chain_pd_handle	private = EV_CHAIN_PRIVATE(views);
	struct ei_span_result	span_result;
	int			group_spec = 0;
	char			buf[EV_BUFSIZE];
	struct es_buf_object	esbuf;

	switch (edit_action) {
	    case EV_EDIT_BACK_CHAR: {
		group_spec = EI_SPAN_CHAR | EI_SPAN_LEFT_ONLY;
		break;
	    }
	    case EV_EDIT_BACK_WORD: {
		group_spec = EI_SPAN_WORD | EI_SPAN_LEFT_ONLY;
		break;
	    }
	    case EV_EDIT_BACK_LINE: {
		group_spec = EI_SPAN_LINE | EI_SPAN_LEFT_ONLY;
		break;
	    }
	    case EV_EDIT_CHAR: {
		group_spec = EI_SPAN_CHAR | EI_SPAN_RIGHT_ONLY;
		break;
	    }
	    case EV_EDIT_WORD: {
		group_spec = EI_SPAN_WORD | EI_SPAN_RIGHT_ONLY;
		break;
	    }
	    case EV_EDIT_LINE: {
		group_spec = EI_SPAN_LINE | EI_SPAN_RIGHT_ONLY;
		break;
	    }
	    default: {
		span_result.flags = 1<<16;
		goto Return;
	    }
	}
	esbuf.esh = views->esh;
	esbuf.buf = buf;
	esbuf.sizeof_buf = sizeof(buf);
	esbuf.first = esbuf.last_plus_one = 0;
	span_result = ei_span_of_group(
	    views->eih, &esbuf, group_spec, private->insert_pos);
	if (span_result.first == ES_CANNOT_SET) {
	    span_result.flags = 2<<16;
	    goto Return;
	}
	if (((group_spec & EI_SPAN_CLASS_MASK) == EI_SPAN_WORD) &&
	    (span_result.flags & EI_SPAN_NOT_IN_CLASS) &&
	    (span_result.flags & EI_SPAN_HIT_NEXT_LEVEL) == 0) {
	    /*
	     * On a FORWARD/BACK_WORD, skip over preceding/trailing white
	     *   space and delete the preceding word.
	     */
	    struct ei_span_result	span2_result;
	    span2_result = ei_span_of_group(
		views->eih, &esbuf, group_spec,
		(group_spec & EI_SPAN_LEFT_ONLY)
		    ? span_result.first : span_result.last_plus_one);
	    if (span2_result.first != ES_CANNOT_SET) {
		if (group_spec & EI_SPAN_LEFT_ONLY)
		    span_result.first = span2_result.first;
		else
		    span_result.last_plus_one = span2_result.last_plus_one;
	    }
	}
Return:
	return(span_result);
}

ev_delete_span(views, first, last_plus_one, delta)
	Ev_chain		 views;
	register Es_index	 first, last_plus_one;
	Es_index		*delta;
{
	Es_handle		 esh = views->esh;
	register Ev_chain_pd_handle
				 private = EV_CHAIN_PRIVATE(views);
	register Es_index	 old_length = es_get_length(esh);
	Es_index		 new_insert_pos,
				 private_insert_pos = private->insert_pos;
	int			 result, used;

	/* Since *delta depends on last_plus_one, normalize ES_INFINITY */
	if (last_plus_one > old_length) {
	    last_plus_one = old_length;
	}
	/* See if operation makes sense */
	if (old_length == 0) {
	    result = 1;
	    goto Return;
	}
	/* We cannot assume where the esh is positioned, so position first. */
	if (first != es_set_position(esh, first)) {
	    result = 2;
	    goto Return;
	}
	new_insert_pos = es_replace(esh, last_plus_one, 0, 0, &used);
	if (new_insert_pos == ES_CANNOT_SET) {
	    result = 3;
	    goto Return;
	}
	*delta = first - last_plus_one;
	private->insert_pos = new_insert_pos;
		/* Above assignment required to make following call work! */
	ev_update_after_edit(
		views, last_plus_one, *delta, old_length, first);
	if (first < private_insert_pos) {
	    if (last_plus_one < private_insert_pos)
		private->insert_pos = private_insert_pos + *delta;
	    else
		/* Don't optimize out in case kludge above vanishes. */
		private->insert_pos = new_insert_pos;
	} else
	    private->insert_pos = private_insert_pos;
	if (private->notify_level & EV_NOTIFY_EDIT_DELETE) {
	    ev_notify(views->first_view,
		      EV_ACTION_EDIT, first, old_length, first, last_plus_one,
			0,
		      0);
	}
	result = 0;
Return:
	return(result);
}

static void
ev_update_fingers_after_edit(ft, insert, delta)
	register Ev_finger_table	*ft;
	register Es_index		 insert;
	register int			 delta;
/* This routine differs from the similar ev_update_lt_after_edit in that it
 * makes use of the extra Ev_finger_info fields in order to potentially
 * adjust entries at the insert point.
 */
{
	register Ev_finger_handle	 fingers;
	register int			 lt_index;

	ev_update_lt_after_edit(ft, insert, delta);
	if (delta > 0) {
	    /* Correct entries that move_at_insert in views->fingers */
	    lt_index = ft_bounding_index(ft, insert);
	    if (lt_index < ft->last_plus_one) {
		fingers = (Ev_finger_handle)ft->seq;
		while (fingers[lt_index].pos == insert) {
		    if (EV_MARK_MOVE_AT_INSERT(fingers[lt_index].info)) {
			fingers[lt_index].pos += delta;
			if (lt_index-- > 0)
			    continue;
		    }
		    break;
		}
	    }
	}
}

static void
ev_update_view_lt_after_edit(line_seq, start, stop_plus_one, minimum, delta)
	register Ev_impl_line_seq	 line_seq;
	int				 start;
	register int			 stop_plus_one;
	register Es_index		 minimum;
	register int			 delta;
/* This routine differs from the similar ev_update_lt_after_edit in that it
 * can introduce positions of ES_CANNOT_SET (which are transient and not
 * valid in a normal finger table).
 */
{
	register int			 i;

	for (i = start; i < stop_plus_one; i++) {
	    if (line_seq[i].pos < minimum) {
		line_seq[i].pos = ES_CANNOT_SET;
	    } else if (line_seq[i].pos != ES_INFINITY)
		line_seq[i].pos += delta;
	}
}

extern void
ev_update_view_display(view, clear_to_bottom)
	register Ev_handle	view;
        int clear_to_bottom;
{
	Es_index		*line_seq;

	ev_lt_format(view, &view->tmp_line_table, &view->line_table);
	ASSERT(allock());
	/*
	 * Swap line tables before paint so that old ev_display
	 * utilities may be called during paint phase.
	 */
	line_seq = view->line_table.seq;
	view->line_table.seq = view->tmp_line_table.seq;
	view->tmp_line_table.seq = line_seq;
	ev_lt_paint(view, &view->line_table, &view->tmp_line_table,
		    clear_to_bottom);
	ASSERT(allock());
}

ev_update_after_edit(views, last_plus_one, delta, old_length, min_insert_pos)
	register Ev_chain	 views;
	Es_index		 last_plus_one;
	int			 delta;
	Es_index		 old_length,
				 min_insert_pos;
/* last_plus_one:  end of inserted/deleted span
 * delta:	   if positive, amount inserted; else, amount deleted
 * old_length:	   length before change
 * min_insert_pos: beginning of inserted/deleted span
 */
{
	register Ev_handle	view;
	register Ev_pd_handle	private;
	Ev_chain_pd_handle	chain = EV_CHAIN_PRIVATE(views);

	ASSERT(allock());

	/*
	 * Update edit number and tables to reflect change to entity stream.
	 * This must be done whenever the entity stream is modified,
	 * regardless of whether the display is updated.
	 */
	chain->edit_number++;
	ev_update_lt_after_edit(&chain->op_bdry, last_plus_one, delta);
	ev_update_fingers_after_edit(&views->fingers, last_plus_one, delta);

	FORALLVIEWS(views, view) {
	    if (ev_lt_delta(view, last_plus_one, delta)) {
		private = EV_PRIVATE(view);
		if (private->state & EV_VS_DELAY_UPDATE) {
		    private->state |= EV_VS_DAMAGED_LT;
		    continue;
		}
		ev_update_view_display(view, FALSE);
	    }
	}
}

extern void
ev_update_chain_display(views)
	register Ev_chain	 views;
{
	register Ev_handle	view;
	register Ev_pd_handle	private;

	FORALLVIEWS(views, view) {
	    private = EV_PRIVATE(view);
	    if (private->state & EV_VS_DAMAGED_LT) {
		ev_update_view_display(view, FALSE);
		private->state &= ~EV_VS_DAMAGED_LT;
	    }
	}
}

extern void
ev_make_visible(view, position, lower_context, auto_scroll_by, delta)
	Ev_handle		view;
	Es_index		position;
	int			lower_context;
	int			auto_scroll_by;
	int			delta;
{
	extern int		ev_xy_in_view();
	Ev_impl_line_seq	line_seq;
	int			top_of_lc;
	int			lt_index;
	struct rect		rect;
	Es_index		new_top;

	line_seq = (Ev_impl_line_seq) view->line_table.seq;
	top_of_lc = max(1,
	    view->line_table.last_plus_one - 1 - lower_context);
	/* Following test works even if line_seq[].pos == ES_INFINITY.
	 * The test catches the common cases and saves the expensive
	 * call on ev_xy_in_view().
	 */
	if (position < line_seq[top_of_lc].pos)
	    return;
	switch (ev_xy_in_view(view, position, &lt_index, &rect)) {
	  case EV_XY_BELOW:
	    /* BUG ALERT: The following heuristic must be replaced! */
	    delta = min(delta, position - line_seq[top_of_lc].pos);
	    if (delta < (50 * view->line_table.last_plus_one)
	    &&  lower_context < view->line_table.last_plus_one - 1
	    &&  auto_scroll_by< view->line_table.last_plus_one - 1)
	    {
		Es_index		old_top = line_seq[0].pos;
		new_top = ev_scroll_lines(view,
			min(view->line_table.last_plus_one - 1,
			max(1, 
			max(lower_context, auto_scroll_by) 
			+ (delta/50))), FALSE);
		/*
		 * ev_scroll_lines swaps line tables; cannot continue using
		 * line_seq
		 */
		while ((old_top != new_top) &&
		       (position >= ev_index_for_line(view, top_of_lc))) {
		    old_top = new_top;
		    new_top = ev_scroll_lines(view,
			((position - ev_index_for_line(view, top_of_lc))
			> 150 ? 2 : 1), FALSE);
		}
	    } else {
		ev_set_start(view, ev_line_start(view, position));
	    }
	    break;
	  case EV_XY_RIGHT:
	  case EV_XY_VISIBLE:
	    /* only scroll if at least 1 newline was inserted */
	    if ((EV_PRIVATE(view))->cached_insert_info.lt_index != lt_index)
	    {
		/* We know lt_index >= top_of_lc */
		new_top = ev_scroll_lines(view,
		    min(lt_index,
		    max(lt_index - top_of_lc + 1,
		    auto_scroll_by)), FALSE);
		ASSUME(new_top >= 0);
	    }
	    break;
	  default:
	    break;
	}
}

extern void
ev_scroll_if_old_insert_visible(views, insert_pos, delta)
	register Ev_chain	views;
	register Es_index	insert_pos;
	register int		delta;
{
	register Ev_handle	view;
	register Ev_pd_handle	private;
	Ev_chain_pd_handle	chain = EV_CHAIN_PRIVATE(views);

	if (delta <= 0)
	    /* BUG ALERT!  We are not handling upper_context auto_scroll. */
	    return;
	/* Scroll views which had old_insert_pos visible, but not new. */
	FORALLVIEWS(views, view) {
	    private = EV_PRIVATE(view);
	    if ((private->state & EV_VS_INSERT_WAS_IN_VIEW) == 0)
		continue /* FORALLVIEWS */;
	    ev_make_visible(view, insert_pos,
	    		    chain->lower_context, chain->auto_scroll_by,
	    		    delta);
	}
}

extern void
ev_input_before(views)
	register Ev_chain	 views;
{
	Ev_chain_pd_handle	 private = EV_CHAIN_PRIVATE(views);

	if (private->lower_context != EV_NO_CONTEXT) {
	    ev_check_insert_visibility(views);
	}
}

extern int
ev_input_partial(views, buf, buf_len)
	register Ev_chain	 views;
	char			*buf;
	long int		 buf_len;
{
	register Ev_chain_pd_handle
				 private = EV_CHAIN_PRIVATE(views);
	register Es_index	 new_insert_pos, old_insert_pos;
	int			 used;

	/* We cannot assume where the esh is positioned, so position first. */
	old_insert_pos = es_set_position(views->esh, private->insert_pos);
	if (old_insert_pos != private->insert_pos) {
	    return(ES_CANNOT_SET);
	}
	new_insert_pos = es_replace(views->esh, old_insert_pos, buf_len, buf,
				    &used);
	if (new_insert_pos == ES_CANNOT_SET || used != buf_len) {
	    return(ES_CANNOT_SET);
	}
	private->insert_pos = new_insert_pos;
	return(0);
}

extern int
ev_input_after(views, old_insert_pos, old_length)
	register Ev_chain	 views;
	Es_index		 old_insert_pos, old_length;
{
	Ev_chain_pd_handle	 private = EV_CHAIN_PRIVATE(views);
	Es_index		 delta = private->insert_pos - old_insert_pos;

	/* Update all of the views' data structures */
	ev_update_after_edit(
		views, old_insert_pos, delta, old_length, old_insert_pos);
	if (private->lower_context != EV_NO_CONTEXT) {
	    ev_scroll_if_old_insert_visible(views, private->insert_pos,
				 	    delta);
	}
	return(delta);
}

#ifdef EXAMPLE
extern int
ev_process_input(views, buf, buf_len)
	register Ev_chain	 views;
	char			*buf;
	int			 buf_len;
{
	Ev_chain_pd_handle	 private = EV_CHAIN_PRIVATE(views);
	Es_index		 old_length = es_get_length(views->esh);
	Es_index	 	 old_insert_pos = private->insert_pos;
	int			 delta;

	ev_input_before(views);
	delta = ev_input_partial(views, buf, buf_len);
	if (delta != ES_CANNOT_SET)
	    delta = ev_input_after(views, old_insert_pos, old_length);
	return(delta);
}
#endif

