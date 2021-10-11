#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)textsw_edit.c 1.1 92/07/30";
#endif
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Programming interface to editing facilities of text subwindows.
 */

#include <suntool/primal.h>
#include <suntool/textsw_impl.h>
#include <suntool/alert.h>
#include <suntool/frame.h>

extern void		ev_input_before();
extern void		textsw_notify_replaced();
pkg_private Seln_rank	textsw_acquire_seln();
extern Textsw_index	textsw_replace();
static int		update_scrollbar();

pkg_private Es_handle
textsw_esh_for_span(view, first, last_plus_one, to_recycle)
	Textsw_view	view;
	Es_index	first, last_plus_one;
	Es_handle	to_recycle;
{
	Es_handle	esh = FOLIO_FOR_VIEW(view)->views->esh;

	return((Es_handle)LINT_CAST(
		es_get5(esh, ES_HANDLE_FOR_SPAN, first, last_plus_one,
			to_recycle, 0, 0) ));
}

pkg_private int
textsw_adjust_delete_span(folio, first, last_plus_one)
	register Textsw_folio	 folio;
	register Es_index	*first, *last_plus_one;
/* Returns:
 *	TXTSW_PE_EMPTY_INTERVAL iff *first < *last_plus_one, else
 *	TEXTSW_PE_READ_ONLY iff NOTHING should be deleted, else
 *	TXTSW_PE_ADJUSTED iff *first adjusted to reflect the constraint
 *	  imposed by folio->read_only_boundary, else
 *	0.
 */
{
	if (*first >= *last_plus_one)
	    return(TXTSW_PE_EMPTY_INTERVAL);
	if TXTSW_IS_READ_ONLY(folio)
	    return(TEXTSW_PE_READ_ONLY);
	if (!EV_MARK_IS_NULL(&folio->read_only_boundary)) {
	    register Es_index	mark_at;
	    mark_at = textsw_find_mark_internal(folio,
						    folio->read_only_boundary);
	    if AN_ERROR(mark_at == ES_INFINITY)
		return(0);
	    if (*last_plus_one <= mark_at)
		return(TEXTSW_PE_READ_ONLY);
	    if (*first < mark_at) {
		*first = mark_at;
		return(TXTSW_PE_ADJUSTED);
	    }
	}
	return(0);
}

pkg_private int
textsw_is_out_of_memory(folio, status)
	Textsw_folio	 folio;
	Es_status	 status;
{
	return ((status == ES_SHORT_WRITE) &&
		(ES_TYPE_MEMORY ==
		   (Es_enum)LINT_CAST(es_get(
		    (Es_handle)LINT_CAST(es_get(folio->views->esh,
						ES_PS_SCRATCH)),
		     ES_TYPE))));
}	

pkg_private int
textsw_esh_failed_msg(view, preamble)
	Textsw_view	 view;
	char		*preamble;
{
	Textsw_folio	 folio = FOLIO_FOR_VIEW(view);
	Es_status	 status;

	status = (Es_status)LINT_CAST(
			es_get(folio->views->esh, ES_STATUS) );
	switch (status) {
	  case ES_SHORT_WRITE:
	    if (textsw_is_out_of_memory(folio, status)) {
		Event	alert_event;

		(void) alert_prompt(
			(Frame)window_get(WINDOW_FROM_VIEW(view), WIN_OWNER),
			&alert_event,
			ALERT_MESSAGE_STRINGS,
			    (strlen(preamble)) ? preamble : "Action failed -",
			    "The memory buffer is full.",
			    "If this is an isolated case, you can circumvent",
			    "this condition by undoing the operation you just",
			    "performed, storing the contents of the subwindow",
			    "to a file using the text menu, and then redoing",
			    "the operation.  Or, you can enlarge the size of",
			    "this buffer by using defaultsedit to increase",
			    "the value of /Text/Memory_Maximum, and then",
			    "restarting your tool.",
			    0,
			ALERT_BUTTON_YES,	"Continue",
			ALERT_TRIGGER,		ACTION_STOP,
			0);
		break;
	    }
	    /* else fall through */
	  case ES_CHECK_ERRNO:
	  case ES_CHECK_FERROR:
	  case ES_FLUSH_FAILED:
	  case ES_FSYNC_FAILED:
	  case ES_SEEK_FAILED: {
	    Event	alert_event;

	    (void) alert_prompt(
		    (Frame)window_get(WINDOW_FROM_VIEW(view), WIN_OWNER),
		    &alert_event,
		    ALERT_MESSAGE_STRINGS,
			(strlen(preamble)) ? preamble : "Action failed -",
			"A problem with the file system has been detected.",
			"File system is probably full.",
			0,
		    ALERT_BUTTON_YES,	"Continue",
		    ALERT_TRIGGER,	ACTION_STOP,
		    0);
	    }
	    break;
	  case ES_REPLACE_DIVERTED:
	    break;
	  default:
	    break;
	}
}

