#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)textsw_seln_svc.c 1.1 92/07/30";
#endif
#endif

/*
 * Copyright (c) 1986, 1987 by Sun Microsystems, Inc.
 */

/*
 * Textsw interface to selection service.
 */

#include <errno.h>
#include <suntool/primal.h>
#include <suntool/textsw_impl.h>
#include <suntool/alert.h>
#include <suntool/frame.h>
#include "sunwindow/sv_malloc.h"

extern int	 errno;

extern Es_status	es_copy();
static Seln_result	textsw_seln_yield();

char		*shelf_name = "/tmp/textsw_shelf";

#define	SAME_HOLDER(folio, holder)				\
	seln_holder_same_client(holder, folio->first_view)

typedef struct continuation {
	int		in_use, type;
	Es_index	current, first, last_plus_one;
	unsigned	span_level;
} Continuation_object;
typedef Continuation_object *	Continuation;
static Continuation_object	fast_continuation; /* Default init to zeros */

pkg_private void
textsw_clear_secondary_selection(textsw, type)
	register Textsw_folio	textsw;
	unsigned		type;
{
	if (type & EV_SEL_SECONDARY) {
	    if (type & TFS_IS_OTHER) {
		Seln_holder	holder;
		holder = seln_inquire(SELN_SECONDARY);
		if (holder.state != SELN_NONE)
		    (void) seln_ask(&holder,
				    SELN_REQ_YIELD, 0,
				    0);
	    } else {
		textsw_set_selection(VIEW_REP_TO_ABS(textsw->first_view),
				     ES_INFINITY, ES_INFINITY, type);
	    }
	}
}

static unsigned
holder_flag_from_seln_rank(rank)
	register Seln_rank	rank;
{
	switch (rank) {
	  case SELN_CARET:
	    return(TXTSW_HOLDER_OF_CARET);
	  case SELN_PRIMARY:
	    return(TXTSW_HOLDER_OF_PSEL);
	  case SELN_SECONDARY:
	    return(TXTSW_HOLDER_OF_SSEL);
	  case SELN_SHELF:
	    return(TXTSW_HOLDER_OF_SHELF);
	  case SELN_UNSPECIFIED:
	    return(0);
	  default:
	    LINT_IGNORE(ASSUME(0));
	    return(0);
	}
}

static unsigned
ev_sel_type_from_seln_rank(rank)
	register Seln_rank	rank;
{
	switch (rank) {
	  case SELN_CARET:
	    return(EV_SEL_CARET);
	  case SELN_PRIMARY:
	    return(EV_SEL_PRIMARY);
	  case SELN_SECONDARY:
	    return(EV_SEL_SECONDARY);
	  case SELN_SHELF:
	    return(EV_SEL_SHELF);
	  default:
	    LINT_IGNORE(ASSUME(0));
	    return(0);
	}
}

static unsigned
holder_flag_from_textsw_info(type)
	register unsigned	type;
{
	if (type & EV_SEL_CARET)
	    return(TXTSW_HOLDER_OF_CARET);
	if (type & EV_SEL_PRIMARY)
	    return(TXTSW_HOLDER_OF_PSEL);
	if (type & EV_SEL_SECONDARY)
	    return(TXTSW_HOLDER_OF_SSEL);
	if (type & EV_SEL_SHELF)
	    return(TXTSW_HOLDER_OF_SHELF);
	return(0);
}

pkg_private Seln_rank
seln_rank_from_textsw_info(type)
	register unsigned	type;
{
	if (type & EV_SEL_CARET)
	    return(SELN_CARET);
	if (type & EV_SEL_PRIMARY)
	    return(SELN_PRIMARY);
	if (type & EV_SEL_SECONDARY)
	    return(SELN_SECONDARY);
	if (type & EV_SEL_SHELF)
	    return(SELN_SHELF);
	return(SELN_UNKNOWN);
}

/* The following acquires the selection of the specified rank unless the
 *   textsw already holds it.
 */
pkg_private Seln_rank
textsw_acquire_seln(textsw, rank)
	register Textsw_folio	textsw;
	register Seln_rank	rank;
{
	unsigned		holder_flag;

	if (textsw_should_ask_seln_svc(textsw, TRUE)) {
	    if ((textsw->holder_state & holder_flag_from_seln_rank(rank)) == 0) {
		rank = seln_acquire(textsw->selection_client, rank);
	    }
	} else if (rank == SELN_UNSPECIFIED) {
	    textsw->holder_state |= TXTSW_HOLDER_OF_ALL;
	    return(SELN_UNKNOWN);
		/* Don't fall through as that will bump time-of-death */
	}
	holder_flag = holder_flag_from_seln_rank(rank);
	if (holder_flag) {
	    textsw->holder_state |= holder_flag;
	} else {
            Event   alert_event;
	    int	result;
	    /* Assume svc temporarily dead, but make sure timer is set before
	     * calling textsw_post_error because it will result in a recursive
	     * call on this routine via textsw_may_win_exit if the timer is
	     * clear.
	     */
	    gettimeofday(&textsw->selection_died, (struct timezone *)0);
 
            result = alert_prompt(
                    (Frame)window_get(WINDOW_FROM_VIEW(textsw), WIN_OWNER),
                    &alert_event,
                    ALERT_MESSAGE_STRINGS,
                        "Cannot contact Selection Service - pressing",
                        " on but selections across windows may not work.",
                        0,    
                    ALERT_BUTTON_YES, "Continue",
                    0);
            if (result == ALERT_FAILED) {
                    fprintf(stderr,
                        "ALERT_FAILED!\nCannot contact Selection Service - pressing on but selections across windows may not work.\n");
            }
	    textsw->holder_state |= TXTSW_HOLDER_OF_ALL;
	}
	return(rank);
}

/*	==========================================================
 *
 *	Routines to support the use of selections.
 *
 *	==========================================================
 */

#define	TXTSW_NEED_SELN_CLIENT	(Seln_client)1
#define MIN_SELN_TIMEOUT        0
#define DEFAULT_SELN_TIMEOUT    10		/* 10 seconds */
#define MAX_SELN_TIMEOUT        300		/* 5 minutes */

