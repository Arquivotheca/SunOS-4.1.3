#ifndef lint
static  char sccsid[] = "@(#)ws_dispense.c 1.1 92/07/30";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * SunWindows Workstation code that is responsible for handling the
 * top of the input queue, dispensing events at the appropriate times
 * to the appropriate windows.
 */

#include <sunwindowdev/wintree.h>
#include <sys/errno.h>
#include <sys/kernel.h>	/* for time */
#include <pixrect/pr_planegroups.h>	/* for plane groups */

static void	ws_set_dtop_grabio();
void	 	ws_set_dtop_waiting();

void dtop_restore_grnds();
void dtop_save_grnds();
void dtop_set_bkgnd();

/* TODO: How about some sort of meta mask? */
/* TODO: Worry about ALL_LOCKS */

#ifdef WINDEVDEBUG
int ws_qget_debug;	/* Temp: Debugging messages when dequeue firm event */
int	ws_qputback_debug;
	/* Temp: Debugging messages when putback firm event */
int	ws_set_focus_debug;
	/* Temp: Debugging messages when ws_set_focus called */
int	ws_track_debug;
	/* Temp: Debug messages in ws_set_pick_focus_next */
#endif
int	ws_focus_during_each_transit;
	/* Temp: Set focus if loc moves & in transit*/

int ws_verbose_event_lock; /* Turn on event lock broken messages */
int ws_no_in_transit;
int ws_no_collapse;

extern	int ws_q_collapse_trigger;	/* # q items that trigger collapse */

#define	ws_is_loc_change(id) \
	    ((((id) >= LOC_FIRST_DELTA) && ((id) <= LOC_LAST_ABSOLUTE)) || \
	    ((id) == LOC_MOVE))

Firm_event ws_get_event();

int	ws_lock_control_pid;	/* Used in lock control enumerator */
int	ws_empty_input_flag;	/* Used in lock control enumerator */ 

ws_window_nudge(w)
	Window *w;
{
	register Workstation *ws;
	register Desktop *dtop;

	dtop = w->w_desktop;
	if (dtop == DESKTOP_NULL)
		return;
	win_sharedq_shift(w);
	ws = dtop->dt_ws;
	if (ws == WORKSTATION_NULL)
		return;
	/*
	 * If window's process is currently unsyncronized, see if can 
	 * allow it to acquire the event lock.
	 */
	if (w->w_flags & WF_UNSYNCHRONIZED)
		ws_try_event_enable(w);
	/* If no current event lock holder then nudge next event off queue */
	if (~ws->ws_flags & WSF_LOCKED_EVENT)
		ws_nudge(ws);
	/*
	 * If window's process is current event lock holder & there is
	 * no input on window's queue then nudge next event (if any) off
	 * of queue.  Before do that, unlock current event lock because
	 * next event may not be for this window.
	 */
	else if ((ws->ws_event_consumer != WINDOW_NULL) &&
	    (ws->ws_event_consumer->w_pid == w->w_pid) &&
	    (dtop->dt_sharedq_owner == w ?
	    (dtop->shared_info->shared_eventq.head ==
	     dtop->shared_info->shared_eventq.tail) :
	    (ws->ws_event_consumer->w_input.c_cc <
	     sizeof (struct inputevent)))) {
		wlok_unlock(&ws->ws_eventlock);
		ws_nudge(ws);
	}
}

/*
 * Try to send an event to the one of the input focuses.
 * Any caller of ws_nudge that has the event lock should unlock it first.
 *
 * We preserve the user's preception of the ordering amoung LOC_WINENTER,
 * LOC_WINEXIT, KBD_REQUEST, KBD_USE, and KBD_DONE, reguardless of kbd
 * focus change event type.  However, KBD_REQUESTs are not issued when
 * LOC_WINENTER is the kbd focus change event.  Here is the order:
 * 
 * 	LOC_WINEXIT
 * 	LOC_WINENTER
 * 	KBD_REQUEST
 * 	KBD_DONE
 * 	KBD_USE
 * 
 * It might seem that the KBD_DONE ought to follow the  LOC_WINEXIT in the
 * LOC_WINEXIT kbd focus change event case.  But this is hard to implement
 * and doesn't move well to a MS_LEFT kbd focus change event case.
 */