pkg_private Es_index
textsw_delete_span(view, first, last_plus_one, flags)
	Textsw_view		view;
	Es_index		first, last_plus_one;
	register unsigned	flags;
/* Returns the change in indices resulting from the operation.  Result is:
 *	a) usually < 0,
 *	b) 0 if span is empty or in a read_only area,
 *	c) ES_CANNOT_SET if ev_delete_span fails
 */
{
	register Textsw_folio	folio = FOLIO_FOR_VIEW(view);
	Es_index		result;

	result = (flags & TXTSW_DS_ADJUST)
		 ? textsw_adjust_delete_span(folio, &first, &last_plus_one)
		 : (first >= last_plus_one) ? TXTSW_PE_EMPTY_INTERVAL : 0;
	switch (result) {
	  case TEXTSW_PE_READ_ONLY:
	  case TXTSW_PE_EMPTY_INTERVAL:
	    result = 0;
	    break;
	  case TXTSW_PE_ADJUSTED:
	    if (flags & TXTSW_DS_CLEAR_IF_ADJUST(0)) {
		textsw_set_selection(VIEW_REP_TO_ABS(view),
				     ES_INFINITY, ES_INFINITY,
				     EV_SEL_BASE_TYPE(flags));
	    }
	    /* Fall through to do delete on remaining span. */
	  default:
	    if (flags & TXTSW_DS_SHELVE) {
		folio->trash = textsw_esh_for_span(view, first, last_plus_one,
						   folio->trash);
		textsw_acquire_seln(folio, SELN_SHELF);
	    }
	    switch (ev_delete_span(folio->views, first, last_plus_one,
				   &result)) {
	      case 0:
		if (flags & TXTSW_DS_RECORD) {
		    textsw_record_delete(folio);
		}
		break;
	      case 3:
		textsw_esh_failed_msg(view, "Deletion failed - ");
		/* Fall through */
	      default:
		result = ES_CANNOT_SET;
		break;
	    }
	    break;
	}
	return(result);
}

pkg_private Es_index
textsw_do_pending_delete(view, type, flags)
	Textsw_view		view;
	unsigned		type;
	int			flags;
{
	register Textsw_folio	folio = FOLIO_FOR_VIEW(view);
	int			is_pending_delete;
	Es_index		first, last_plus_one, delta, insert;

	is_pending_delete = (EV_SEL_PENDING_DELETE &
	    ev_get_selection(folio->views, &first, &last_plus_one, type) );
	if (first >= last_plus_one)
	    return(0);
	textsw_take_down_caret(folio);
	insert = (flags & TFC_INSERT) ? ev_get_insert(folio->views) : first;
	if (is_pending_delete &&
	    (first <= insert) && (insert <= last_plus_one)) {
	    delta = textsw_delete_span(view, first, last_plus_one,
					TXTSW_DS_ADJUST|TXTSW_DS_SHELVE|
					TXTSW_DS_CLEAR_IF_ADJUST(type));
	} else {
	    if (flags & TFC_SEL) {
		textsw_set_selection(VIEW_REP_TO_ABS(view),
				     ES_INFINITY, ES_INFINITY, type);
	    }
	    delta = 0;
	}
	return(delta);
}

