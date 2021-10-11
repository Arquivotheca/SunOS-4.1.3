#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)textsw_find.c 1.1 92/07/30";
#endif
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Procedures to do searching for patterns in text subwindows.
 */

#include <suntool/primal.h>
#include <suntool/textsw_impl.h>
#include <suntool/frame.h>

pkg_private int
textsw_begin_find(view)
	register Textsw_view	 view;
{
	textsw_begin_function(view, TXTSW_FUNC_FIND);
	(void) textsw_inform_seln_svc(FOLIO_FOR_VIEW(view),
				      TXTSW_FUNC_FIND, TRUE);
}

pkg_private int
textsw_end_find(view, event_code, x, y)
	register Textsw_view	view;
	int			x, y;
	unsigned		event_code;
{
	static void		textsw_find_selection_and_normalize();
	extern void		textsw_create_search_frame();
	register Textsw_folio	folio = FOLIO_FOR_VIEW(view);

	(void) textsw_inform_seln_svc(folio, TXTSW_FUNC_FIND, FALSE);
	if ((folio->func_state & TXTSW_FUNC_FIND) == 0)
	    return(ES_INFINITY);
	if ((folio->func_state & TXTSW_FUNC_EXECUTE) == 0)
	    goto Done;
	    
	if (event_code == TXTSW_REPLACE) {    
	    (void) textsw_create_search_frame(view);
	    
	} else {
	    textsw_find_selection_and_normalize(
	        view, x, y,
	        (long unsigned)(TFSAN_SHELF_ALSO | (
	            (event_code ==  TXTSW_FIND_BACKWARD) ? TFSAN_BACKWARD : 0 )) );
	}
Done:
	textsw_end_function(view, TXTSW_FUNC_FIND);
	return(0);
}

pkg_private void
textsw_find_selection_and_normalize(view, x, y, options)
	register Textsw_view	 view;
	int			 x, y;
	register long unsigned	 options;
{
	register Es_index	 primary_first, primary_last_plus_one;
	Textsw_selection_object	 selection;
	char			 buf[2096];
	unsigned		 flags;
	int			 try_shelf = FALSE;
	register Textsw_folio	 textsw = FOLIO_FOR_VIEW(view);

	textsw_init_selection_object(
	    textsw, &selection, buf, sizeof(buf), FALSE);
	if (EV_SEL_BASE_TYPE(options)) {
	    selection.type = textsw_func_selection_internal(
			textsw, &selection, EV_SEL_BASE_TYPE(options),
			TFS_FILL_ALWAYS);
	    switch (selection.type) {
	      case TFS_SELN_SVC_ERROR:
		return;
	      default:
		if (TFS_IS_ERROR(selection.type) ||
		    (selection.last_plus_one <= selection.first)) {
		    if (EV_SEL_BASE_TYPE(options) == EV_SEL_SHELF)
			return;
		    try_shelf = TRUE;
		}
		break;
	    }
	} else if (TFS_IS_ERROR(textsw_func_selection(textsw, &selection,
						      TFS_FILL_ALWAYS))) {
	    if (textsw->selection_holder)
		return;
	    try_shelf = TRUE;
	}
	if (try_shelf) {
	    selection.type = textsw_func_selection_internal(
			textsw, &selection, EV_SEL_SHELF, TFS_FILL_ALWAYS);
	    if (TFS_IS_ERROR(selection.type))
		return;
	}
	if ((selection.type & EV_SEL_SHELF) == 0)
	    textsw_clear_secondary_selection(textsw, selection.type);
	flags = (options & TFSAN_BACKWARD)
		? EV_FIND_BACKWARD : EV_FIND_DEFAULT;
	if ((selection.type & TFS_IS_SELF) &&
	    (selection.type & EV_SEL_PRIMARY)) {
	    primary_first = selection.first;
	    primary_last_plus_one = selection.last_plus_one;
	} else {
	    Es_index	dummy_first, dummy_last_plus_one;
	    (void) ev_get_selection(textsw->views, &dummy_first,
				    &dummy_last_plus_one, EV_SEL_PRIMARY);
	    if (dummy_first < dummy_last_plus_one) {
		primary_first = dummy_first;
		primary_last_plus_one = dummy_last_plus_one;
	    } else {
		primary_first = (TXTSW_IS_READ_ONLY(textsw)
				? 0 : ev_get_insert(textsw->views));
		primary_last_plus_one = primary_first;
	    }
	}
	selection.first = (flags == EV_FIND_BACKWARD)
				? primary_first : primary_last_plus_one;
	textsw_find_pattern_and_normalize(
	    view, x, y, &selection.first, &selection.last_plus_one,
	    selection.buf, (unsigned)selection.buf_len, flags);
}