ws_nudge(ws)
	register Workstation *ws;
{
	Firm_event firm_event; /* Gotten from input q */
	Firm_event fe;	/* Contructed */


	/*
	 * See if some process/window in middle of processing input.
	 */
	if (ws->ws_flags & WSF_LOCKED_EVENT)
		return;
	/* Collapse locator events at top of queue to keep from being full */
	ws_collapse_q(ws);
	/*
	 * Go around focus change state machine until need to waiting
	 * for event consumer to clear the event lock.
	 */
	while (~ws->ws_flags & WSF_LOCKED_EVENT) {
		/* Pre-set fe fields */
		fe.value = 1;
		    /* Set to one because event has no neg */
		if (ws->ws_flags & WSF_SEND_FOCUS_EVENT)
			fe.time = ws->ws_focus_event.time;
		else if (!vq_is_empty(&ws->ws_q))
			fe.time = ws->ws_q.top->firm_event.time;
		else
			fe.time = time;
		fe.pair = 0;
		fe.pair_type = FE_PAIR_NONE;

		switch (ws->ws_focus_state) {

		case SEND_EXIT:
			/* Send LOC_WINEXIT to ws_pickfocus */
			fe.id = LOC_WINEXIT;
			if (ws->ws_pickfocus)
				(void) win_send_one(ws->ws_pickfocus,
				    &fe, &ws->ws_pickfocus->w_pickmask,
				    INPUTMASK_NULL);
			/* Set in-transit flag */
			if (!ws_no_in_transit)
				if (ws->ws_flags & WSF_LATEST_WAS_MOTION)
					ws->ws_flags |= WSF_LOC_IN_TRANSIT;
			/* Move to next state */
			ws->ws_focus_state = SEND_ENTER;
			break;

		case SEND_ENTER:
			/*
			 * Do in-transit processing, by conditionally gliding
			 * over windows without waking them up.
			 */
			ws_window_transit(ws);
			/* See if completed focus transition */
			if (ws->ws_flags & WSF_LOC_IN_TRANSIT) {
				/*
				 * See if should ignore in-transit locator
				 * event surpression.
				 */
				if(ws->ws_pickfocus_next &&
				  (ws->ws_pickfocus_next->w_pickmask.im_flags &
				  IM_INTRANSIT)) {
					/* Continue */
				} else
					return;
			}
			ws_set_focus(ws, FF_PICK_CHANGE);
			/* Send LOC_WINENTER to ws_pickfocus_next */
			fe.id = LOC_WINENTER;
			if (ws->ws_pickfocus_next)
				(void) win_send_one(ws->ws_pickfocus_next,
				    &fe, &ws->ws_pickfocus_next->w_pickmask,
				    INPUTMASK_NULL);
			/* Update ws_pickfocus */
			ws->ws_pickfocus = ws->ws_pickfocus_next;
			/* Move to next state */
			ws->ws_focus_state = SEND_Q_TOP;
			break;

		case SEND_DIRECT_REQUEST:
			ws_send_kbd_request(ws, ws->ws_kbdfocus_next, &fe);
			break;

		case SEND_REQUEST:
			ws_send_kbd_request(ws, ws->ws_pickfocus, &fe);
			break;

		case REQUEST_WAIT:
			/*
			 * If requesting flag is set then didn't disapprove
			 * of the request.
			 */
			if (ws->ws_flags & WSF_KBD_REQUEST_PENDING) {
				/* Clear requesting flag */
				ws->ws_flags &= ~WSF_KBD_REQUEST_PENDING;
				/* Move to next state */
				ws->ws_focus_state = SEND_DONE;
			} else {
				/* Reset ws_kbdfocus_next */
				ws->ws_kbdfocus_next = ws->ws_kbdfocus;
				/* Move to next state */
				ws->ws_focus_state = SEND_FOCUS_EVENT;
			}
			break;

		case SEND_DONE:
			/* Send KBD_DONE to ws_kbdfocus */
			fe.id = KBD_DONE;
			if (ws->ws_kbdfocus)
				(void) win_send_one(ws->ws_kbdfocus,
				    &fe, &ws->ws_kbdfocus->w_kbdmask,
				    INPUTMASK_NULL);
			/* Move to next state */
			ws->ws_focus_state = SEND_USE;
			break;

		case SEND_USE:
			/* Send KBD_USE to ws_kbdfocus_next */
			fe.id = KBD_USE;
			if (ws->ws_kbdfocus_next)
				(void) win_send_one(ws->ws_kbdfocus_next,
				    &fe, &ws->ws_kbdfocus_next->w_kbdmask,
				    INPUTMASK_NULL);
			/* Update ws_kbdfocus */
			ws->ws_kbdfocus = ws->ws_kbdfocus_next;
			/* Move to next state */
			if ((ws->ws_flags & WSF_SWALLOW_FOCUS_EVENT) ||
			    (!(ws->ws_flags & WSF_SEND_FOCUS_EVENT)))
				ws->ws_focus_state = SEND_Q_TOP;
			else
				ws->ws_focus_state = SEND_FOCUS_EVENT;
			ws->ws_flags &= ~WSF_SWALLOW_FOCUS_EVENT;
			break;

		case SEND_FOCUS_EVENT:
			/*
			 * Send ws_focus_event to kbd or pick focus.  If did a
			 * win_send on this event then would have windows
			 * jumping around because of passing, for example,
			 * MS_LEFT's up to a tool parent (which cause a top).
			 */
			if ((ws->ws_kbdfocus == WINDOW_NULL) ||
			    (win_send_one(ws->ws_kbdfocus, &ws->ws_focus_event,
			      &ws->ws_kbdfocus->w_kbdmask,
			      INPUTMASK_NULL) == -1)) {
				if (ws->ws_pickfocus) {
					(void) win_send_one(ws->ws_pickfocus,
					    &ws->ws_focus_event,
					    &ws->ws_pickfocus->w_pickmask,
					    INPUTMASK_NULL);
				}
			}
			/* Clear focus event flags */
			ws->ws_flags &= ~WSF_SEND_FOCUS_EVENT;
			/* Move to next state */
			ws->ws_focus_state = SEND_Q_TOP;
			break;

		case SEND_Q_TOP: /* Could break into smaller states */
			/*
			 * Set state if new focuses and break.  Gets any focus
			 * changes since last got to this state.
			 */
			if (ws_set_focus_state(ws))
				break;
			/*
			 * Get top of q and use this event to try to change
			 * focuses.  Nothing to do if the input queue is empty.
			 */
			if (vq_is_empty(&ws->ws_q))
				return;
			firm_event = ws_get_event(ws);
			/*
			 * Set focus if LOC_MOVE.  Also, if LOC_STILL set the
			 * focus because all the LOC_MOVE's may have been
			 * collapsed away.
			 *
			 * Order of swallow vs pass thru test is important.
			 * Since swallow contricts what should be done, it
			 * comes first.
			 */
			if ((firm_event.id == LOC_MOVE) ||
			    (firm_event.id == LOC_STILL))
				ws_set_focus(ws, FF_PICK_CHANGE);
			else if ((firm_event.id == ws->ws_kbd_focus_sw.id) &&
			    ws_focus_match(ws, &ws->ws_kbd_focus_sw,
			    &firm_event)) {
				ws->ws_flags |= WSF_SWALLOW_FOCUS_EVENT;
				ws->ws_focus_event = firm_event;
				ws_set_focus(ws, FF_KBD_EVENT);
			} else if ((firm_event.id == ws->ws_kbd_focus_pt.id) &&
			    ws_focus_match(ws, &ws->ws_kbd_focus_pt,
			    &firm_event)) {
				ws->ws_flags &= ~WSF_SWALLOW_FOCUS_EVENT;
				ws->ws_focus_event = firm_event;
				ws_set_focus(ws, FF_KBD_EVENT);
			}
			/* Send firm_event if no new focuses */
			if (ws_set_focus_state(ws))
				break;
			else {
				/* Clear focus event flag if haven't yet */
				ws->ws_flags &= ~WSF_SEND_FOCUS_EVENT;
				/* Send batched events all at once */
				if (firm_event.id == VLOC_BATCH)
					ws_send_batch(ws, firm_event.value);
				else
					win_send(ws, &firm_event);
			}
			break;

#ifdef WINDEVDEBUG
		default:
			printf("Unknown input state\n");
#endif
		}
	}
}

