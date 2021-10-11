#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)textsw_get_key.c 1.1 92/07/30";
#endif
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * GET key processing.
 */

#include <suntool/primal.h>
#include <suntool/textsw_impl.h>
#include <suntool/ev_impl.h>	/* For declaration of ev_add_finder */
#include <errno.h>

extern int		errno;
extern Es_index		ev_get_insert();
extern Es_index		ev_set_insert();

static void		textsw_do_get();
pkg_private Es_index	textsw_find_mark_internal();
pkg_private Es_index	textsw_insert_pieces();

pkg_private int
textsw_begin_get(view)
	Textsw_view	 view;
{
	register Textsw_folio	 textsw = FOLIO_FOR_VIEW(view);

	textsw_begin_function(view, TXTSW_FUNC_GET);
	ASSUME(EV_MARK_IS_NULL(&textsw->save_insert));
	EV_MARK_SET_MOVE_AT_INSERT(textsw->save_insert);
	ev_add_finger(&textsw->views->fingers, ev_get_insert(textsw->views),
		      0, &textsw->save_insert);
	(void) textsw_inform_seln_svc(textsw, TXTSW_FUNC_GET, TRUE);
}

pkg_private int
textsw_end_get(view)
	register Textsw_view	view;
{
	int			easy, result = 0;
	register Textsw_folio	folio = FOLIO_FOR_VIEW(view);

	easy = textsw_inform_seln_svc(folio, TXTSW_FUNC_GET, FALSE);
	if ((folio->func_state & TXTSW_FUNC_GET) == 0)
	    return(0);
	if ((folio->func_state & TXTSW_FUNC_EXECUTE) == 0)
	    goto Done;
	if (TXTSW_IS_READ_ONLY(folio)) {
	    result = TEXTSW_PE_READ_ONLY;
	    textsw_clear_secondary_selection(folio, EV_SEL_SECONDARY);
	    goto Done;
	}
	textsw_checkpoint_undo(VIEW_REP_TO_ABS(view),
				(caddr_t)TEXTSW_INFINITY-1);
	ASSUME(allock());
	textsw_do_get(view, easy);
	ASSUME(allock());
	textsw_checkpoint_undo(VIEW_REP_TO_ABS(view),
				(caddr_t)TEXTSW_INFINITY-1);
Done:
	if (!EV_MARK_IS_NULL(&folio->save_insert)) {
	    ev_remove_finger(&folio->views->fingers, folio->save_insert);
	    (void) ev_init_mark(&folio->save_insert);
	}
	textsw_end_function(view, TXTSW_FUNC_GET);
	
	textsw_update_scrollbars(folio, TEXTSW_VIEW_NULL);
	
	return(result);
}

pkg_private Es_index
textsw_read_only_boundary_is_at(folio)
	register Textsw_folio	folio;
{
	register Es_index	result;

	if (EV_MARK_IS_NULL(&folio->read_only_boundary)) {
	    result = 0;
	} else {
	    result = textsw_find_mark_internal(folio,
					folio->read_only_boundary);
	    if AN_ERROR(result == ES_INFINITY)
		result = 0;
	}
	return(result);
}