static int
textsw_should_ask_seln_svc(textsw, retry_okay)
	register Textsw_folio	textsw;
	int			retry_okay;
{
	pkg_private void	textsw_seln_svc_function();
	pkg_private Seln_result	textsw_seln_svc_reply();
	struct timeval		now;
	int     		result;
	Event   		alert_event;
	int          selection_timeout;


	if (textsw->state & TXTSW_DELAY_SEL_INQUIRE) {
	    textsw->state &= ~TXTSW_DELAY_SEL_INQUIRE;
	    return(textsw_sync_with_seln_svc(textsw));
	}
	
	if (textsw->selection_client == 0)
	    return(FALSE);
	if (timerisset(&textsw->selection_died)) {
	    if (!retry_okay)
		return(FALSE);
	    /* Retry if contact lost at least 60 seconds ago */
	    gettimeofday(&now, (struct timezone *)0);
	    if (now.tv_sec < textsw->selection_died.tv_sec + 60)
		return(FALSE);
	    /* textsw_post_error will eventually call textsw_may_win_exit
	     * which will call textsw_should_ask_seln_svc (with retry_okay set
	     * to FALSE), but we must make sure that the timer is still set
	     * on that recursive call, so don't clear it until AFTER
	     * textsw_post_error returns!
	     */
            result = alert_prompt(
                    (Frame)window_get(WINDOW_FROM_VIEW(textsw), WIN_OWNER),
                    &alert_event,
                    ALERT_MESSAGE_STRINGS,
                        "textsw: attempting to re-contact Selection Service",
                        " - selections may be temporarily incorrect.",
                        0,
                    ALERT_BUTTON_YES, "Continue",
                    0);
            if (result == ALERT_FAILED) {
		    fprintf(stderr,"ALERT_FAILED!\ntextsw: attempting to re-contact Selection Service - selections may be temporarily incorrect.\n");
            }
	    timerclear(&textsw->selection_died);
	}
		
	if (textsw->selection_client == TXTSW_NEED_SELN_CLIENT) {    	    	   
	    selection_timeout = defaults_get_integer_check("/SunView/Selection_Timeout",
		DEFAULT_SELN_TIMEOUT, MIN_SELN_TIMEOUT, MAX_SELN_TIMEOUT, 0);

	    seln_use_timeout(selection_timeout);		/* set selection timeout */

	    /* Try to establish the initial connection with Seln. Svc. */
	    textsw->selection_client = seln_create(
		textsw_seln_svc_function, textsw_seln_svc_reply,
		(caddr_t)LINT_CAST(textsw->first_view));
	    if (textsw->selection_client == 0) {
                int     result;
                Event   alert_event;
 
		/* Set up to retry later and then tell user we lost. */
		textsw->selection_client = TXTSW_NEED_SELN_CLIENT;
		gettimeofday(&textsw->selection_died, (struct timezone *)0);
                    result = alert_prompt(
                        (Frame)window_get(WINDOW_FROM_VIEW(textsw), WIN_OWNER),                        &alert_event,
                        ALERT_MESSAGE_STRINGS,
                            "textsw: cannot initiate contact with Selection",
                            " Service; pressing on and will retry later",
                            0,
                        ALERT_BUTTON_YES, "Continue",
                        0);
                if (result == ALERT_FAILED) {
		    fprintf(stderr,"ALERT_FAILED!\ntextsw: cannot initiate contact with Selection Service; pressing on and will retry later\n");
                }
		return(FALSE);
	    }
	}
	return(TRUE);
}

pkg_private unsigned
textsw_determine_selection_type(textsw, acquire)
	register Textsw_folio	textsw;
	int			acquire;
{
	register unsigned	result;
	register int		ask_svc =
				textsw_should_ask_seln_svc(textsw, TRUE);
	register Seln_rank	rank;
	Seln_holder		holder;

	if (textsw->holder_state & TXTSW_HOLDER_OF_SSEL) {
	    if ((textsw->holder_state & TXTSW_HOLDER_OF_ALL) ==
	        TXTSW_HOLDER_OF_ALL) {
		ask_svc = 0;
	    } else {
		Firm_event	focus;
		int		shift;
		if (!win_get_focus_event(WIN_FD_FOR_VIEW(textsw->first_view),
					 &focus, &shift)) {
		    ask_svc = (focus.id != LOC_WINENTER);
		}
	    }
	    if (!ask_svc)
		result = (textsw->track_state & TXTSW_TRACK_SECONDARY)
			 ? EV_SEL_SECONDARY : EV_SEL_PRIMARY;
	}
#ifdef DEBUG_SVC
fprintf(stderr, "Holders: %d%s", textsw->holder_state & TXTSW_HOLDER_OF_ALL,
	(ask_svc) ? "\t" : "\n");
#endif
	if (ask_svc) {
	    if (acquire == 0) {
		holder = seln_inquire(SELN_UNSPECIFIED);
		result = (holder.rank == SELN_SECONDARY)
			 ? EV_SEL_SECONDARY : EV_SEL_PRIMARY;
	    } else {
		rank = textsw_acquire_seln(textsw, SELN_UNSPECIFIED);
#ifdef DEBUG_SVC
fprintf(stderr, "textsw_acquire_seln returned rank = %d\n", (int)rank);
#endif
		switch (rank) {
		  case SELN_PRIMARY:
		    result = EV_SEL_PRIMARY;
		    textsw->track_state &= ~TXTSW_TRACK_SECONDARY;
		    break;
		  case SELN_SECONDARY:
		    result = EV_SEL_SECONDARY;
		    textsw->track_state |= TXTSW_TRACK_SECONDARY;
		    if (textsw->func_state) {
		    } else if (seln_get_function_state(SELN_FN_DELETE)) {
			/* Act as if it was local DELETE down */
			textsw->func_state |=
			    TXTSW_FUNC_DELETE|
			    TXTSW_FUNC_SVC_SAW(TXTSW_FUNC_DELETE);
			if (!TXTSW_IS_READ_ONLY(textsw))
			    textsw->state |= TXTSW_PENDING_DELETE;
		    } else if (seln_get_function_state(SELN_FN_PUT)) {
			/* Act as if it was local PUT down */
			textsw_begin_put(textsw->first_view, 0);
			textsw->func_state |=
			    TXTSW_FUNC_SVC_SAW(TXTSW_FUNC_PUT);
		    }
		    break;
		  default:		/* Assume svc temporarily dead */
		    result = EV_SEL_PRIMARY;
		    break;
		}
	    }
	} else if (textsw->selection_client == 0) {
	    textsw->holder_state |= TXTSW_HOLDER_OF_ALL;
	}
	if (textsw->state & TXTSW_PENDING_DELETE) {
	    result |= (textsw->track_state & TXTSW_TRACK_SECONDARY)
			? EV_SEL_PD_SECONDARY : EV_SEL_PD_PRIMARY;
	}
	return(result);
}

/* For function keys that deal with selections, the following cases must be
 *   considered as plausible:
 * No primary selection
 *    (See below for subcases)
 * Primary selection in window A (our window)
 *    FN key DOWN in window A
 *	No secondary selection
 *	  FN key UP in window A
 *	  FN key UP in window B (some other window)
 *	Secondary selection in window A
 *	  FN key UP in window A
 *	  FN key UP in window B
 *	Secondary selection in window B
 *	  FN key UP in window A
 *	  FN key UP in window B
 *    FN key DOWN in window B
 *	No secondary selection
 *	  FN key UP in window A
 *	  FN key UP in window B
 *	Secondary selection in window A
 *	  FN key UP in window A
 *	  FN key UP in window B
 *	Secondary selection in window B
 *	  FN key UP in window A
 *	  FN key UP in window B
 * Primary selection in window B
 *    (See above for subcases)
 *
 * On the FN key UP, the cases can be viewed as:
 *    DOWN in same window  => inform service iff informed it about DOWN
 *    DOWN in other window => inform other window via service
 *
 * Window that is to process FN key (assumed to be holder of Primary
 *   selection) has three UP cases to deal with:
 *    UP in this window, service not informed =>
 *	process directly/locally, any Secondary selection is local
 *    UP in this window, service informed (by this window) ||
 *    UP in other window =>
 *	Secondary selection can be:
 *	  local		=> process directly
 *	  remote	=> process indirectly
 *	  non-existent	=>
 *	    Primary selection can be:
 *		local		=> process directly
 *		remote		=> process indirectly
 *		non-existent	=> no-op
 */