ws_send_kbd_request(ws, w, fe)
	Workstation *ws;
	Window *w;
	Firm_event *fe;
{
	/*
	 * If LOC_WINENTER is not the kbd focus change event and
	 * event timeout is not zero (both of which make a
	 * kbd change request not workable) then give
	 * application a chance to say no to change.
	 */
	fe->id = KBD_REQUEST;
	if (((ws->ws_kbd_focus_sw.id != LOC_WINENTER) ||
	    (ws->ws_kbd_focus_pt.id != LOC_WINENTER)) &&
	    (timerisset(&ws->ws_eventtimeout)) &&
	    (w != WINDOW_NULL) &&
	    (win_send_one(w, fe, &w->w_pickmask, INPUTMASK_NULL) != -1)) {
		/* Set requesting flag */
		ws->ws_flags |= WSF_KBD_REQUEST_PENDING;
		/* Move to next state */
		ws->ws_focus_state = REQUEST_WAIT;
	} else
		/* Move to next state */
		ws->ws_focus_state = SEND_DONE;
}

int
ws_focus_match(ws, fs, fe)
	Workstation *ws;
	Ws_focus_set *fs;
	Firm_event *fe;
{
	if ((fe->id == fs->id) &&
	    (vuid_get_value(ws->ws_instate, fe->id) == fs->value) &&
	    ((fs->shifts == WS_FOCUS_ANY_SHIFT) ||
	     (((fs->shifts & ws->ws_shiftmask) ^ fs->shifts) == 0)))
		return (1);
	else
		return (0);
}

/* Return 1 if change focus state due to new focuses, return 0 otherwise */
int
ws_set_focus_state(ws)
	register Workstation *ws;
{
	/* Do error check */
	if (ws->ws_focus_state != SEND_Q_TOP) {
#ifdef WINDEVDEBUG
		printf("Focus state should be SEND_Q_TOP %D\n",
		    ws->ws_focus_state);
#endif
		ws->ws_focus_state = SEND_Q_TOP;
		return (1);
	}
	/*
	 * If current pick focus not equal to next pick focus
	 * then set to new state (SEND_ENTER or SEND_EXIT)
	 * and return 1.
	 */
	if (ws->ws_pickfocus != ws->ws_pickfocus_next) {
		if (ws->ws_pickfocus == WINDOW_NULL)
			ws->ws_focus_state = SEND_ENTER;
		else
			ws->ws_focus_state = SEND_EXIT;
		return (1);
	}
	/*
	 * If current kbd focus not equal to next kbd focus
	 * then set to new state (SEND_REQUEST) and return 1.
	 */
	if (ws->ws_kbdfocus != ws->ws_kbdfocus_next) {
		ws->ws_focus_state = SEND_REQUEST;
		return (1);
	}
	return (0);
}