extern Textsw_index
textsw_delete(abstract, first, last_plus_one)
	Textsw		abstract;
	Es_index	first, last_plus_one;
{
	Textsw_view	view = VIEW_ABS_TO_REP(abstract);
	Textsw_folio	folio = FOLIO_FOR_VIEW(view);
	int		result;

	textsw_take_down_caret(folio);
	result = textsw_delete_span(view, first, last_plus_one,
				    TXTSW_DS_ADJUST|TXTSW_DS_SHELVE);
	if (result == ES_CANNOT_SET) {
	    result = 0;
	} else {
	    result = -result;
	}
	return(result);
}

extern Textsw_index
textsw_erase(abstract, first, last_plus_one)
	Textsw		abstract;
	Es_index	first, last_plus_one;
/* This routine is identical to textsw_delete EXCEPT it does not affect the
 * contents of the shelf (useful for client implementing ^W/^U or mailtool).
 */
{
	Textsw_view	view = VIEW_ABS_TO_REP(abstract);
	Textsw_folio	folio = FOLIO_FOR_VIEW(view);
	int		result;

	textsw_take_down_caret(folio);
	result = textsw_delete_span(view, first, last_plus_one,
				    TXTSW_DS_ADJUST);
	if (result == ES_CANNOT_SET) {
	    result = 0;
	} else {
	    result = -result;
	}
	return(result);
}

pkg_private int
textsw_do_insert_makes_visible(view)
	register Textsw_view	view;
{
	extern int		ev_insert_visible_in_view();
	register Textsw_folio	folio = FOLIO_FOR_VIEW(view);
	int			lower_context;

	if ((folio->insert_makes_visible == TEXTSW_ALWAYS) &&
	    (folio->state & TXTSW_DOING_EVENT) &&
	    (!ev_insert_visible_in_view(view->e_view)) ) {
	    lower_context = (int)LINT_CAST(
			    ev_get((Ev_handle)folio->views,
				   EV_CHAIN_LOWER_CONTEXT));
	    textsw_normalize_internal(view, ev_get_insert(folio->views),
		ES_INFINITY, 0, lower_context,
		TXTSW_NI_AT_BOTTOM|TXTSW_NI_NOT_IF_IN_VIEW|TXTSW_NI_MARK);
	}

}

extern void
textsw_insert_makes_visible(textsw)
	Textsw		textsw;
{
	Textsw_view	        view = VIEW_ABS_TO_REP(textsw);
	register Textsw_folio   folio = FOLIO_FOR_VIEW(view);
	int			old_insert_makes_visible = folio->insert_makes_visible;
	int			old_state = folio->state;

	folio->insert_makes_visible = TEXTSW_ALWAYS;
	folio->state |= TXTSW_DOING_EVENT;
	
	textsw_do_insert_makes_visible(view);
	
	folio->insert_makes_visible = old_insert_makes_visible;
	folio->state = old_state;

}

pkg_private int
textsw_do_edit(view, unit, direction)
	register Textsw_view	view;
	unsigned		unit, direction;
{
	extern struct ei_span_result
				ev_span_for_edit();
	register Textsw_folio	folio = FOLIO_FOR_VIEW(view);
	struct ei_span_result	span;
	int			delta;

	span = ev_span_for_edit(folio->views, (int)(unit|direction));
	
		 
	if ((span.flags>>16) == 0) {
	
	    /* Don't join with next line for ERASE_LINE_END */
	
	    if ((unit == EV_EDIT_LINE) && (direction == 0)) {
	        Es_index	file_length = es_get_length(folio->views->esh);
	        
		if (span.last_plus_one < file_length)
	            span.last_plus_one--;
	    }
	    
	    delta = textsw_delete_span(view, span.first, span.last_plus_one,
				       TXTSW_DS_ADJUST);
	    if (delta == ES_CANNOT_SET) {
		delta = 0;
	    } else {
		textsw_do_insert_makes_visible(view);
		textsw_record_edit(folio, unit, direction);
		delta = -delta;
	    }
	} else
	    delta = 0;
	return(delta);
}

