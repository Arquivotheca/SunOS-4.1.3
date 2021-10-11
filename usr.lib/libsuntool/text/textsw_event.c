#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)textsw_event.c 1.1 92/07/30";
#endif
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * New style, notifier-based, event and timer support by text subwindows.
 */

#include <suntool/primal.h>
#include <suntool/textsw_impl.h>
#include <errno.h>
#include <fcntl.h>

#ifdef KEYMAP_DEBUG
#include "../../libsunwindow/win/win_keymap.h"
#else
#include <sunwindow/win_keymap.h>
#endif

extern int		errno;
extern void		pw_batch();	/* Used by pw_batch_on/off */
extern Notify_error	win_post_event();
extern Ev_status	ev_set();
extern void		ev_blink_caret();

#define	invert_caret_if_idle(textsw)					\
	if (!TXTSW_IS_BUSY(textsw)) textsw_invert_caret(textsw)
	
#define	ESC		27

pkg_private Textsw_folio
textsw_folio_for_view(view)
	Textsw_view	view;
{
	ASSUME(view->magic == TEXTSW_VIEW_MAGIC);
	ASSUME(view->folio->magic == TEXTSW_MAGIC);
	return(view->folio);
}

pkg_private Textsw_view
textsw_view_abs_to_rep(abstract)
	Textsw	abstract;
{
	Textsw_view	view = (Textsw_view)LINT_CAST(abstract);

	ASSUME((view == 0) || (view->magic == TEXTSW_VIEW_MAGIC));
	return(view);
}

pkg_private Textsw
textsw_view_rep_to_abs(rep)
	Textsw_view	rep;
{
	ASSUME((rep == 0) || (rep->magic == TEXTSW_VIEW_MAGIC));
	return((Textsw)rep);
}

static int
is_left_arrow_key(folio, fd, ie)
    	Textsw_folio	folio;
    	int		fd;
    	Event		*ie;
{
	int 		bytes_read;
	errno = 0;
	
	bytes_read = read(fd, (char *)&folio->pending_events[0], (sizeof(Event)));
	if (bytes_read == sizeof(Event)) {
	    if (event_id(&folio->pending_events[0]) == '[') {
	        bytes_read = read(fd, (char *)&folio->pending_events[1],
							  (sizeof(Event)));
	        if (bytes_read == sizeof(Event)) {
	                switch (event_id(&folio->pending_events[1])) {
		    	    case 'A':
		    	        event_id(ie) = ACTION_GO_COLUMN_BACKWARD;
		    	        break;
		    	    case 'B':
		    	        event_id(ie) = ACTION_GO_COLUMN_FORWARD;
		    	        break;
		    	    case 'C':
		    	        event_id(ie) = ACTION_GO_CHAR_FORWARD;
		    	        break;
		    	    case 'D':
		    	        event_id(ie) = ACTION_GO_CHAR_BACKWARD;
		    	        break;
		    	    default:
		    	        folio->pending_event_count = 2;
		    	        break;           		    	     
		    	}
	        } /* else toss [ also */
	    } else
	        folio->pending_event_count = 1;
	}
	
	if (folio->pending_event_count == 0) {
	    if (bytes_read == -1) {
	        if (errno != EWOULDBLOCK) {
		    perror("textsw: input failed");
	            return(-1);
	        }
	    } else if ((bytes_read % sizeof(Event)) != 0) {
	        perror("textsw: Partial event read.");
	        return(-1);
	    }
	}
	return(0);   /* not an arrow key, but otherwise success */
}

extern Notify_value
textsw_input_func(abstract, fd)
	Textsw			 abstract;
	register int		 fd;
{
	register Event		 this_ie;
	register int		 loop_count;
	register Textsw_view	 view = VIEW_ABS_TO_REP(abstract);
	register Textsw_folio	 folio = FOLIO_FOR_VIEW(view);

	/* Keymap debugging statement; to be removed later */
	
	char str[64];
	
	/* NOTE:
	 * we used to ASSERT(view->window_fd == fd);
	 * we do not any more because cmdsw violates this assumption
	 * while switching from ttysw to text mode, to transfer events
	 * queued for the ttysw to the text subwindow.
	 */
	folio->state |= TXTSW_DOING_EVENT;
	folio->pending_event_count = 0;
	
	for (loop_count = 0; ;loop_count++) {
	    /* Following test and action makes pending deletes happen after
	     *   processing the first batch of input events.  Otherwise, the
	     *   processing can be held off indefinitely as long as the user
	     *   stays ahead of the textsw.
	     */
	    if (loop_count == 1) {
		textsw_flush_caches(view, TFC_STD);
	    }
	    /* Kernel now does LOC_MOVE compression so only read one
	     * event at a time (allowing code called from here to reach back
	     * into the kernel to find out about the state of the input
	     * devices as viewed by the kernal at the current virtual time).
	     */

		/*
		 *	Keymapping modification
		 */
		        if ((read(fd, (char *)&this_ie, sizeof(this_ie))) == -1) {
		            if (errno != EWOULDBLOCK || loop_count == 0) 
		 	        perror("textsw: input failed");
			    goto Return;			    
		        }
		        
			if ((event_id(&this_ie) == ESC)  && 	
		    	    (is_left_arrow_key(folio, fd, &this_ie) < 0)) 
		    	     goto Return;     	   
		    	
		   (void)win_keymap_map(fd, &this_ie);
#ifdef PRINT_EVENT_CODES
fprintf(stderr, "Code is %d:%d%s, shiftmask is %d\n",
	this_ie.ie_code/256, this_ie.ie_code%256,
	(this_ie.ie_flags == 1) ? "-" : "+", this_ie.ie_shiftmask);
#endif

#ifdef KEYMAP_DEBUG
		/* Keymap debugging statement; to be removed later */
			
		if (keymap_debug_mask & KEYMAP_SHOW_EVENT_STREAM)
			printf("textsw_input_func: posted '%s'\n",
					win_keymap_event_decode(&this_ie, str));
#endif

	    switch (win_post_event((Notify_client)abstract, &this_ie,
		NOTIFY_SAFE)) {
	      case NOTIFY_OK:
		break;
	      default:
		goto Return;
	    }
	}
Return:
	if ((folio->to_insert_next_free != folio->to_insert) &&
	    ((folio->func_state & TXTSW_FUNC_FILTER) == 0)) {
	    textsw_flush_caches(view, TFC_STD);
	}
	folio->state &= ~TXTSW_DOING_EVENT;
	return(NOTIFY_DONE);
}