/* Conditionally collapse locator events at top of input queue */
ws_collapse_q(ws)
	register Workstation *ws;
{
	Firm_event peek_fe;
	Firm_event collapse_fe;
	int collapsed = 0;
	extern u_int ws_vq_node_bytes;
	extern void ws_shrink_queue();

	if (ws_no_collapse)
		return;
	if (vq_used(&ws->ws_q) == 0 && ws->ws_qbytes > ws_vq_node_bytes)
		ws_shrink_queue(ws);
	/*
	 * Don't collapse if going to send focus event because will mess up
	 * shift mask state and loc positions when get around to sending, later.
	 */
	if (ws->ws_flags & WSF_SEND_FOCUS_EVENT)
		return;
	/* Don't collapse if pick focus is tracking trajectory */
#ifndef lint
	if ((ws->ws_pickfocus != WINDOW_NULL) &&
	    (win_getinputcodebit(&ws->ws_pickfocus->w_pickmask,LOC_TRAJECTORY)))
		return;
#endif
	while ((vq_used(&ws->ws_q) > 1) && /* Check possibility of collapse */
	    (vq_used(&ws->ws_q) > ws_q_collapse_trigger) &&
	    (vq_peek(&ws->ws_q, &peek_fe) != VUID_Q_EMPTY)) {
		if ((ws_is_loc_change(peek_fe.id)) ||
		    (peek_fe.id == LOC_STILL)) {
			collapse_fe = ws_get_event(ws);
			collapsed = 1;
		} else
			break;
	}
	/*
	 * Put back last event taken so don't wipe out event trigger
	 * and loose an event.
	 */
	if (collapsed) {
		if (ws_is_loc_change(collapse_fe.id))
			collapse_fe.id = LOC_MOVE;
		/* else will be LOC_STILL */
		collapse_fe.value = 1; /* Set to one because event has no neg */
		/* use given time */
		collapse_fe.pair = 0;
		collapse_fe.pair_type = FE_PAIR_NONE;
#ifdef	WS_DEBUG
if (ws_qputback_debug) printf("q putback id %D value %D avail %D\n",
    collapse_fe.id, collapse_fe.value, vq_avail(&ws->ws_q));
#endif	WS_DEBUG
		if (vq_putback(&ws->ws_q, &collapse_fe) != VUID_Q_OK)
#ifdef WINDEVDEBUG
			printf("putback error\n");
#else
			;
#endif
	}
}

/*
 * Non-wakeup window transition capability over windows which are looking
 * for LOC_MOVEs, LOC_WINENTERs, LOC_WINEXITs, KBD_USE and KBD_DONE.
 */
ws_window_transit(ws)
	register Workstation *ws;
{
	Firm_event firm_event;
	int dequeued_loc_change = 0;

	while (ws->ws_flags & WSF_LOC_IN_TRANSIT) {
		/* Peek at top of input queue */
		if (vq_peek(&ws->ws_q, &firm_event) == VUID_Q_EMPTY)
			break;
		if (!ws_is_loc_change(firm_event.id)) {
			ws->ws_flags &= ~WSF_LOC_IN_TRANSIT;
		} else  {
			/* Get locator motion from input queue */
			firm_event = ws_get_event(ws);
			dequeued_loc_change = 1;
		if (ws_focus_during_each_transit)
			ws_set_focus(ws, FF_PICK_CHANGE);
		}
	}
	if (dequeued_loc_change && !ws_focus_during_each_transit)
		ws_set_focus(ws, FF_PICK_CHANGE);
}

/*
 * Depending on flags and ws fields update either ws_kbdfocus_next or
 * ws_pickfocus_next or both.  FF_PICK_CHANGE or FF_KBD_EVENT are the flags.
 *
 * This routine is called in a variety of circumstances:
 *
 *	1) A user action has been pulled off the top of the input q that might
 *	   require a change in focus.
 *	2) A window is being closed that held one or both of the input focuses
 *	   thus forcing the selection of a new focus.
 *	3) A change of window clipping, visibility or position that might
 *	   affect locator position dependent focus changing (the pick focus
 *	   will surely be affected).  Programmatic setting of the locator
 *	   position is a similar situation.
 *	4) When a workstation is first started and there is no focus.
 *	5) When a window has grabbed or released the input focus.
 *	6) When trying to complete a focus transition.
 *
 * A focus is never null unless starting out or current focus closed.
 * There is enough difference between a LOC_WINENTER based kbd focus change
 * event and others (say MS_LEFT) that we treat the others separately.
 */
void
ws_set_focus(ws, flags)
	register Workstation *ws;
	int flags;
{
#ifdef	WS_DEBUG
if (ws_set_focus_debug) printf(
    "ws_set_focus start: flags %X k %X kn %X p %X pn %X\n",
    flags,
    ws->ws_kbdfocus, ws->ws_kbdfocus_next,
    ws->ws_pickfocus, ws->ws_pickfocus_next);
#endif	WS_DEBUG
	/* Check pick focus if might have changed */
	if (flags & (FF_PICK_CHANGE | FF_KBD_EVENT))
		ws_set_pick_focus_next(ws);
	/* If kbd focus event is LOC_WINENTER follow pick focus */
	if (ws->ws_kbd_focus_sw.id == LOC_WINENTER &&
	    ws->ws_kbd_focus_pt.id == LOC_WINENTER)
		ws->ws_kbdfocus_next = ws->ws_pickfocus_next;
	/* Set both focuses from ws_inputgrabber */
	if (ws->ws_inputgrabber != WINDOW_NULL) {
		ws->ws_pickfocus_next = ws->ws_inputgrabber;
		ws->ws_kbdfocus_next = ws->ws_inputgrabber;
		return;
	}
	/* Update kbd focus on a focus change event */
	if (flags & FF_KBD_EVENT) {
		ws->ws_flags |= WSF_SEND_FOCUS_EVENT;
		ws->ws_kbdfocus_next = ws->ws_pickfocus_next;
	}
#ifdef	WS_DEBUG
if (ws_set_focus_debug) printf("ws_set_focus end: k %X kn %X p %X pn %X\n",
    ws->ws_kbdfocus, ws->ws_kbdfocus_next,
    ws->ws_pickfocus, ws->ws_pickfocus_next);
#endif	WS_DEBUG
}