extern Textsw_index
textsw_edit(abstract, unit, count, direction)
	Textsw			abstract;
	register unsigned	unit, count;
	unsigned		direction;
{
	Textsw_view		view = VIEW_ABS_TO_REP(abstract);
	Textsw_folio		folio = FOLIO_FOR_VIEW(view);
	int			result = 0;

	if (direction) direction = EV_EDIT_BACK;
	switch (unit) {
	  case TEXTSW_UNIT_IS_CHAR:
	    unit = EV_EDIT_CHAR;
	    break;
	  case TEXTSW_UNIT_IS_WORD:
	    unit = EV_EDIT_WORD;
	    break;
	  case TEXTSW_UNIT_IS_LINE:
	    unit = EV_EDIT_LINE;
	    break;
	  default:
	    return 0;
	}
	textsw_take_down_caret(folio);
	for (; count; count--) {
	    result += textsw_do_edit(view, unit, direction);
	}
	return(result);
}

pkg_private void
textsw_input_before(view, old_insert_pos, old_length)
	Textsw_view	 view;
	Es_index	*old_insert_pos, *old_length;
{
	Textsw_folio	 	folio = FOLIO_FOR_VIEW(view);
	register Ev_chain	chain = folio->views;

	*old_length = es_get_length(chain->esh);
	*old_insert_pos = ev_get_insert(chain);
	ev_input_before(chain);
}

pkg_private int
textsw_input_partial(view, buf, buf_len)
	Textsw_view		 view;
	char			*buf;
	long int		 buf_len;
{
	int			 status;

	status = ev_input_partial(FOLIO_FOR_VIEW(view)->views, buf, buf_len);
	if (status) {
	    textsw_esh_failed_msg(view, "Insertion failed - ");
	}
        return(status);
}

pkg_private Es_index
textsw_input_after(view, old_insert_pos, old_length, record)
	Textsw_view		view;
	Es_index		old_insert_pos, old_length;
	int			record;
{
	register Textsw_folio	folio = FOLIO_FOR_VIEW(view);
	Es_index		delta;

	delta = ev_input_after(folio->views, old_insert_pos, old_length);
	if (delta != ES_CANNOT_SET) {
	    textsw_do_insert_makes_visible(view);
	    if (record) {
		Es_handle	pieces;
		pieces = textsw_esh_for_span(folio->first_view,
				old_insert_pos, old_insert_pos+delta, ES_NULL);
		textsw_record_piece_insert(folio, pieces);
	    }
	    if ((folio->state & TXTSW_EDITED) == 0)
		textsw_possibly_edited_now_notify(folio);
	    if (folio->notify_level & TEXTSW_NOTIFY_EDIT) {
		textsw_notify_replaced((Textsw_opaque)folio->first_view, 
			      old_insert_pos, old_length, old_insert_pos,
			      old_insert_pos, delta);
	    }
	    (void) textsw_checkpoint(folio);
	}
	return(delta);
}

pkg_private Es_index
textsw_do_input(view, buf, buf_len, flag)
	Textsw_view		 view;
	char			*buf;
	long int		 buf_len;
	unsigned		 flag;
{
	register Textsw_folio	folio = FOLIO_FOR_VIEW(view);
	register Ev_chain	chain = folio->views;
	int			record;
	Es_index		delta, old_insert_pos, old_length;

	textsw_input_before(view, &old_insert_pos, &old_length);
	delta = textsw_input_partial(view, buf, buf_len);
	if (delta == ES_CANNOT_SET)
	    return(0);
	record = (TXTSW_DO_AGAIN(folio) &&
		  ((folio->func_state & TXTSW_FUNC_AGAIN) == 0) );
	delta = textsw_input_after(view, old_insert_pos, old_length,
				   record && (buf_len > 100));
	if (delta == ES_CANNOT_SET)
	    return(0);
	    
	if ((int)ev_get(chain, EV_CHAIN_DELAY_UPDATE) == 0) { 
	    ev_update_chain_display(chain);
	       
	    if (flag & TXTSW_UPDATE_SCROLLBAR) 
	        textsw_update_scrollbars(folio, TEXTSW_VIEW_NULL);
	    else if ((flag & TXTSW_UPDATE_SCROLLBAR_IF_NEEDED) &&  
	              update_scrollbar(delta, old_length, buf, buf_len)) 
	               textsw_update_scrollbars(folio, TEXTSW_VIEW_NULL);
	}	    
	    
	if (record && (buf_len <= 100))
	    textsw_record_input(folio, buf, buf_len);
	return(delta);
}