pkg_private void
textsw_give_shelf_to_svc(folio)
	register Textsw_folio	folio;
{
	extern Es_handle	es_file_create();
	register Es_handle	output;
	Es_status		create_status, copy_status;

	if (folio->trash == ES_NULL)
	    return;
	if (!textsw_should_ask_seln_svc(folio, TRUE))
	    return;
	/* Write the contents to a file */
	output = es_file_create(shelf_name, ES_OPT_APPEND, &create_status);
	/* If create failed due to permission problem, retry */
	if ((output == 0) && (create_status == ES_CHECK_ERRNO) &&
	    (errno == EACCES)) {
	    unlink(shelf_name);
	    output = es_file_create(shelf_name, ES_OPT_APPEND, &create_status);
	}
	if (output) {
	    copy_status = es_copy(folio->trash, output, FALSE);
	    if (copy_status == ES_SUCCESS) {
		seln_hold_file(SELN_SHELF, shelf_name);
		folio->holder_state &= ~TXTSW_HOLDER_OF_SHELF;
	    }
	    es_destroy(output);
	}
	/* Now destroy the trashbin */
	es_destroy(folio->trash);
	folio->trash = ES_NULL;
}

pkg_private int
textsw_sync_with_seln_svc(folio)
	register Textsw_folio	folio;
{
	Seln_holders_all	holders;
	int			result;

	if (result = textsw_should_ask_seln_svc(folio, TRUE)) {
	    holders = seln_inquire_all();
	    if (SAME_HOLDER(folio, (caddr_t)LINT_CAST(&holders.caret))) {
		folio->holder_state |= TXTSW_HOLDER_OF_CARET;
	    } else {
		folio->holder_state &= ~TXTSW_HOLDER_OF_CARET;
	    }
	    if (SAME_HOLDER(folio, (caddr_t)LINT_CAST(&holders.primary))) {
		folio->holder_state |= TXTSW_HOLDER_OF_PSEL;
	    } else {
		folio->holder_state &= ~TXTSW_HOLDER_OF_PSEL;
	    }
	    if (SAME_HOLDER(folio, (caddr_t)LINT_CAST(&holders.secondary))) {
		folio->holder_state |= TXTSW_HOLDER_OF_SSEL;
	    } else {
		folio->holder_state &= ~TXTSW_HOLDER_OF_SSEL;
	    }
	    if (SAME_HOLDER(folio, (caddr_t)LINT_CAST(&holders.shelf))) {
		folio->holder_state |= TXTSW_HOLDER_OF_SHELF;
	    } else {
		folio->holder_state &= ~TXTSW_HOLDER_OF_SHELF;
	    }
	} else {
	    folio->holder_state |= TXTSW_HOLDER_OF_ALL;
	}
	return(result);
}

/* Returns TRUE iff function is completely local to textsw, and all
 *   selections are guaranteed to be within the textsw.
 * The caller should ignore the return value when is_down == TRUE.
 */
pkg_private int
textsw_inform_seln_svc(textsw, function, is_down)
	register Textsw_folio	textsw;
	register unsigned	function;
	int			is_down;
{
#define	ALL_SELECTIONS_ARE_LOCAL(textsw)	\
	(((textsw)->holder_state & TXTSW_HOLDER_OF_ALL) == TXTSW_HOLDER_OF_ALL)

	register Seln_function	seln_function;

	if (!textsw_should_ask_seln_svc(textsw, is_down)) {
	    return(TRUE);
	}
	switch (function) {
	  case TXTSW_FUNC_DELETE:
	    seln_function = SELN_FN_DELETE;
	    break;
	  case TXTSW_FUNC_FIND:
	    seln_function = SELN_FN_FIND;
	    break;
	  case TXTSW_FUNC_GET:
	    seln_function = SELN_FN_GET;
	    break;
	  case TXTSW_FUNC_PUT:
	    seln_function = SELN_FN_PUT;
	    break;
	  case TXTSW_FUNC_FIELD:
	    /* Seln. Svc. does not (yet) have the concept of FIELD search. */
	  default:
	    return(TRUE);
	}
#ifdef DEBUG_SVC
fprintf(stderr, "Holders: %d	Func: %d(%s)	",
	textsw->holder_state & TXTSW_HOLDER_OF_ALL, (int)function,
	(is_down) ? "v" : "^");
#endif
	if (is_down) {
	    if (!ALL_SELECTIONS_ARE_LOCAL(textsw)) {
		textsw_acquire_seln(textsw, SELN_CARET);
		textsw->selection_func =
		    seln_inform(textsw->selection_client,
				seln_function, is_down);
		/* The Seln. Svc. resets the Secondary Selection to be
		 * non-existent on any inform, so make our state agree!
		 */
		textsw->holder_state &= ~TXTSW_HOLDER_OF_SSEL;
		ASSUME(textsw->selection_func.function == SELN_FN_ERROR);
		textsw->func_state |= TXTSW_FUNC_SVC_SAW(function);
	    }
	    return(TRUE);
	} else if (textsw->func_state & TXTSW_FUNC_SVC_REQUEST) {
#ifdef DEBUG_SVC
fprintf(stderr, "Request from service\n");
#endif
	    if (textsw->selection_holder == 0 ||
		textsw->selection_holder->state == SELN_NONE ||
		ALL_SELECTIONS_ARE_LOCAL(textsw)) {
		return(TRUE);
	    }
	    goto Service_Request;
	} else {
	    if (ALL_SELECTIONS_ARE_LOCAL(textsw) &&
		(textsw->func_state & function) &&
		((textsw->func_state & TXTSW_FUNC_SVC_SAW_ALL) == 0)) {
		/* Don't tell svc about UP iff we also had DOWN and
		 *   did not tell svc about it.  Test for local selections
		 *   insures that we would have seen the DOWN.
		 */
		    return(TRUE);
	    } else {
		textsw->selection_func =
		    seln_inform(textsw->selection_client,
				seln_function, is_down);
#ifdef DEBUG_SVC
fprintf(stderr, "%d\n", textsw->selection_func.function);
#endif
		if (textsw->selection_func.function != SELN_FN_ERROR) {
		    static Seln_response	textsw_setup_function();
		    if (SELN_IGNORE !=
			textsw_setup_function(textsw,
					      &textsw->selection_func))
			goto Service_Request;
		}
		textsw->func_state &= ~TXTSW_FUNC_EXECUTE;
		return(FALSE);
	    }
	}
Service_Request:
#ifdef DEBUG_SVC
fflush(stderr);
#endif
	switch (function) {
	  case TXTSW_FUNC_GET:
	  case TXTSW_FUNC_PUT:
	    return((textsw->selection_holder == 0) ||
		   SAME_HOLDER(textsw,
			(caddr_t)LINT_CAST(textsw->selection_holder)));
	  case TXTSW_FUNC_DELETE:
	    /* Following is required to make DELETEs initiated from another
	     * window work.
	     */
	    if ((textsw->holder_state & TXTSW_HOLDER_OF_CARET) == 0)
	 	textsw->func_state |= TXTSW_FUNC_DELETE;
	    /* Fall through */
	  default:
	    return(TRUE);
	}
#undef	ALL_SELECTIONS_ARE_LOCAL(textsw)
}