/*
 * Take next item off input queue.
 * Take two if locator changes if have same time.
 */
Firm_event
ws_get_event(ws)
	register Workstation *ws;
{
	Firm_event firm_event;
	register Desktop *dtop_pick = ws->ws_pick_dtop;

Again:
	/* Get top of input queue */
	if (vq_get(&ws->ws_q, &firm_event) == VUID_Q_EMPTY) {
#ifdef WINDEVDEBUG
		printf("Window input: tried to read empty q\n");
#endif
	}
#ifdef WINDEVDEBUG
if (ws_qget_debug) printf("q get id %D value %D avail %D sec %D usec %D\n",
    firm_event.id, firm_event.value, vq_avail(&ws->ws_q),
    firm_event.time.tv_sec, firm_event.time.tv_usec);
#endif
	/* Turn locator motion into LOC_MOVE */
	if (ws_is_loc_change(firm_event.id)) {
		Firm_event loc_event;

		switch (firm_event.id) {

		case LOC_X_DELTA:
			dtop_pick->dt_ut_x += firm_event.value;
			break;

		case LOC_Y_DELTA:
			dtop_pick->dt_ut_y += firm_event.value;
			break;

		case LOC_X_ABSOLUTE:
			dtop_pick->dt_ut_x = firm_event.value;
			break;

		case LOC_Y_ABSOLUTE:
			dtop_pick->dt_ut_y = firm_event.value;
			break;

		case LOC_MOVE:
			break;

		default:
			{}
		}
		ws->ws_flags |= WSF_LATEST_WAS_MOTION;
		if (vq_peek(&ws->ws_q, &loc_event) == VUID_Q_EMPTY)
			goto LocMove;
		/* See if should merge with firm_event in to one event */
		if ((ws_is_loc_change(loc_event.id)) &&
		    (tv_equal(loc_event.time, firm_event.time)))
			goto Again;
LocMove:
		firm_event.id = LOC_MOVE;
		firm_event.value = 1; /* Set to one because event has no neg */
		firm_event.pair = 0;
		firm_event.pair_type = FE_PAIR_NONE;
		/* use given time */
	} else {
		/* Set user time state */
		vuid_set_value(&ws->ws_instate, &firm_event);
		/* Maintain shiftmask state for win_sendevent */
		ws_set_shiftmask(ws, &firm_event);
		/* Maintain latest motion flag */
		if (firm_event.id == LOC_STILL)
			ws->ws_flags &= ~WSF_LATEST_WAS_MOTION;
	}
	return (firm_event);
}

ws_flush_input(ws)
	register Workstation *ws;
{
	register Desktop *dtop;

	while (!vq_is_empty(&ws->ws_q))
		(void) ws_get_event(ws);
	vuid_destroy_state(ws->ws_instate);
	ws->ws_instate = vuid_copy_state(ws->ws_rtstate);
	if ((dtop = ws->ws_loc_dtop) != DESKTOP_NULL) {
		dtop->dt_ut_x = dtop->dt_rt_x;
		dtop->dt_ut_y = dtop->dt_rt_y;
	}
	bzero((caddr_t)&ws->ws_surpress_mask, sizeof (ws->ws_surpress_mask));
}

ws_set_shiftmask(ws, fe)
	register Workstation *ws;
	register Firm_event *fe;
{
#define	shift_mask_update(ws, fe, bit)			\
	if ((fe)->value)				\
		(ws)->ws_shiftmask |= (1 << (bit));	\
	else						\
		(ws)->ws_shiftmask &= ~(1 << (bit));

	switch (fe->id) {

	case SHIFT_CAPSLOCK:
		shift_mask_update(ws, fe, CAPSLOCK);
		break;

	case SHIFT_LOCK:
		shift_mask_update(ws, fe, SHIFTLOCK);
		break;

	case SHIFT_LEFT:
		shift_mask_update(ws, fe, LEFTSHIFT);
		break;

	case SHIFT_RIGHT:
		shift_mask_update(ws, fe, RIGHTSHIFT);
		break;

	case SHIFT_LEFTCTRL:
		shift_mask_update(ws, fe, LEFTCTRL);
		break;

	case SHIFT_RIGHTCTRL:
		shift_mask_update(ws, fe, RIGHTCTRL);
		break;

	case SHIFT_META:
#define	META_SHIFT_BIT	6
		/*
		 * Use of 6 instead of constant: Done at last minute in 3.0
		 * release so would have meta mask.  Wanted to use METAMASK
		 * (from kbd.h) but value is the same as the no longer
		 * supported UPMASK.  Chose to use 6 because corresponds to
		 * the "unused...		0x0040" value in kbd.h.
		 * META_SHIFT_MASK is in win_input.h & need to claim that it
		 * is used in kbd.h.
		 */
		shift_mask_update(ws, fe, META_SHIFT_BIT);
		break;

	case SHIFT_ALT:
		shift_mask_update(ws, fe, ALT);
		break;

	default:
			/* No longer supporting UPMASK or CTLSMASK */
		{}
	}
}