extern Textsw_index
textsw_insert(abstract, buf, buf_len)
	Textsw			abstract;
	char			*buf;
	long int		 buf_len;
{
	Textsw_view		 view = VIEW_ABS_TO_REP(abstract);
	register Textsw_folio	 folio = FOLIO_FOR_VIEW(view);
	Es_index		 result;

	textsw_take_down_caret(folio);
	result = textsw_do_input(view, buf, buf_len, 
	         TXTSW_UPDATE_SCROLLBAR_IF_NEEDED);
	return(result);
}

extern Textsw_index
textsw_replace_bytes(abstract, first, last_plus_one, buf, buf_len)
	Textsw			 abstract;
	Es_index		 first, last_plus_one;
	char			*buf;
	long int		 buf_len;
/* This routine is a placeholder that can be documented without casting the
 * calling sequence to textsw_replace (the preferred name) in concrete.
 */
{
	return(textsw_replace(abstract, first, last_plus_one, buf, buf_len));
}

extern Textsw_index
textsw_replace(abstract, first, last_plus_one, buf, buf_len)
	Textsw			 abstract;
	Es_index		 first, last_plus_one;
	char			*buf;
	long int		 buf_len;
{
	extern void		textsw_remove_mark_internal();
	pkg_private Ev_mark_object
				 textsw_add_mark_internal();
	Ev_mark_object		 saved_insert_mark;
	Es_index		 saved_insert;
	Textsw_view		 view = VIEW_ABS_TO_REP(abstract);
	register Textsw_folio	 folio = FOLIO_FOR_VIEW(view);
	register Ev_chain	 chain = folio->views;
	Es_index		 result, insert_result;
	int			 lower_context;

	insert_result = 0;
	textsw_take_down_caret(folio);
	/* BUG ALERT: change this to avoid the double paint. */
	if (first < last_plus_one) {
	    ev_set(0, chain, EV_CHAIN_DELAY_UPDATE, TRUE, 0);
	    result = textsw_delete_span(view, first, last_plus_one,
					TXTSW_DS_ADJUST);
	    ev_set(0, chain, EV_CHAIN_DELAY_UPDATE, FALSE, 0);
	    if (result == ES_CANNOT_SET) {
		if (ES_REPLACE_DIVERTED == (Es_status)LINT_CAST(
			es_get(chain->esh, ES_STATUS) )) {
		    result = 0;
		}
	    }
	} else {
	    result = 0;
	}
	if (result == ES_CANNOT_SET) {
	    result = 0;
	} else {
	    ev_check_insert_visibility(chain);
	    lower_context =
	        (int) ev_get(chain, EV_CHAIN_LOWER_CONTEXT);
	    ev_set(0, chain,
	        EV_CHAIN_LOWER_CONTEXT, EV_NO_CONTEXT, 0);

	    saved_insert = ev_get_insert(chain);
	    saved_insert_mark =
		textsw_add_mark_internal(folio, saved_insert,
					 TEXTSW_MARK_MOVE_AT_INSERT);
	    (void) ev_set_insert(chain, first);
	    insert_result += textsw_do_input(view, buf, buf_len, 
	                     TXTSW_DONT_UPDATE_SCROLLBAR);
	    result += insert_result;
	    saved_insert = textsw_find_mark_internal(folio, saved_insert_mark);
	    if AN_ERROR(saved_insert == ES_INFINITY) {
	    } else
		(void) ev_set_insert(chain, saved_insert);
	    textsw_remove_mark_internal(folio, saved_insert_mark);

	    ev_set(0, chain,
	        EV_CHAIN_LOWER_CONTEXT, lower_context, 0);
	    ev_scroll_if_old_insert_visible(chain,
	        saved_insert, insert_result);
	    textsw_update_scrollbars(folio, TEXTSW_VIEW_NULL);    
	}
	return(result);
}

static int
update_scrollbar(delta, old_length)
	long int	delta, old_length;
	
{
	long int	THRESHOLD = 20;
	
	return ((THRESHOLD * delta) >= old_length);
	
	
}