static void
textsw_do_get(view, local_operands)
	register Textsw_view	 view;
	int			 local_operands;
{
	/*
	 * The following table indicates what the final contents of the
	 *   trashbin should be based on the modes of the primary and secondary
	 *   selections.  P-d is short for pending-delete.
	 * An empty primary selection is treated as not pending-delete.
	 *
	 *			    Primary
	 *			 ~P-d	 P-d
	 *	Secondary	=================
	 *	  Empty		| Tbin	| Pri.	|
	 *	  ~P-d		| Tbin	| Pri.	|
	 *	   P-d		| Sec.	| Pri.	|
	 *			=================
	 */
	extern void		 ev_check_insert_visibility(),
				 ev_scroll_if_old_insert_visible();
	extern int		 ev_get_selection();
	extern Es_handle	 textsw_esh_for_span();
	register Textsw_folio	 folio = FOLIO_FOR_VIEW(view);
	register Ev_chain	 views = folio->views;
	register Es_handle	 secondary;
	register int		 is_pending_delete;
	int			 end_with_sec_as_trash = FALSE;
	int			 lower_context = (int)
				 ev_get(views, EV_CHAIN_LOWER_CONTEXT);
	int			 acquire_shelf = FALSE;
	int			 was_secondary_selection;
	Es_index		 delta, first, last_plus_one, ro_bdry;

	/*
	 * First, pre-process the primary selection.
	 */
	ev_set(0, views, EV_CHAIN_DELAY_UPDATE, TRUE, 0);
	ro_bdry = textsw_read_only_boundary_is_at(folio);
	is_pending_delete = (EV_SEL_PENDING_DELETE & ev_get_selection(
		views, &first, &last_plus_one, EV_SEL_PRIMARY) );
	if (last_plus_one <= ro_bdry) {
	    is_pending_delete = 0;
	}
	if ((first < last_plus_one) && is_pending_delete) {
	    /*
	     * A non-empty pending-delete primary selection exists.
	     * It must be the contents of the trashbin when we are done.
	     */
	    secondary = folio->trash;		/* Recycle old trash pieces */
	    folio->trash =
		textsw_esh_for_span(view, first, last_plus_one, ES_NULL);
	    acquire_shelf = TRUE;
	} else {
	    secondary = ES_NULL;
	    if (local_operands)
		end_with_sec_as_trash = TRUE;
	}
	/*
	 * Second, completely process local secondary selection.
	 */
	if (local_operands) {
	    is_pending_delete = (EV_SEL_PENDING_DELETE & ev_get_selection(
		    views, &first, &last_plus_one, EV_SEL_SECONDARY) );
	    if (last_plus_one <= ro_bdry) {
		is_pending_delete = 0;
	    }
	    if (was_secondary_selection = (first < last_plus_one)) {
		/* A non-empty secondary selection exists */
		if (is_pending_delete == 0) {
		    end_with_sec_as_trash = FALSE;
		} else if (end_with_sec_as_trash) {
		    secondary = folio->trash;	/* Recycle old trash pieces */
		}
		secondary = textsw_esh_for_span(
			      view, first, last_plus_one, secondary);
		if (is_pending_delete) {
		    ev_delete_span(views,
				   (first < ro_bdry) ? ro_bdry : first,
				   last_plus_one, &delta);
		}
	    } else {
		if (end_with_sec_as_trash) {
		    secondary = folio->trash;
		}
	    }
	    if (first != ES_INFINITY) {
		textsw_set_selection(VIEW_REP_TO_ABS(view),
				     ES_INFINITY, ES_INFINITY,
				     EV_SEL_SECONDARY);
	    }
	}
	/*
	 * Third, post-process the primary selection.
	 */
	is_pending_delete = (EV_SEL_PENDING_DELETE & ev_get_selection(
		views, &first, &last_plus_one, EV_SEL_PRIMARY) );
	if (first < last_plus_one) {
	    /*
	     * We still have a non-empty primary selection (it could have been
	     *   deleted because of the secondary selection but was not).
	     */
	    if (is_pending_delete && (ro_bdry < last_plus_one)) {
		ev_delete_span(views,
			       (first < ro_bdry) ? ro_bdry : first,
			       last_plus_one, &delta);
	    }
	}
	if (first != ES_INFINITY) {
	    textsw_set_selection(VIEW_REP_TO_ABS(view),
				 ES_INFINITY, ES_INFINITY, EV_SEL_PRIMARY);
	}
	/*
	 * Fourth, insert the text being gotten.
	 */
	ev_set(0, views, EV_CHAIN_DELAY_UPDATE, FALSE, 0);
	if (local_operands) {
	    first = ev_set_insert(folio->views, textsw_get_saved_insert(folio));
	    if AN_ERROR(first == ES_INFINITY) {
		if (secondary && (secondary != folio->trash))
		    es_destroy(secondary);
		return;
	    }
	    if (lower_context != EV_NO_CONTEXT) {
		ev_check_insert_visibility(views);
	    }
	    last_plus_one = textsw_insert_pieces(view, first, secondary);
	} else {
	    first = ev_get_insert(views);
	    was_secondary_selection =
		(textsw_seln_svc_had_secondary(folio))
		? TRUE : FALSE;
	    if (lower_context != EV_NO_CONTEXT) {
		ev_check_insert_visibility(views);
	    }
	    /* Note: textsw_stuff_selection uses routines that record the
	     *   insertion for AGAIN, so we need not worry about that.
	     */
	    textsw_stuff_selection(view, (unsigned) (was_secondary_selection ?
		EV_SEL_SECONDARY : EV_SEL_SHELF));
	    last_plus_one = ev_get_insert(views);
	    if (was_secondary_selection)
	        (void) seln_ask(&folio->selection_func.secondary,
				SELN_REQ_COMMIT_PENDING_DELETE,
				0);
	    if (secondary) {
		ASSUME(secondary != folio->trash);
		es_destroy(secondary);
		secondary = ES_NULL;
	    }
	}
	ev_update_chain_display(views);
	if (lower_context != EV_NO_CONTEXT) {
	    ev_scroll_if_old_insert_visible(folio->views,
					    last_plus_one,
					    last_plus_one - first);
	}
	if (end_with_sec_as_trash) {
	    folio->trash = secondary;
	    acquire_shelf = TRUE;
	    if (TXTSW_DO_AGAIN(folio)) {
		if (was_secondary_selection) {
		    secondary = textsw_esh_for_span(
				view, first, last_plus_one, ES_NULL);
		    textsw_record_piece_insert(folio, secondary);
		} else {
		    textsw_record_trash_insert(folio);
		}
	    }
	} else if (secondary) {
	    int		destroy_pieces = TRUE;
	    if (TXTSW_DO_AGAIN(folio)) {
		if (was_secondary_selection) {
		    textsw_record_piece_insert(folio, secondary);
		    destroy_pieces = FALSE;
		} else {
		    textsw_record_trash_insert(folio);
		}
	    }
	    if (destroy_pieces) {
		es_destroy(secondary);
	    }
	}
	if (acquire_shelf)
	    textsw_acquire_seln(folio, SELN_SHELF);
}