pkg_private int
textsw_abort(textsw)
	register Textsw_folio	textsw;
{
	if (textsw_should_ask_seln_svc(textsw, TRUE) &&
	    (textsw->func_state & TXTSW_FUNC_SVC_SAW_ALL)) {
	    seln_clear_functions();
	}
	if (textsw->track_state & TXTSW_TRACK_SECONDARY) {
	    textsw_set_selection(VIEW_REP_TO_ABS(textsw->first_view),
				 ES_INFINITY, ES_INFINITY, EV_SEL_SECONDARY);
	}
	textsw_clear_pending_func_state(textsw);
	textsw->state &= ~TXTSW_PENDING_DELETE;
	textsw->track_state &= ~TXTSW_TRACK_ALL;
}

#ifdef DEBUG_SVC
static long inform_service = 0x7FFFFFFF;
#endif
pkg_private void
textsw_may_win_exit(textsw)
	Textsw_folio	 textsw;
{
#define	PENDING_EVENT(textsw, func)				\
	(((textsw)->func_state & func) &&			\
	 (((textsw)->func_state & TXTSW_FUNC_SVC_SAW(func)) == 0))

	textsw_flush_caches(textsw->first_view, TFC_STD);
#ifdef DEBUG_SVC
	if (inform_service-- <= 0)
	    return;
#endif
	if (textsw->state & TXTSW_DELAY_SEL_INQUIRE) 
	    return;    /* Textsw does not know about the state selection svc */
	
	
	/*
	 *  If selection service already got the request, then
	 *  it should know about the state.
	 */
	if (((textsw->func_state & TXTSW_FUNC_SVC_REQUEST) == 0) && 
	    textsw_should_ask_seln_svc(textsw, FALSE)) {	
	    unsigned	holder_state;
	    Es_index	first, last_plus_one;

	    holder_state = textsw->holder_state & TXTSW_HOLDER_OF_ALL;
	    (void) ev_get_selection(textsw->views, &first, &last_plus_one,
				    EV_SEL_SECONDARY);
	    /* Following makes sure cache actually tells the svc. */
	    textsw->holder_state &= ~TXTSW_HOLDER_OF_ALL;
	    if (PENDING_EVENT(textsw, TXTSW_FUNC_DELETE))
		(void) textsw_inform_seln_svc(textsw, TXTSW_FUNC_DELETE, TRUE);
	    if (PENDING_EVENT(textsw, TXTSW_FUNC_FIND))
		(void) textsw_inform_seln_svc(textsw, TXTSW_FUNC_FIND, TRUE);
	    if (PENDING_EVENT(textsw, TXTSW_FUNC_GET))
		(void) textsw_inform_seln_svc(textsw, TXTSW_FUNC_GET, TRUE);
	    if (PENDING_EVENT(textsw, TXTSW_FUNC_PUT))
		(void) textsw_inform_seln_svc(textsw, TXTSW_FUNC_PUT, TRUE);
	    /* Restore the cache's state, and sync Seln. Svc. - sticky part
	     * is we had not told Svc. about key down and may be part way
	     * into 2nd-ary Seln., which we also have not told Svc. about.
	     */
	    textsw->holder_state |= holder_state;
	    textsw->holder_state &= ~TXTSW_HOLDER_OF_SSEL;
	    if (first < last_plus_one) {
		textsw_acquire_seln(textsw, SELN_SECONDARY);
	    }
	}
#undef	PENDING_EVENT
}

/* Returns new value for buf_max_len.
 * Can alter to_read and buf as side_effects.
 */
pkg_private int
textsw_prepare_buf_for_es_read(to_read, buf, buf_max_len, fixed_size)
	register int	 *to_read;
	register char	**buf;
	register int	  buf_max_len;
	int		  fixed_size;
{
	if (*to_read > buf_max_len) {
	    if (fixed_size) {
		*to_read = buf_max_len;
	    } else {
		free(*buf);
		buf_max_len = *to_read+1;
		*buf = sv_malloc((u_int)buf_max_len);
	    }
	}
	return(buf_max_len);
}

/* Returns the actual number of entities read into the buffer.
 * Caller is responsible for making sure that the buffer is large enough.
 */
pkg_private int
textsw_es_read(esh, buf, first, last_plus_one)
	register Es_handle	 esh;
	register char		*buf;
	Es_index		 first;
	register Es_index	 last_plus_one;
{
	register int		 result;
	int			 count;
	register Es_index	 current, next;

	result = 0;
	current = first;
	(void) es_set_position(esh, current);
	while (last_plus_one > current) {
	    next = es_read(esh, last_plus_one-current, buf+result, &count);
	    if READ_AT_EOF(current, next, count)
	        break;
	    result += count;
	    current = next;
	}
	return(result);
}

typedef struct tsfh_object {
	Textsw_view		view;
	Textsw_selection_handle	selection;
	Seln_attribute		continued_attr;
	unsigned		flags;
	int			fill_result;
} Tsfh_object;
typedef Tsfh_object	*Tsfh_handle;

pkg_private int
textsw_fill_selection_from_reply(context, reply)
	register Tsfh_handle	 context;
	register Seln_request	*reply;
{
	int			 result = 0;
	register caddr_t	*attr;
	int			 got_contents_ascii = FALSE, to_read;
	register Textsw_selection_handle
				 selection = context->selection;

	if (context->continued_attr != SELN_REQ_END_REQUEST) {
	    return(TFS_ERROR);
	}
	for (attr = (caddr_t *)LINT_CAST(reply->data); *attr;
	     attr = attr_next(attr)) {
	    switch ((Seln_attribute)(*attr)) {
	      case SELN_REQ_BYTESIZE:
		selection->first = 0;
		selection->last_plus_one = (Es_index)attr[1];
		break;
	      case SELN_REQ_FIRST:
		selection->first = (Es_index)attr[1];
		break;
	      case SELN_REQ_LAST:
		selection->last_plus_one = 1 + (Es_index)attr[1];
		break;
	      case SELN_REQ_CONTENTS_ASCII:
		if (reply->status == SELN_CONTINUED) {
		    context->continued_attr = (Seln_attribute)(*attr);
		    to_read = strlen((char *)(attr+1));
		} else {
		    to_read = selection->last_plus_one - selection->first;
		}
		selection->buf_max_len =
		    textsw_prepare_buf_for_es_read(&to_read,
			&selection->buf, selection->buf_max_len,
			!selection->buf_is_dynamic);
		selection->buf_len = to_read;
		/* Clients must be aware that either due to continuation
		 *   or because the buffer was not big enough,
		 *   selection->last_plus_one may be greater than
		 *   selection->first + to_read
		 */
		bcopy((char *)(attr+1), selection->buf, to_read);
		got_contents_ascii = TRUE;
		if (reply->status == SELN_CONTINUED)
		    goto Return;
		break;
	      case SELN_REQ_UNKNOWN:
		result |= TFS_BAD_ATTR_WARNING;
		break;
	      default:
		if ((attr == (caddr_t *)LINT_CAST(reply->data)) ||
		    ((context->flags & TFS_FILL_IF_OTHER) &&
		     (got_contents_ascii == FALSE)))
		    goto Seln_Error;
		result |= TFS_BAD_ATTR_WARNING;
		break;
	    }
	}
Return:
	return(result);

Seln_Error:
	return(TFS_SELN_SVC_ERROR);
}