ws_set_pick_focus_next(ws)
	Workstation *ws;
{
	Desktop *dtop_pick = ws->ws_pick_dtop;	/* can't be register */
	int x_now, y_now;	/* can't be register */

	if ((dtop_pick == NULL) || (dtop_pick->dt_rootwin == NULL))
		return;

	/* Clamp locator to desktop or move onto adjoinning desktop */
	x_now = dtop_pick->dt_ut_x;
	y_now = dtop_pick->dt_ut_y;
#ifdef	WS_DEBUG
if (ws_track_debug) printf("TRACK dtop=%X, x=%D, y=%D\n",
dtop_pick, x_now, y_now);
#endif	WS_DEBUG
	dtop_track_locator(&dtop_pick, &x_now, &y_now);
#ifdef	WS_DEBUG
if (ws_track_debug) printf("RESULT dtop=%X, x=%D, y=%D\n",
dtop_pick, x_now, y_now);
#endif	WS_DEBUG
	/* Reset pick desktop */
	ws->ws_pick_dtop = dtop_pick;
	/* Update dtop_pick's notion of locator position */
	dtop_pick->dt_ut_x = x_now;
	dtop_pick->dt_ut_y = y_now;

	/* Use locator position to determine pick focus */
	ws->ws_pickfocus_next = wt_intersected(dtop_pick->dt_rootwin,
	    x_now, y_now);
}

ws_send_batch(ws, n)
	register Workstation *ws;
	int n;
{
	Firm_event firm_event;
	register i;

	if (vq_used(&ws->ws_q) < n) {
#ifdef WINDEVDEBUG
		printf("ws_send_batch: batch number too large\n");
#endif
		return;
	}
	for (i = 0; i < n; i++) {
		if (vq_is_empty(&ws->ws_q))
			break;
		firm_event = ws_get_event(ws);
		win_send(ws, &firm_event);
	}
}

/* return TRUE if a workstation lock is set or
 * a lock is set on the desktop.
 */
static int
ws_check_all_locks(lok_client)
	caddr_t	lok_client;
{
	register Desktop	*dtop = (Desktop *) lok_client;

	if (!dtop)
		return FALSE;

	return ((dtop->dt_ws->ws_flags & WSF_ALL_LOCKS) || 
		dtop_check_all_locks((caddr_t) dtop));
}


/*
 * Acquire the event processing lock for the event consumer window's process.
 * Kernel thread of control can't sleep, so must know that lock is
 * not on when call.  Actually, unlike other locks, callers of this routine
 * should never block and it should never be locked multiple times.
 * This lock is acquired before placing an input event on a window's clist.
 */

extern int noproc;

ws_lock_event(ws, w)
	register Workstation *ws;
	register Window *w;
{
	void	ws_unlock_event(), ws_timedout_event(), ws_force_event();
	register struct proc *process;
	extern int noproc;		/* true if no one is running */

	if (ws->ws_flags & WSF_LOCKED_EVENT) {
#ifdef WINDEVDEBUG
		printf("ws_lock_event: already locked\n");
#endif
		return;
	}
	if (w == WINDOW_NULL) {
#ifdef WINDEVDEBUG
		printf("ws_lock_event: NULL window arg\n");
#endif
		return;
	}
	/*
	 * If this process can not acquire the
	 * event lock then no event synchronization.
	 */
	if (w->w_flags & WF_UNSYNCHRONIZED)
		return;
	/* Favor the event recipient */
	ws_favor(ws, w);
	/* If event time out is not set then no event synchronization */
	if (!timerisset(&ws->ws_eventtimeout))
		return;
	/* 
	 * Get process pointer to use for lock 
	 * Note - cannot use u.u_procp if noproc is true!
	 */
	if (noproc == 0 && w->w_pid == u.u_procp->p_pid) {
		/* Case when read or select thread reacquiring lock */
		process = u.u_procp;
	} else {
		extern	struct proc *pfind();

		/* Case when kernel thread acquiring lock (more expensive) */
		if ((process = pfind(w->w_pid)) == ((struct proc *)0)) {
			/* Process has gone away */
			return;
		}
	}
	wlok_setlock(&ws->ws_eventlock, &ws->ws_flags, WSF_LOCKED_EVENT,
	    &ws->ws_eventlock.lok_count_storage, process);
	ws->ws_eventlock.lok_client = (caddr_t)w->w_desktop;
	ws->ws_eventlock.lok_unlock_action = ws_unlock_event;
	ws->ws_eventlock.lok_limit = ws->ws_eventtimeout;
	ws->ws_eventlock.lok_timeout_action = ws_timedout_event;
	ws->ws_eventlock.lok_string = "event";
	ws->ws_eventlock.lok_force_action = ws_force_event;
	ws->ws_eventlock.lok_wakeup = (caddr_t) &ws->ws_flags;
	if (!ws_verbose_event_lock)
		ws->ws_eventlock.lok_options |= WLOK_SILENT;
	ws->ws_eventlock.lok_other_check = ws_check_all_locks;
	ws->ws_event_consumer = w;
}