pkg_private Es_index
textsw_insert_pieces(view, pos, pieces)
	Textsw_view		view;
	register Es_index	pos;
	Es_handle		pieces;
{
	extern Es_index		ev_get_insert(),
				ev_set_insert(),
				textsw_set_insert();
	register Textsw_folio	folio = FOLIO_FOR_VIEW(view);
	register Ev_chain	chain = folio->views;
	Es_index		delta,
				old_insert_pos,
				old_length = es_get_length(chain->esh),
				new_insert_pos;

	if (pieces == ES_NULL)
	    return(pos);
	if (folio->notify_level & TEXTSW_NOTIFY_EDIT)
	    old_insert_pos = ev_get_insert(chain);
	(void) ev_set_insert(chain, pos);
	    /* Required since es_set(ES_HANDLE_TO_INSERT) bypasses ev code. */
	es_set(chain->esh, ES_HANDLE_TO_INSERT, pieces, 0);
	new_insert_pos = es_get_position(chain->esh);
	(void) textsw_set_insert(folio, new_insert_pos);
	delta = new_insert_pos - pos;
	/* The esh may simply swallow the pieces (in the cmdsw), so check
	 * to see if any change actually occurred.
	 */
	if (delta) {
	    ev_update_after_edit(chain, pos, delta, old_length, pos);
	    if (folio->notify_level & TEXTSW_NOTIFY_EDIT) {
		textsw_notify_replaced((Textsw_opaque)folio->first_view,
				old_insert_pos, old_length, pos, pos, delta);
	    }
	    textsw_checkpoint(folio);
	}
	return(new_insert_pos);
}

pkg_private int
textsw_function_get(view)
	Textsw_view	view;
{
	int		result;

	textsw_begin_get(view);
	result = textsw_end_get(view);
	return(result);
}