/* ARGSUSED */
extern Notify_value
textsw_event(abstract, event, arg, type)
	Textsw			 abstract;
	register Event		*event;
	Notify_arg		 arg;
	Notify_event_type	 type;		/* Currently unused */
{
	Textsw_view		 view = VIEW_ABS_TO_REP(abstract);
	register Textsw_folio	 folio = FOLIO_FOR_VIEW(view);
	register int		 process_status;

	switch (event_id(event)) {
	  case KBD_USE:
	    textsw_take_down_caret(folio);
	    textsw_add_timer(folio, &folio->timer);
	    folio->state |= TXTSW_HAS_FOCUS;
	    (void) ev_set(view->e_view, EV_CHAIN_CARET_IS_GHOST, FALSE, 0);
	    if (folio->tool) {
		tool_kbd_use(TOOL_FROM_FOLIO(folio), (caddr_t)LINT_CAST(view));
	    }
	    goto Default;
	  case KBD_DONE:
	    folio->state &= ~TXTSW_HAS_FOCUS;
	    textsw_take_down_caret(folio);	/* resets timer */
	    (void) ev_set(view->e_view, EV_CHAIN_CARET_IS_GHOST, TRUE, 0);
	    if (folio->tool) {
		tool_kbd_done(TOOL_FROM_FOLIO(folio),(caddr_t)LINT_CAST(view));
	    }
	    goto Default;
	  case KBD_REQUEST:
	    if (TXTSW_IS_READ_ONLY(folio) ||
	        (textsw_determine_selection_type(folio, 0) &
	    	    EV_SEL_SECONDARY) ) {
		(void) win_refuse_kbd_focus(view->window_fd);
	    }
	    goto Default;
	  default: {
Default:
	    process_status = textsw_process_event(abstract, event, arg);
	    if (process_status & TEXTSW_PE_READ_ONLY) {
		textsw_read_only_msg(folio, event_x(event), event_y(event));
 	    } else if (process_status == 0) {
		return(NOTIFY_IGNORED);
	    }
 	    break;
 	  }
	}
	return(NOTIFY_DONE);
}

/* ARGSUSED */
pkg_private Notify_value
textsw_timer_expired(textsw, which)
	register Textsw_folio	textsw;
	int			which;		/* Currently unused */
{
	invert_caret_if_idle(textsw);
	if (TXTSW_IS_READ_ONLY(textsw)) {
	} else if (((textsw->state & TXTSW_HAS_FOCUS) &&
		    (textsw->caret_state & TXTSW_CARET_FLASHING)) ||
		   !(textsw->caret_state & TXTSW_CARET_ON))
	    /* if caret is flashing and have kbd focus, or if caret is off */
	    textsw_add_timer(textsw, &textsw->timer);
	return(NOTIFY_DONE);
}

pkg_private void
textsw_add_timer(textsw, timeout)
	register Textsw_folio	 textsw;
	register struct timeval	*timeout;
{
	struct itimerval	itimer;

	itimer = NOTIFY_NO_ITIMER;
	itimer.it_value = *timeout;
	if (NOTIFY_FUNC_NULL ==
	    notify_set_itimer_func((Notify_client)textsw,
				   textsw_timer_expired, ITIMER_REAL,
				   &itimer, (struct itimerval *)0))
	    notify_perror("textsw_add_timer");
}

pkg_private void
textsw_remove_timer(textsw)
	register Textsw_folio	textsw;
{
	textsw_add_timer(textsw, &NOTIFY_NO_ITIMER.it_value);
	if (textsw->caret_state & TXTSW_CARET_ON) {
	    textsw_invert_caret(textsw);
	}
}

pkg_private void
textsw_take_down_caret(textsw)
	register Textsw_folio	textsw;
{
	if (textsw->caret_state & TXTSW_CARET_ON) {
	    textsw_invert_caret(textsw);
	}
	/* if caret is not flashing or we don't have kbd focus */
	if (!(textsw->caret_state & TXTSW_CARET_FLASHING) ||
	    !(textsw->state & TXTSW_HAS_FOCUS)) {
	    /* if timer is not already set, set the timer */
	    if (notify_get_itimer_func((Notify_client)textsw, ITIMER_REAL)
	    ==  NOTIFY_FUNC_NULL)
		textsw_add_timer(textsw, &textsw->timer);
	}
}

pkg_private void
textsw_invert_caret(textsw)
	register Textsw_folio	textsw;
{
	
	if (TXTSW_IS_READ_ONLY(textsw) &&
	    ((textsw->caret_state & TXTSW_CARET_ON) == 0))
	    return;
	ev_blink_caret(textsw->views, !(textsw->caret_state & TXTSW_CARET_ON));
	textsw->caret_state ^= TXTSW_CARET_ON;
}