static Seln_result
only_one_buffer(request)
	Seln_request	*request;
{
	Tsfh_handle	 context = (Tsfh_handle)LINT_CAST(
					request->requester.context);

	if (request->status == SELN_CONTINUED) {
	    context->fill_result = TFS_ERROR;
	    return(SELN_FAILED);
	} else {
	    context->fill_result =
		textsw_fill_selection_from_reply(context, request);
	    return(SELN_SUCCESS);
	}
}

pkg_private int
textsw_selection_from_holder(textsw, selection, holder, type, flags)
	register Textsw_folio			 textsw;
	register Textsw_selection_handle	 selection;
	Seln_holder				*holder;
	unsigned				 type, flags;

{
	extern int		 ev_get_selection();
	unsigned		 mode;
	register caddr_t	*req_attr;
	int			 result = type, to_read;
	caddr_t			 req_for_ascii[3];
	Seln_result		 query_result;
	Tsfh_object		 context;

	if (holder) {
	    if (holder->state == SELN_NONE)
		goto Seln_Error;
	    if (SAME_HOLDER(textsw, (caddr_t)LINT_CAST(holder))) {
		textsw->holder_state |= holder_flag_from_seln_rank(holder->rank);
		result = type = ev_sel_type_from_seln_rank(holder->rank);
		if ((type == EV_SEL_PRIMARY) || (type == EV_SEL_SECONDARY))
		    goto Get_Local;
	    }
	    /*
	     * Make the request to the selection holder
	     */
	    if (selection->per_buffer) {
		context.view = textsw->first_view;
		context.selection = selection;
		context.continued_attr = SELN_REQ_END_REQUEST;
		context.flags = flags;
	    } else {
		LINT_IGNORE(ASSERT(0));
		goto Seln_Error;
	    }
	    req_attr = req_for_ascii;
	    if (flags & TFS_FILL_IF_OTHER) {
		*req_attr++ = (caddr_t)SELN_REQ_CONTENTS_ASCII;
		*req_attr++ = (caddr_t)0;  /* Null value */
	    }
	    *req_attr++ = (caddr_t)0;  /* Null terminate request list */
	    query_result =
		seln_query(holder, selection->per_buffer,
			   (caddr_t)LINT_CAST(&context),
			   SELN_REQ_BYTESIZE, ES_INFINITY,
			   SELN_REQ_FIRST, ES_INFINITY,
			   SELN_REQ_LAST, ES_INFINITY,
			   ATTR_LIST, req_for_ascii,
			   0);
	    if (query_result != SELN_SUCCESS)
		goto Seln_Error;
	    return(TFS_IS_ERROR(context.fill_result)
			? context.fill_result 
			: (context.fill_result | result | TFS_IS_OTHER) );
	} else {
Get_Local:
	    mode = ev_get_selection(textsw->views, &selection->first,
				    &selection->last_plus_one, type);
	    result |= mode;
	    if (selection->first >= selection->last_plus_one) {
		return(TFS_ERROR);
	    }
	    if (flags & TFS_FILL_IF_SELF) {
		to_read = selection->last_plus_one - selection->first;
		selection->buf_max_len =
		    textsw_prepare_buf_for_es_read(&to_read, &selection->buf,
			selection->buf_max_len, !selection->buf_is_dynamic);
		selection->last_plus_one = selection->first + to_read;
		selection->buf_len =
		    textsw_es_read(
			(type & EV_SEL_SHELF)
			    ? textsw->trash : textsw->views->esh,
			selection->buf,
			selection->first, selection->last_plus_one);
	    }
	    return(result|TFS_IS_SELF);
	}
Seln_Error:
	return(TFS_SELN_SVC_ERROR);
}

pkg_private int
textsw_func_selection_internal(textsw, selection, type, flags)
	register Textsw_folio			textsw;
	register Textsw_selection_handle	selection;
	unsigned				type, flags;

{
	Seln_holder		 holder;
	int			 result;

	if ( ((EV_SEL_BASE_TYPE(type) == EV_SEL_PRIMARY) ||
	      (EV_SEL_BASE_TYPE(type) == EV_SEL_SECONDARY)) &&
	     (textsw->holder_state & holder_flag_from_textsw_info(type)) ) {
	    result = textsw_selection_from_holder(
			    textsw, selection, (Seln_holder *)0, type, flags);
	} else if (textsw_should_ask_seln_svc(textsw, TRUE)) {
	    if (textsw->selection_holder) {
		holder = *textsw->selection_holder;
	    } else {
		holder = seln_inquire(seln_rank_from_textsw_info(type));
	    }
	    result = textsw_selection_from_holder(
			    textsw, selection, &holder, type, flags);
	} else {
	    result = TFS_SELN_SVC_ERROR;
	}
	return(result);
}

pkg_private int
textsw_func_selection(textsw, selection, flags)
	register Textsw_folio			textsw;
	register Textsw_selection_handle	selection;
	unsigned				flags;

{
	int					result;

	if (textsw->selection_holder) {
	    result = textsw_selection_from_holder(
			    textsw, selection, textsw->selection_holder,
			    0, flags);
	} else {
	    result = textsw_func_selection_internal(
			    textsw, selection, EV_SEL_SECONDARY, flags);
	    if (TFS_IS_ERROR(result)) {
		result = textsw_func_selection_internal(
			    textsw, selection, EV_SEL_PRIMARY, flags);
	    }
	}
	selection->type = result;
	return(result);
}

/* ARGSUSED */
pkg_private void
textsw_init_selection_object(
    textsw, selection, buf, buf_max_len, buf_is_dynamic)
	Textsw_folio			 textsw;    /* Currently unused */
	register Textsw_selection_handle selection;
	char				*buf;
	int				 buf_max_len;
	int				 buf_is_dynamic;
{
	selection->type = 0;
	selection->first = selection->last_plus_one = ES_INFINITY;
	selection->per_buffer = only_one_buffer;
	if (buf) {
	    selection->buf = buf;
	    selection->buf_max_len = buf_max_len-1;
	    selection->buf_is_dynamic = buf_is_dynamic;
	} else {
	    selection->buf = sv_malloc(SELN_BUFSIZE+1);
	    selection->buf_max_len = SELN_BUFSIZE;
	    selection->buf_is_dynamic = TRUE;
	}
	selection->buf_len = 0;
}