/*
 * Unlock the access right to the next event on the queue.
 * ws_unlock_event doesn't care if the unlock is forced or
 * congenial.
 */
void
ws_unlock_event(wlock)
	Winlock *wlock;
{
	Desktop *dtop = (Desktop *)wlock->lok_client;
	Workstation *ws = dtop->dt_ws;

	ws->ws_event_consumer = WINDOW_NULL;
}

/* No-op but needed if doing timeout stuff */
void
ws_timedout_event()
{
#ifdef WINDEVDEBUG
	if (ws_event_timeout_msg) printf("TIMEOUT EVENT\n");
#endif
}

/*
 * Make it so that the offending process doesn't acquire the event lock
 * until input queue empties or gets to read or select.
 */
void
ws_force_event(wlock)
	Winlock *wlock;
{
	Desktop *dtop = (Desktop *)wlock->lok_client;
	Workstation *ws = dtop->dt_ws;
	int win_dont_lock();

	/* Find every window in this process and mark as not lockable */
	ws_lock_control_pid = wlock->lok_pid;
	if (ws->ws_event_consumer && ws->ws_event_consumer->w_desktop)
		wt_enumeratechildren(win_dont_lock,
		    ws->ws_event_consumer->w_desktop->dt_rootwin,
		    (struct rect *)0);
	ws_lock_control_pid = 0;
}

/*
 * If window's process is currently unsyncronized, see if can 
 * allow it to acquire the event lock.
 */
ws_try_event_enable(w)
	register Window *w;
{
	int win_empty_input();

	/* Find every window in this process and see if all have empty qs */
	ws_lock_control_pid = w->w_pid;
	ws_empty_input_flag = 1;
	wt_enumeratechildren(win_empty_input, w->w_desktop->dt_rootwin,
	    (struct rect *)0);
	ws_lock_control_pid = 0;
	if (ws_empty_input_flag)
		ws_enable_event(w);
}

/*
 * Make it so that the windows in w's process can acquire the event lock.
 */
ws_enable_event(w)
	register Window *w;
{
	int win_can_lock();

	/* Find every window in this process and mark as not lockable */
	ws_lock_control_pid = w->w_pid;
	wt_enumeratechildren(win_can_lock, w->w_desktop->dt_rootwin,
	    (struct rect *)0);
	ws_lock_control_pid = 0;
}

/*
 * Called from wt_enumeratechildren so don't change calling sequence
 */
/*ARGSUSED*/
win_dont_lock(w, rect)
	struct	window *w;
	struct	rect *rect;	/* Don't use for anything else (not caddr_t) */
{

	if (!(w->w_flags & WF_OPEN) || (w->w_pid != ws_lock_control_pid))
		return (-1);
	w->w_flags |= WF_UNSYNCHRONIZED;
	return (0);
}

/*
 * Called from wt_enumeratechildren so don't change calling sequence
 */
/*ARGSUSED*/
win_empty_input(w, rect)
	struct	window *w;
	struct	rect *rect;	/* Don't use for anything else (not caddr_t) */
{

	if (!(w->w_flags & WF_OPEN) || (w->w_pid != ws_lock_control_pid))
		return (-1);
	if (w->w_desktop->dt_sharedq_owner == w) {
		win_sharedq_shift(w);
		if (w->w_desktop->shared_info->shared_eventq.head
		==  w->w_desktop->shared_info->shared_eventq.tail)
			ws_empty_input_flag = 0;
	} else
		if (w->w_input.c_cc != 0)
			ws_empty_input_flag = 0;
	return (0);
}

/*
 * Called from wt_enumeratechildren so don't change calling sequence
 */
/*ARGSUSED*/
win_can_lock(w, rect)
	struct	window *w;
	struct	rect *rect;	/* Don't use for anything else (not caddr_t) */
{

	if (!(w->w_flags & WF_OPEN) || (w->w_pid != ws_lock_control_pid))
		return (-1);
	w->w_flags &= ~WF_UNSYNCHRONIZED;
	return (0);
}