pkg_private int
textsw_put_then_get(view)
    Textsw_view			view;
{
    extern Es_handle		textsw_esh_for_span();
    register Textsw_folio	textsw = FOLIO_FOR_VIEW(view);
    Es_index			first, last_plus_one, insert;
    register int		is_pending_delete;
    int				seln_nonzero;

    if (seln_nonzero = textsw_is_seln_nonzero(textsw, EV_SEL_PRIMARY)) {
	textsw_checkpoint_undo(VIEW_REP_TO_ABS(view),
				(caddr_t)TEXTSW_INFINITY-1);
	if (seln_nonzero == 2) {
	    is_pending_delete = (EV_SEL_PENDING_DELETE & 
		ev_get_selection(textsw->views,
			    &first, &last_plus_one, EV_SEL_PRIMARY));
	    if (first < last_plus_one) {
		insert = ev_get_insert(textsw->views);
		textsw->trash = textsw_esh_for_span(
			    view, first, last_plus_one, textsw->trash);
		textsw_set_selection(VIEW_REP_TO_ABS(view),
			    ES_INFINITY, ES_INFINITY, EV_SEL_PRIMARY);
		if (!is_pending_delete ||
		    (insert < first) || (last_plus_one < insert))
		    textsw_insert_pieces(view, insert, textsw->trash);
		textsw_acquire_seln(textsw, SELN_SHELF);
	    }
	} else {
	    /* Item is "Put then Get", but there is a potential race to
	     * the Shelf between us and the Primary Selection holder,
	     * so we Copy primary then Put instead.
	     */ 
	    textsw_stuff_selection(view, EV_SEL_PRIMARY);
	    textsw_put(view);
	}
	textsw_checkpoint_undo(VIEW_REP_TO_ABS(view),
				(caddr_t)TEXTSW_INFINITY-1);
    } else if (textsw_is_seln_nonzero(textsw, EV_SEL_SHELF)) {
	textsw_function_get(view);
    } /* else menu item should have been grayed out! */
}

/*	===============================================================
 *
 *	Misc. marking utilities
 *
 *	===============================================================
 */
pkg_private void	textsw_remove_mark_internal();
 
pkg_private Ev_mark_object
textsw_add_mark_internal(textsw, position, flags)
	Textsw_folio		textsw;
	Es_index		position;
	unsigned		flags;
{
	Ev_mark_object		mark;
	register Ev_mark	mark_to_use;

	if (flags & TEXTSW_MARK_READ_ONLY) {
	    mark_to_use = &textsw->read_only_boundary;
	    textsw_remove_mark_internal(textsw, *mark_to_use);
	} else {
	    mark_to_use = &mark;
	}
	(void) ev_init_mark(mark_to_use);
	if (flags & TEXTSW_MARK_MOVE_AT_INSERT)
	    EV_MARK_SET_MOVE_AT_INSERT(*mark_to_use);
	ev_add_finger(&textsw->views->fingers, position, 0, mark_to_use);
	return(*mark_to_use);
}

extern Textsw_mark
textsw_add_mark(abstract, position, flags)
	Textsw		abstract;
	Es_index	position;
	unsigned	flags;
{
	Textsw_view	view = VIEW_ABS_TO_REP(abstract);
	return((Textsw_mark)textsw_add_mark_internal(
				FOLIO_FOR_VIEW(view), position, flags));
}

pkg_private Es_index
textsw_find_mark_internal(textsw, mark)
	Textsw_folio	textsw;
	Ev_mark_object	mark;
{
	Ev_finger_handle	finger;

	finger = ev_find_finger(&textsw->views->fingers, mark);
	return(finger ? finger->pos : ES_INFINITY);
}

extern Textsw_index
textsw_find_mark(abstract, mark)
	Textsw		abstract;
	Textsw_mark	mark;
{
	Textsw_view	view = VIEW_ABS_TO_REP(abstract);
	Ev_mark_object *dummy_for_compiler = (Ev_mark_object *)&mark;

#ifdef	lint
	view->magic = *dummy_for_compiler; /* To get rid of unused msg */
	return((Textsw_index)0);
#else	lint
	return((Textsw_index)textsw_find_mark_internal(FOLIO_FOR_VIEW(view),
					 *dummy_for_compiler));
#endif	lint
}

pkg_private void
textsw_remove_mark_internal(textsw, mark)
	Textsw_folio	textsw;
	Ev_mark_object	mark;
{
	if (!EV_MARK_IS_NULL(&mark)) {
	    if (EV_MARK_ID(mark) == EV_MARK_ID(textsw->read_only_boundary)) {
		ev_init_mark(&textsw->read_only_boundary);
	    }
	    ev_remove_finger(&textsw->views->fingers, mark);
	}
}
 
extern void
textsw_remove_mark(abstract, mark)
	Textsw		abstract;
	Textsw_mark	mark;
{
	Textsw_view	view = VIEW_ABS_TO_REP(abstract);
	long unsigned	*dummy_for_compiler = (long unsigned *)&mark;

	textsw_remove_mark_internal(FOLIO_FOR_VIEW(view),
				    *((Ev_mark)dummy_for_compiler));
}
 