pkg_private int
textsw_is_seln_nonzero(textsw, type)
	register Textsw_folio			textsw;
	unsigned				type;
{
	Textsw_selection_object	 selection;
	int			 result;

	textsw_init_selection_object(textsw, &selection, "", 1, FALSE);
	result =
	    textsw_func_selection_internal(textsw, &selection, type, 0);
	if (TFS_IS_ERROR(result) ||
	    (selection.first >= selection.last_plus_one)) {
	    return(0);
	} else {
	    return((result & TFS_IS_SELF) ? 2 : 1);
	}
}

static Seln_result
textsw_stuff_all_buffers(request)
	register Seln_request	*request;
{
	register Tsfh_handle	 context = (Tsfh_handle)LINT_CAST(
						request->requester.context);
	int			 status;

	if (context->continued_attr == SELN_REQ_END_REQUEST) {
	    context->fill_result =
		textsw_fill_selection_from_reply(context, request);
	    if (TFS_IS_ERROR(context->fill_result)) {
		return(SELN_FAILED);
	    } else {
		status = textsw_input_partial(context->view,
					      context->selection->buf,
					      context->selection->buf_len);
		return(status ? SELN_FAILED : SELN_SUCCESS);
	    }
	} else if (request->status == SELN_CONTINUED) {
	    status = textsw_input_partial(context->view,
					  request->data,
					  (long int)request->buf_size);
	    return(status ? SELN_FAILED : SELN_SUCCESS);
	} else if (request->status == SELN_SUCCESS) {
	    status = textsw_input_partial(context->view,
					  request->data,
					  strlen((char *)(request->data)));
	    return(status ? SELN_FAILED : SELN_SUCCESS);
	} else {
	    context->fill_result = TFS_SELN_SVC_ERROR;
	    return(SELN_FAILED);
	}
}

pkg_private int
textsw_stuff_selection(view, type)
	Textsw_view		view;
	unsigned		type;
{
	register Textsw_folio	folio = FOLIO_FOR_VIEW(view);
	Textsw_selection_object	selection;
	int			result;
	int			delta, record;
	Es_index		old_insert_pos, old_length;

	textsw_init_selection_object(folio, &selection, NULL, 0, 0);
	selection.per_buffer = textsw_stuff_all_buffers;
	textsw_input_before(view, &old_insert_pos, &old_length);
	result = textsw_func_selection_internal(
			folio, &selection, type, TFS_FILL_IF_OTHER);
	if ((TFS_IS_ERROR(result) != 0) ||
	    (selection.first >= selection.last_plus_one))
	    goto Done;
	if (result & TFS_IS_SELF) {
	    /* Selection is local, so copy pieces, not contents. */
	    extern Es_handle	textsw_esh_for_span();
	    Es_handle		pieces;
	    pieces = textsw_esh_for_span(view, selection.first,
					selection.last_plus_one, ES_NULL);
	    (void) textsw_insert_pieces(view, old_insert_pos, pieces);
	} else {
	    record = (TXTSW_DO_AGAIN(folio) &&
		      ((folio->func_state & TXTSW_FUNC_AGAIN) == 0) );
	    delta = textsw_input_after(view, old_insert_pos, old_length,
				       record);
	    if AN_ERROR(delta == ES_CANNOT_SET) {}
	}
Done:
	free(selection.buf);
	return(result);
}

/*	==========================================================
 *
 *	Routines to support the use of the selection service.
 *
 *	==========================================================
 */

pkg_private int
textsw_seln_svc_had_secondary(textsw)
	register Textsw_folio	textsw;
{
	return( (textsw->selection_func.function != SELN_FN_ERROR) &&
		(textsw->selection_func.secondary.access.pid != 0) &&
		(textsw->selection_func.secondary.state != SELN_NONE) );
}

pkg_private Seln_response
textsw_setup_function(folio, function)
	register Textsw_folio		 folio;
	register Seln_function_buffer	*function;
{
	register Seln_response		 response;

	response = seln_figure_response(function, &folio->selection_holder);
#ifdef DEBUG_SVC
fprintf(stderr, "Response = %d\n", response);
#endif
	switch (response) {
	  case SELN_IGNORE:
	    /* Make sure we don't cause a yield to secondary. */
	    function->function = SELN_FN_ERROR;
	    return(response);
	  case SELN_DELETE:
	  case SELN_SHELVE:
	    /* holder describes shelf, not sel., so ignore it */
	    folio->selection_holder = (Seln_holder *)0;
	    /* Fall through */
	  case SELN_FIND:
	  case SELN_REQUEST:
	    break;
	  default:
	    LINT_IGNORE(ASSUME(0));
	    /* Make sure we don't cause a yield to secondary. */
	    function->function = SELN_FN_ERROR;
	    return(SELN_IGNORE);
	}
	/*
	 * The holder information must be treated with special care, as the
	 *   following can otherwise happen:
	 *	1) Primary holder (or listener) gets function request
	 *	2) Primary gets secondary, then requests secondary to yield
	 *	3) Secondary yields, and its state is now correct
	 *	4) (Ex)secondary gets same function request, and based on the
	 *	   (now out of date) information, notes that it is the
	 *	   current secondary holder (and its state is now incorrect).
	 *   Although the above is a secondary selection state problem, it
	 *   also applies to the other holders, as each holder gets a copy of
	 *   the function request (multiple copies if a single client is
	 *   multiple holders).
	 *
	 * The golden rule is that only the "driver" can trust the holder
	 *   information!
	 */
	if (SAME_HOLDER(folio, (caddr_t)LINT_CAST(&function->caret)))
	    folio->holder_state |= TXTSW_HOLDER_OF_CARET;
	else
	    folio->holder_state &= ~TXTSW_HOLDER_OF_CARET;
	if (SAME_HOLDER(folio, (caddr_t)LINT_CAST(&function->primary)))
	    folio->holder_state |= TXTSW_HOLDER_OF_PSEL;
	else
	    folio->holder_state &= ~TXTSW_HOLDER_OF_PSEL;
	if (SAME_HOLDER(folio, (caddr_t)LINT_CAST(&function->secondary)))
	    folio->holder_state |= TXTSW_HOLDER_OF_SSEL;
	else
	    folio->holder_state &= ~TXTSW_HOLDER_OF_SSEL;
	if (SAME_HOLDER(folio, (caddr_t)LINT_CAST(&function->shelf)))
	    folio->holder_state |= TXTSW_HOLDER_OF_SHELF;
	else
	    folio->holder_state &= ~TXTSW_HOLDER_OF_SHELF;
	textsw_take_down_caret(folio);
	return(response);
}

pkg_private void
textsw_end_selection_function(folio)
	register Textsw_folio	folio;
{
	folio->selection_holder = (Seln_holder *)0;
	if (folio->selection_func.function != SELN_FN_ERROR) {
	    if (textsw_seln_svc_had_secondary(folio) &&
		((folio->holder_state & TXTSW_HOLDER_OF_SSEL) == 0)) {
		(void) seln_ask(&folio->selection_func.secondary,
				SELN_REQ_YIELD, 0,
				0);
	    }
	    folio->selection_func.function = SELN_FN_ERROR;
	}
}