int
ws_lockio(dev)
	dev_t	dev;
{
	register struct window *w = winfromdev(dev);
	register Desktop *dtop = w->w_desktop;
	register Workstation *ws = dtop->dt_ws;
	void	ws_unlock_io();

	/* Currently, locking a dev you have locked increments a ref count */
	if ((ws->ws_flags & WSF_LOCKED_IO) == 0) {
		/* update the shared memory to show we are waiting */
		ws_set_dtop_waiting(ws, 1);

		while ((ws->ws_flags & WSF_LOCKED_IO) &&
		       ws_dtop_mutex_locked(ws))
			if (sleep((caddr_t)&ws->ws_flags, LOCKPRI|PCATCH)) {
				ws_set_dtop_waiting(ws, -1);
				return (-1);
			}

		/* update the shared memory to show we are not waiting */
		ws_set_dtop_waiting(ws, -1);

		wlok_setlock(&ws->ws_iolock, &ws->ws_flags, WSF_LOCKED_IO,
		    &ws->ws_iolock.lok_count_storage, u.u_procp);
		ws->ws_iolock.lok_client = (caddr_t)dtop;
		    /* So can get to ws, too */
		ws->ws_iolock.lok_unlock_action = ws_unlock_io;
		ws->ws_iolock.lok_timeout_action = NULL;
		ws->ws_iolock.lok_string = "io";
		ws->ws_iolock.lok_wakeup = (caddr_t) &ws->ws_flags;
		ws->ws_iolock.lok_other_check = ws_check_all_locks;

		/* update the shared memory */
		ws_set_dtop_grabio(ws, TRUE, ws->ws_iolock.lok_pid);

		dtop->dt_displaygrabber = w;
		ws->ws_inputgrabber = w;
		ws->ws_pre_grab_kbd_focus = ws->ws_kbdfocus;
		ws->ws_pre_grab_kbd_focus_next = ws->ws_kbdfocus_next;
		ws->ws_pre_grab_pick_focus = ws->ws_pickfocus;
		ws->ws_pre_grab_pick_focus_next = ws->ws_pickfocus_next;
		ws->ws_kbdfocus = w;
		ws->ws_pickfocus = w;
		ws_set_focus(ws, 0);
		/* Save extremes of std colormap, set to standard values */
		dtop_save_grnds(dtop);
		dtop_set_bkgnd(dtop);
		dtop->dt_flags |= DTF_NEWCMAP;
		/* Set cursor */
		dtop_set_cursor(dtop);
		/* Write cursor to enable plane */
		if (dtop->dt_flags & DTF_MULTI_PLANE_GROUPS &&
		    dtop->dt_plane_groups_available[PIXPG_OVERLAY_ENABLE]) {
			enable_plane_cursor_set_active(dtop, TRUE);
			/* Choose color of enable plane cursor */
			switch (w->w_plane_group) {
			case PIXPG_OVERLAY:
			case PIXPG_CURRENT:
			case PIXPG_MONO:
#ifndef PRE_FLAMINGO
			case PIXPG_24BIT_COLOR:
			case PIXPG_VIDEO:
#endif
				dtop->dt_cursor.enable_color = 1;
				break;
			default:
				dtop->dt_cursor.enable_color = 0;
			}
		}
#ifndef PRE_FLAMINGO
		/* Write cursor to video enable plane */
		if (dtop->dt_flags & DTF_MULTI_PLANE_GROUPS &&
		    dtop->dt_plane_groups_available[PIXPG_VIDEO_ENABLE])
			videnb_plane_cursor_set_active(dtop, TRUE);
#endif
	} else
		*(ws->ws_iolock.lok_count) += 1;
	return (0);
}

void
ws_unlock_io(wlock)
	Winlock *wlock;
{
	register Desktop *dtop = (Desktop *)wlock->lok_client;
	register Workstation *ws = dtop->dt_ws;

	dtop->dt_displaygrabber = NULL;
	ws->ws_inputgrabber = NULL;
	/*
	 * This is a hack to force reloading of the colormap even 
	 * though it may not be needed.  This is meant to give the 
	 * user a way that he can cleanup after a non-window
	 * or errant program that left the color map messed up.
	 * Moving a window, envoking a menu both are ways to cause
	 * WINRELEASEIO to be called.
	 */
	dtop_restore_grnds(dtop);
	dtop->dt_flags |= DTF_NEWCMAP;
	ws->ws_kbdfocus = ws->ws_pre_grab_kbd_focus;
	ws->ws_kbdfocus_next = ws->ws_pre_grab_kbd_focus_next;
	ws->ws_pickfocus = ws->ws_pre_grab_pick_focus;
	ws->ws_pickfocus_next = ws->ws_pre_grab_pick_focus_next;
	ws_set_focus(ws, FF_PICK_CHANGE);
	/*
	 * Stop writing cursor to enable plane.
	 * Take down cursor first so that enable plane is restored.
	 */
	dtop_cursordown(dtop);
	enable_plane_cursor_set_active(dtop, FALSE);
#ifndef PRE_FLAMINGO
	videnb_plane_cursor_set_active(dtop, FALSE);
#endif
	dtop_set_cursor(dtop);
	dtop_cursorup(dtop);

	/* clear the grabio lock in each dtop's 
	 * shared memory.
	 */
	ws_set_dtop_grabio(ws, FALSE, 0);

	/*
	 * The following wakeup is done because a WINLOCKSCREEN
	 * may be waiting on io lock being released.
	 */
	wakeup((caddr_t)dtop->shared_info);
}

static int
ws_dtop_mutex_locked(ws)
	Workstation	*ws;
{
	register Desktop	*dtop;

	for (dtop = ws->ws_dtop; dtop; dtop = dtop->dt_next)
		if (win_lock_mutex_locked(dtop->shared_info))
			return TRUE;

	return FALSE;
}


static void
ws_set_dtop_grabio(ws, on, pid)
	Workstation	*ws;
	int		on;
	int		pid;
{
	register Desktop	*dtop;

	for (dtop = ws->ws_dtop; dtop; dtop = dtop->dt_next) {
		win_lock_set(&dtop->shared_info->grabio, on);
		dtop->shared_info->grabio.pid = pid;
	}
}


void
ws_set_dtop_waiting(ws, num)
	Workstation	*ws;
	int		num;
{
	register Desktop	*dtop;

	for (dtop = ws->ws_dtop; dtop; dtop = dtop->dt_next) {
		dtop->shared_info->waiting += num;
	}
}