/* Caller must set *first to be position at which to start the search. */
pkg_private int
textsw_find_pattern(textsw, first, last_plus_one, buf, buf_len, flags)
	Textsw_folio	 textsw;
	Es_index	*first, *last_plus_one;
	char		*buf;
	unsigned	 buf_len;
	unsigned	 flags;
{
	Es_handle	 esh = textsw->views->esh;
	Es_index	 start_at = *first;
	int		 i;

	if (buf_len == 0) {
	    *first = ES_CANNOT_SET;
	    return;
	}
	for (i = 0; i < 2; i++) {
	    ev_find_in_esh(esh, buf, buf_len, start_at, 1, flags,
				first, last_plus_one);
	    if (*first != ES_CANNOT_SET) {
		return;
	    }
	    if (flags & EV_FIND_BACKWARD) {
		Es_index	length = es_get_length(esh);
		if (start_at == length) {
		    return;
		}
		start_at = length;
	    } else {
		if (start_at == 0) {
		    return;
		}
		start_at = 0;
	    }
	}
}

/* Caller must set *first to be position at which to start the search. */
/* ARGSUSED */
pkg_private int
textsw_find_pattern_and_normalize(
    view, x, y, first, last_plus_one, buf, buf_len, flags)
	Textsw_view	 view;
	int		 x, y;		/* Currently unused */
	Es_index	*first, *last_plus_one;
	char		*buf;
	unsigned	 buf_len;
	unsigned	 flags;
	
{
	register Textsw_folio	 folio = FOLIO_FOR_VIEW(view);
	Es_index		 pattern_index;

	pattern_index = (flags & EV_FIND_BACKWARD)
			? *first : (*first - buf_len);
	textsw_find_pattern(folio, first, last_plus_one, buf, buf_len, flags);
	if (*first == ES_CANNOT_SET) {
	    (void) window_bell(WINDOW_FROM_VIEW(view));
	} else {
	    if (*first == pattern_index)
		(void) window_bell(WINDOW_FROM_VIEW(view));
	    textsw_possibly_normalize_and_set_selection(
		VIEW_REP_TO_ABS(view), *first, *last_plus_one, EV_SEL_PRIMARY);
	    (void) textsw_set_insert(folio, *last_plus_one);
	    textsw_record_find(folio, buf, (int)buf_len, (int)flags);
	}
}

pkg_private int
textsw_function_find(view, x, y)
	Textsw_view	view;
	int		x, y;
{
	textsw_begin_find(view);
	(void) textsw_end_find(view, x, y);
}

/*
 *  If the pattern is found, return the index where it is found,
 *  else return -1.
 */
extern int
textsw_find_bytes(abstract, first, last_plus_one, buf, buf_len, flags)
	Textsw		 abstract;	/* find in this textsw */
	Es_index	*first;		/* start here, return start of 
					 * found pattern here */
	Es_index	*last_plus_one;	/* return end of found pattern */
	char		*buf;		/* pattern */
	unsigned	 buf_len;	/* pattern length */
	unsigned	 flags;		/* 0=forward, !0=backward */
{
	Textsw_folio	folio = FOLIO_FOR_VIEW(VIEW_ABS_TO_REP(abstract));
	int		save_first = *first;

	textsw_find_pattern(folio, first, last_plus_one, buf, buf_len,
			    (unsigned)(flags ? EV_FIND_BACKWARD : 0));
	if (*first == ES_CANNOT_SET) {
	    *first = save_first;
	    return -1;
	} else {
	    return *first;
	}
}