pkg_private void
textsw_seln_svc_function(first_view, function)
	Textsw_view			 first_view;
	register Seln_function_buffer	*function;
{
	Seln_response			 response;
	register Textsw_folio		 textsw = FOLIO_FOR_VIEW(first_view);
	register int			 result = 0;

	response = textsw_setup_function(textsw, function);
	if (!textsw->func_view) {
	    textsw->func_view = first_view;
	    textsw->func_x = textsw->func_y = 0;
	}
	textsw->selection_func = *function;
	textsw->func_state &= ~TXTSW_FUNC_SVC_SAW_ALL;
	textsw->func_state |= (TXTSW_FUNC_EXECUTE | TXTSW_FUNC_SVC_REQUEST);
	switch (response) {
	  case SELN_REQUEST:
	    switch (function->function) {
	      case SELN_FN_GET:
		textsw->func_state |= TXTSW_FUNC_GET;
		result |= textsw_end_get(textsw->func_view);
		break;
	      case SELN_FN_PUT:
		textsw->func_state |= TXTSW_FUNC_PUT;
		result = textsw_end_put(textsw->func_view);
		break;
	    }
	    break;
	  case SELN_SHELVE:
	    textsw->func_state |= TXTSW_FUNC_PUT;
	    result = textsw_end_put(textsw->func_view);
	    break;
	  case SELN_FIND:
	    textsw->func_state |= TXTSW_FUNC_FIND;
	    textsw_end_find(textsw->func_view,
			    textsw->func_x, textsw->func_y);
	    break;
	  case SELN_DELETE:
	    textsw->func_state |= TXTSW_FUNC_DELETE;
	    result |= textsw_end_delete(textsw->func_view);
	    break;
	  default:
	    goto Done;
	}
	if (result & TEXTSW_PE_READ_ONLY)
	    textsw_read_only_msg((Textsw_folio)textsw->func_view,
				 textsw->func_x, textsw->func_y);
Done:
	textsw_clear_pending_func_state(textsw);
	textsw->func_state &= ~TXTSW_FUNC_SVC_ALL;
	textsw->func_view = (Textsw_view)0;
	textsw->state &= ~TXTSW_PENDING_DELETE;
	textsw->track_state &= ~TXTSW_TRACK_ALL;
	textsw_end_selection_function(textsw);
}

#define	TCAR_CONTINUED	0x40000000
#define	TCARF_ESH	0
#define	TCARF_STRING	1

static int
textsw_copy_ascii_reply(first, last_plus_one, response,
		        max_length, flags, data)
	Es_index		 first, last_plus_one;
	register caddr_t	*response;
	int			 max_length;
	unsigned		 flags;
	caddr_t			 data;
{
	register char		*dest = (char *)response;
	int			 count, continued;

	if (continued = (max_length < (last_plus_one-first))) {
	    switch (flags) {
	      case TCARF_ESH:
		count = textsw_es_read((Es_handle)LINT_CAST(data), dest,
				       first, first+max_length);
		break;
	      case TCARF_STRING:
		count = max_length;
		bcopy(((char *)data)+first, dest, count);
		break;
	      default:
		LINT_IGNORE(ASSUME(0));
		count = 0;
		break;
	    }
	} else {
	    count = last_plus_one - first;
	    if (count) {
		switch (flags) {
		  case TCARF_ESH:
		    count = textsw_es_read((Es_handle)LINT_CAST(data), dest,
					   first, last_plus_one);
		    break;
		  case TCARF_STRING:
		    bcopy(((char *)data)+first, dest, count);
		    break;
		  default:
		    LINT_IGNORE(ASSUME(0));
		    break;
		}
	    }
	    /* Null terminate string (and round count up.  If count is
	     *   already rounded, then the null that terminates
	     *   the value doubles as the string terminator.)
	     */
	    while ((count % sizeof(*response)) != 0) {
		dest[count++] = '\0';
	    }
	}
	return((continued) ? count+TCAR_CONTINUED : count);
}

char *
textsw_base_name(full_name)
	char *full_name;
{
	extern char	*rindex();
	register char	*temp;

	if ((temp = rindex(full_name, '/')) == NULL)
	    return(full_name);
	else
	    return(temp+1);
}

pkg_private Es_index
textsw_check_valid_range(esh, first, ptr_last_plus_one)
	register Es_handle	 esh;
	register Es_index	 first;
	Es_index		*ptr_last_plus_one;	/* Can be NULL */
{
	register Es_index	 first_valid = first;
	int			 count_read;
	char			 buf[200];   /* Must be bigger than wrap msg */

	if (first != ES_INFINITY &&
	    ((int)LINT_CAST(es_get(esh, ES_PS_SCRATCH_MAX_LEN)) !=
	      	ES_INFINITY) ) {
	    (void) es_set_position(esh, first);
	    first_valid = es_read(esh, sizeof(buf)-1, buf, &count_read);
	    if (first+count_read == first_valid) {
		first_valid = first;
	    } else {
		/* Hit hole, use es_read result, but check last_plus_one */
		if (ptr_last_plus_one &&
		    *ptr_last_plus_one < first_valid) {
		    *ptr_last_plus_one = first_valid;
		}
	    }
	}
	return(first_valid);
}

pkg_private Seln_result
textsw_seln_svc_reply(attr, context, max_length)
	Seln_attribute		    attr;
	register Seln_replier_data *context;
	int			    max_length;
{
	Textsw_view		    first_view = (Textsw_view)
					    LINT_CAST(context->client_data);
	register Textsw_folio	    textsw = FOLIO_FOR_VIEW(first_view);
	register Continuation	    cont_data;
	register Seln_result	    result = SELN_SUCCESS;

	if (context->context) {
	    cont_data = (Continuation)LINT_CAST(context->context);
	} else if (attr == SELN_REQ_END_REQUEST) {
	    /* Don't set up state after having already flushed it due to
	     * previous error.
	     */
	    cont_data = (Continuation)0;
	} else {
	    unsigned	holder_flag =
			holder_flag_from_seln_rank(context->rank);
	    /* First attribute: set up for this set of replies. */
	    if (fast_continuation.in_use) {
	    	cont_data = NEW(Continuation_object);
	    } else {
	    	cont_data = &fast_continuation;
	    }
	    cont_data->in_use = TRUE;
	    context->context = (char *)cont_data;
	    if ((textsw->holder_state & holder_flag) == 0) {
		result = SELN_DIDNT_HAVE;
		goto Return;
	    }
 	    switch (context->rank) {
	      case SELN_CARET:
		cont_data->type = EV_SEL_CARET;
		cont_data->first = cont_data->last_plus_one = ES_INFINITY;
		break;
	      case SELN_PRIMARY:
		cont_data->type = EV_SEL_PRIMARY;
		goto Get_Info;
	      case SELN_SECONDARY:
		cont_data->type = EV_SEL_SECONDARY;
Get_Info:
		cont_data->span_level = textsw->span_level;
		ev_get_selection(textsw->views, &cont_data->first,
				 &cont_data->last_plus_one,
				(unsigned)cont_data->type);
		break;
	      case SELN_SHELF:
		cont_data->type = EV_SEL_SHELF;
		if (textsw->trash) {
		    cont_data->first = es_set_position(textsw->trash, 0);
		    cont_data->last_plus_one = es_get_length(textsw->trash);
		} else {
		    cont_data->first = cont_data->last_plus_one = ES_INFINITY;
		}
		break;
	      default:
		result = SELN_FAILED;
		goto Return;
	    }
	    cont_data->current = ES_INFINITY;
	    /* Check for, and handle, wrap-around edit log */
	    cont_data->first = textsw_check_valid_range(
		(cont_data->type == EV_SEL_SHELF)
		  ? textsw->trash : textsw->views->esh,
		cont_data->first,
		&cont_data->last_plus_one);
	}
	switch(attr) {
	  case SELN_REQ_BYTESIZE:
	    *context->response_pointer++ =
		(caddr_t)(cont_data->last_plus_one - cont_data->first);
	    goto Return;
	  case SELN_REQ_CONTENTS_ASCII: {
	    int		continued, count;
	    Es_index	current_first;
	    Es_handle	esh_to_use;
	    esh_to_use = (cont_data->type == EV_SEL_SHELF)
			    ? textsw->trash : textsw->views->esh;
	    current_first = (cont_data->current == ES_INFINITY)
			    ? cont_data->first : cont_data->current;
	    count = textsw_copy_ascii_reply(
		current_first, cont_data->last_plus_one,
		context->response_pointer, max_length, TCARF_ESH,
		(caddr_t)esh_to_use);
	    if (continued = (count >= TCAR_CONTINUED))
		count -= TCAR_CONTINUED;
	    context->response_pointer +=
		(count / sizeof(*context->response_pointer));
	    if (continued) {
		cont_data->current = current_first + count;
		result = SELN_CONTINUED;
	    } else {
	        *context->response_pointer++ = 0;    /* Null terminate value */
		cont_data->current = ES_INFINITY;
	    }
	    goto Return;
	  }
	  case SELN_REQ_COMMIT_PENDING_DELETE: {
	    (void) textsw_do_pending_delete(first_view, EV_SEL_SECONDARY, 0);
	    goto Return;
	  }
	  case SELN_REQ_DELETE: {
	    (void) textsw_delete_span(
		    first_view, cont_data->first, cont_data->last_plus_one,
		    TXTSW_DS_ADJUST|TXTSW_DS_SHELVE);
	    goto Return;
	  }
	  case SELN_REQ_FAKE_LEVEL: {
	    Seln_level	target_level = (Seln_level)*context->request_pointer;
	    switch (target_level) {
	      case TEXTSW_UNIT_IS_LINE:
		cont_data->span_level = EI_SPAN_LINE;
		ev_span(textsw->views, cont_data->first, &cont_data->first,
			&cont_data->last_plus_one, EI_SPAN_LINE);
		*context->response_pointer++ = (caddr_t)target_level;
		break;
	      default:
		LINT_IGNORE(ASSUME(0));	/* Let implementor look at this. */
		*context->response_pointer++ = (caddr_t)(0xFFFFFFFF);
		break;
	    }
	    goto Return;
	  }
	  case SELN_REQ_FILE_NAME: {
	    char	*name;
	    int	continued, count;
	    int	str_first, str_last_plus_one;
	    if (textsw_file_name(textsw, &name) != 0) {
		*context->response_pointer++ = 0;  /* Null terminate value */
		goto Return;
	    }
	    /* For dbxtool: return full name, rather than base name ...
	    name = textsw_base_name(name);
	     */
	    str_first = (cont_data->current == ES_INFINITY)
			? 0 : cont_data->current;
	    str_last_plus_one = strlen(name);
	    count = textsw_copy_ascii_reply(
		str_first, str_last_plus_one,
		context->response_pointer,
		max_length, TCARF_STRING, name);
	    if (continued = (count >= TCAR_CONTINUED))
		count -= TCAR_CONTINUED;
	    context->response_pointer +=
		(count / sizeof(*context->response_pointer));
	    *context->response_pointer++ = 0;  /* Null terminate value */
	    if (continued) {
		cont_data->current = str_first+count;
		result = SELN_CONTINUED;
	    } else {
		cont_data->current = ES_INFINITY;
	    }
	    goto Return;
	  }
	  case SELN_REQ_FIRST:
	    *context->response_pointer++ = (caddr_t)cont_data->first;
	    goto Return;
	  case SELN_REQ_FIRST_UNIT:
	    switch (cont_data->span_level) {
	      case EI_SPAN_LINE: {
		int	line_number;
		if (cont_data->first == 0)
		    line_number = 0;
		else
		    line_number = ev_newlines_in_esh(textsw->views->esh,
						     0, cont_data->first);
		*context->response_pointer++ = (caddr_t)line_number;
		break;
	      }
	      case EI_SPAN_CHAR:
		/* Fall through */
	      default:	/* BUG ALERT: if in doubt, use char units. */
		*context->response_pointer++ = (caddr_t)cont_data->first;
	    }
	    goto Return;
	  case SELN_REQ_LAST:
	    *context->response_pointer++ =
		(caddr_t)(cont_data->last_plus_one-1);
	    goto Return;
	  case SELN_REQ_LAST_UNIT:
	    /* BUG ALERT: look at current level and adjust for it. */
	    *context->response_pointer++ = (caddr_t)-1;
	    goto Return;
	  case SELN_REQ_YIELD: {
	    result = textsw_seln_yield(textsw->first_view, context->rank);
	    *context->response_pointer++ = (caddr_t)result;
	    goto Return;
	  }
	  case SELN_REQ_END_REQUEST:
	    result = SELN_FAILED;	/* Ignored by caller! */
	    goto Return;
	  default:
	    result = SELN_UNRECOGNIZED;
	    goto Return;
	}
Return:
	switch (result) {
	  case SELN_SUCCESS:
	  case SELN_CONTINUED:
	  case SELN_UNRECOGNIZED:
	    break;
	  default:
	    /* We are not going to get another chance, so clean up. */
	    if (cont_data == &fast_continuation) {
		cont_data->in_use = FALSE;
	    } else {
		free((caddr_t)cont_data);
	    }
	    context->context = NULL;	/* Be paranoid! */
	}
	return(result);
}

static Seln_result
textsw_seln_yield(first_view, rank)
	Textsw_view		first_view;
	register Seln_rank	rank;
{
	register Textsw_folio	textsw = FOLIO_FOR_VIEW(first_view);
	unsigned		holder_flag = holder_flag_from_seln_rank(rank);

	if (holder_flag) {
	    switch (rank) {
	      case SELN_PRIMARY:
		textsw_set_selection(VIEW_REP_TO_ABS(textsw->first_view),
				     ES_INFINITY, ES_INFINITY, EV_SEL_PRIMARY);
		break;
	      case SELN_SECONDARY:
		textsw_set_selection(VIEW_REP_TO_ABS(textsw->first_view),
				     ES_INFINITY, ES_INFINITY,
				     EV_SEL_SECONDARY);
		textsw->track_state &= ~TXTSW_TRACK_SECONDARY;
		break;
	      case SELN_SHELF:
		if (textsw->trash) {
		    es_destroy(textsw->trash);
		    textsw->trash = ES_NULL;
		}
		break;
	    }
	    textsw->holder_state &= ~holder_flag;
	    return(SELN_SUCCESS);
	} else {
	    return(SELN_DIDNT_HAVE);
	}
}
