#ifndef lint
static	char sccsid[] = "@(#)ws_interrupt.c 1.1 92/07/30";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * SunWindows Workstation code concerned with the bottom end of the
 * input queue: polling input devices, enqueuing events, tracking the
 * cursor.  It is also responsible for driving deadlock detection
 * from the interrupt loop.
 */
#include <sunwindowdev/wintree.h>
#include <sunwindow/win_input.h>
#include <sunwindow/win_ioctl.h>
#include <sys/errno.h>
#include <sys/file.h>
#include <sys/kernel.h>	/* for time */

/* TODO: Place Ws_usr_async ws_flush_default in Workstation */
/* TODO: Turn on ws_sync_msg and debug out of sync problem */

#ifdef WINDEVDEBUG
int	ws_kreads;	/* Temp: Monitor number of kernel reads */
int	ws_q_compress;	/* Temp: Monitor number of compresses */
int	ws_qput_debug;	/* Temp: Debugging messages when enqueue firm event */
int	ws_compress_debug;/* Temp: Turn on to monitor collapse statistics */
int	ws_sync_msg;	/* Temp: Message about locator ut & rt out of sync */

int	ws_break_msg;		/* Message when event lock break */
int	ws_stop_msg;		/* Message when stop sent */
int	ws_flush_msg;		/* Message when user specified flush done */
#endif
int	ws_sync_debug = 1;/* Temp: Try to catch locator ut & rt out of sync */
int	ws_no_q_compress;	/* Non-zero mean don't do q compression */

int	ws_poll_rate;		/* # of ticks until next input poll */
static	int ws_quiet_ticks;	/* # of ticks since latest input activity */

extern	int	ws_fast_timeout;	/* Fast polling rate in hz */
extern	int	ws_slow_timeout;	/* Slow polling rate in hz */
extern	int	ws_fast_poll_duration;	/* Stop fast polling after this # hz */
extern	int	ws_loc_still;		/* Locator is still after this # of hz*/

int		ws_button_order;	/*  current button mapping  */
static int	buttoncodes[6][3] = {	/*  translate table for buttons	*/
    { BUT(1), BUT(2), BUT(3) },		/*  L M R  */
    { BUT(1), BUT(3), BUT(2) },		/*  L R M  */
    { BUT(2), BUT(1), BUT(3) },		/*  M L R  */
    { BUT(2), BUT(3), BUT(1) },		/*  M R L  */
    { BUT(3), BUT(1), BUT(2) },		/*  R L M  */
    { BUT(3), BUT(2), BUT(1) },		/*  R M L  */
};

extern Ws_scale	ws_scaling[WS_SCALE_MAX_COUNT+1];

#define	WS_QUEUE_COMPRESS_FACTOR 4	/* Try to collapse q to 1/4th */
#define	WS_QUEUE_COMPRESS_MORE	 1	/* ws_q_compress_factor increment */
int	ws_q_compress_factor = WS_QUEUE_COMPRESS_FACTOR;
int	ws_q_compress_base = WS_QUEUE_COMPRESS_FACTOR;
int	ws_q_compress_more = WS_QUEUE_COMPRESS_MORE;

ws_interrupt()
{
	register Workstation *ws;
	register Desktop *dtop;
	register Wsindev *in_dev;
	register int i;
	int	num_ws = 0;

	ws_quiet_ticks += ws_poll_rate;
	/* Cycle through every workstation */
	for (i = 0; i < nworkstations; i++) {
		ws = &workstations[i];
		if ((~ws->ws_flags & WSF_PRESENT) ||
		    (ws->ws_flags & WSF_EXITING))
			continue;
		num_ws++;
		/* Cycle through every input device */
		for (in_dev = ws->ws_indev; in_dev; in_dev = in_dev->wsid_next){
			if (ws_read_indev(ws, in_dev))
				/* Some input activity */
				ws_quiet_ticks = 0;
		}
		/*
		 * Update how long since locator has been still (may change
		 * in ws_track_locator)
		 */
		ws->ws_loc_stillticks += ws_poll_rate;
		if (ws->ws_flags & WSF_LOC_UPDATED)
			/* Do locator tracking */
			ws_track_locator(ws);
		/* Send LOC_STILL */
		if ((ws->ws_loc_stillticks >= ws_loc_still) &&
		    (ws->ws_loc_stillticks <
		      ws_loc_still + ws_fast_poll_duration)) {
			ws_loc_is_still(ws);
			/* Make sure don't send multiple stills */
			ws->ws_loc_stillticks = ws_loc_still +
			    ws_fast_poll_duration;
		}
		/* If no event lock holder then nudge next event off queue */
		if (~ws->ws_flags & WSF_LOCKED_EVENT)
			ws_nudge(ws);
		/* Do grabio running check */
		if (ws->ws_flags & WSF_LOCKED_IO)
			wlok_running_check(&ws->ws_iolock);
		/* Do current event timeout deadlock resolution */
		if (ws->ws_flags & WSF_LOCKED_EVENT)
			wlok_lockcheck(&ws->ws_eventlock, ws_poll_rate,
			    1 /* charge sleep time to elasped time */);
		/* Cycle through every desktop */
		for (dtop = ws->ws_dtop; dtop; dtop = dtop->dt_next)
			/* Do display/data lock timeout resolution */
			dtop_interrupt(dtop, ws_poll_rate);
	}
	/* Adjust polling rate */
	if (ws_quiet_ticks >= ws_fast_poll_duration)
		/*
		 * Slow input polling down when no activity
		 * in order to save processor cycles.
		 */
		ws_poll_rate = ws_slow_timeout;
	else
		ws_poll_rate = ws_fast_timeout;
	if (num_ws == 0) {
		/* Turn self off if no more active workstations */
		untimeout(ws_interrupt, (caddr_t)0);
		/* Resetting ws_poll_rate means ws_open should restart */
		ws_poll_rate = 0;
	} else
		/* Come around again */
		timeout(ws_interrupt, (caddr_t)0, ws_poll_rate);
}

int
ws_read_indev(ws, in_dev)
	register Workstation *ws;
	register Wsindev *in_dev;
{
#define	EVENT_BUF_LEN	20
	Firm_event events[EVENT_BUF_LEN];
	Firm_event afe;
	caddr_t aevents;
	int len;
	register int loop = 1;
	register int e;
	int read_one = 0;
	int error;

	/* Drain input device of all its input in loop */
	while (loop) {
		len = sizeof(Firm_event) * EVENT_BUF_LEN;
#ifdef WINDEVDEBUG
ws_kreads++;
#endif
		error = kern_read(in_dev->wsid_vp, (caddr_t)&events[0], &len);
		switch (error) {
		case 0:
			/* Turn ascii into Firm_events */
			if (in_dev->wsid_flags & WSID_TREAT_AS_ASCII) {
				aevents = (caddr_t)&events[0];
				for (e = 0; e < len; e++) {
					afe.id = *(aevents+e);
					afe.pair_type = FE_PAIR_NONE;
					afe.pair = 0;
					afe.value = 1;
					afe.time = time;
					read_one =
					  ws_consume_input_event(ws, &afe);
				}
				continue;
			}
			/* Break out of loop if read less then max */
			if (len < sizeof(Firm_event) * EVENT_BUF_LEN)
				loop = 0;
			/* Enqueue each event with workstation */
			for (e = 0; len >= sizeof(Firm_event);
			    e++, len -= sizeof(Firm_event)) {
				read_one =
				  ws_consume_input_event(ws, &events[e]);
			}
#ifdef WINDEVDEBUG
			/* Check for incomplete firm event read */
			if (len != 0)
				printf("ws_read_indev length error %D\n", len);
#endif
			break;
		case EWOULDBLOCK:
			/* No more input from the device */
			loop = 0;
			continue;
#ifdef WINDEVDEBUG
		default:
			printf("ws_read_indev error %D\n", error);
#endif
		}
	}
	return (read_one);
}

/*
 * Update cursor if locator moved and jump between desktops.
 */
ws_track_locator(ws)
	register Workstation *ws;
{
	Desktop *dtop_loc = ws->ws_loc_dtop; /* can't be register */
	int x_now, y_now; /* can't be register */
	register Desktop *dtop_old;

	ws->ws_flags &= ~WSF_LOC_UPDATED;
	if (dtop_loc == DESKTOP_NULL)
		return;
	/* Get locator position on desktop */
	x_now = dtop_loc->dt_rt_x;
	y_now = dtop_loc->dt_rt_y;
	/* Restart timer to see if still */
	ws->ws_loc_stillticks = 0;
	/* Clamp locator to desktop or move onto adjoinning desktop */
	dtop_old = dtop_loc;
	dtop_track_locator(&dtop_loc, &x_now, &y_now);
	/* Clear old dtop cursor and cursor owner */
	if (dtop_loc != dtop_old) {
		void dtop_update_enable();

		dtop_cursordown(dtop_old);
		win_shared_update_cursor_active(dtop_old->shared_info, FALSE);
		ws->ws_loc_dtop = dtop_loc;
		win_shared_update_cursor_active(dtop_loc->shared_info, TRUE);
		dtop_old->dt_cursorwin = WINDOW_NULL;
		win_shared_update_cursor(dtop_old);
		dtop_update_enable(dtop_loc, dtop_old, 0);
		/* Reload colormap as enter new dtop */
		dtop_loc->dt_flags |= DTF_NEWCMAP;
	}
	/*
	 * Update the shared memory mouse x, y
	 */
	if (x_now != dtop_loc->dt_rt_x || y_now != dtop_loc->dt_rt_y)
	    win_shared_update_mouse_xy(dtop_loc->shared_info, x_now, y_now);
	/* Update dtop_loc's notion of locator position */
	dtop_loc->dt_rt_x = x_now;
	dtop_loc->dt_rt_y = y_now;
	/*
	 * Find out new cursor owner, arrange for new cursor, update
	 * the shared cursor if window changed.
	 */
	dtop_set_cursor(dtop_loc);
}

ws_consume_event(ws, event)
	register Workstation *ws;
	register Firm_event *event;
{
	register Desktop *dtop_loc = ws->ws_loc_dtop;
	int num;
	int orig_x, orig_y;

	/* Deal with full input queue */
	while ((vq_is_full(&ws->ws_q)) && (!ws_no_q_compress)) {
		num = vq_compress(&ws->ws_q, ws_q_compress_factor);
#ifdef WINDEVDEBUG
ws_q_compress++;
if (ws_compress_debug)
    printf("Win input q collapsed %D, factor %D\n", num, ws_q_compress_factor);
#endif
		if (num == 0) {
			/* Handle compression failure */
			if (ws_q_compress_factor != 0)
				/* Zero factor means super compress */
				ws_q_compress_factor = 0;
			else {
				extern	void	ws_handle_overflow();

				ws_handle_overflow(ws);
			}
		} else
			/* Up compress factor for next time (reset on empty q)*/
			ws_q_compress_factor += ws_q_compress_more;
	}
	/* Reset compression factor if q empties */
	if (vq_is_empty(&ws->ws_q)) {
		ws_q_compress_factor = ws_q_compress_base;
#ifdef WINDEVDEBUG
if (dtop_loc == DESKTOP_NULL) printf("Unexpected dtop_loc == NULL!\n");
#endif
if (ws_sync_debug &&
    (ws->ws_pick_dtop == dtop_loc) &&
    ((dtop_loc->dt_rt_x != dtop_loc->dt_ut_x) ||
    (dtop_loc->dt_rt_y != dtop_loc->dt_ut_y))) {
#ifdef WINDEVDEBUG
    if (ws_sync_msg)
	printf("Ut (%D,%D) & rt(%D,%D) out of sync: now in sync\n",
    	dtop_loc->dt_ut_x, dtop_loc->dt_ut_y,
    	dtop_loc->dt_rt_x, dtop_loc->dt_rt_y);
#endif
    dtop_loc->dt_ut_x = dtop_loc->dt_rt_x;
    dtop_loc->dt_ut_y = dtop_loc->dt_rt_y;
}
	}
	/* Update state of virtual user input device */
	orig_x = dtop_loc->dt_rt_x;
	orig_y = dtop_loc->dt_rt_y;
	switch (event->id) {
	case LOC_X_DELTA:
		/* Modify locator delta events for acceleration */
		if (ws->ws_loc_stillticks < ws_loc_still)
			ws_scale_event(&event->value);
		dtop_loc->dt_rt_x += event->value;
		ws->ws_flags |= WSF_LOC_UPDATED;
		break;

	case LOC_Y_DELTA:
		/* Modify locator delta events for acceleration */
		if (ws->ws_loc_stillticks < ws_loc_still)
			ws_scale_event(&event->value);
		/* Adjust y direction from locator */
		event->value = 0 - event->value;
		dtop_loc->dt_rt_y += event->value;
		ws->ws_flags |= WSF_LOC_UPDATED;
		break;

	case LOC_X_ABSOLUTE:
		dtop_loc->dt_rt_x = event->value;
		ws->ws_flags |= WSF_LOC_UPDATED;
		break;

	case LOC_Y_ABSOLUTE:
		dtop_loc->dt_rt_y = event->value;
		ws->ws_flags |= WSF_LOC_UPDATED;
		break;

	case LOC_MOVE:
		ws->ws_flags |= WSF_LOC_UPDATED;
		break;

	case MS_LEFT:
	case MS_MIDDLE:
	case MS_RIGHT:
		if (ws_button_order > 0)
		    event->id =
			buttoncodes[ws_button_order][event->id - MS_LEFT];
		/* FALL THROUGH to do all the normal processing */

	default: {
		extern Ws_usr_async ws_flush_default;
		register int async_triggered = 0;

		/* Set real time state */
		vuid_set_value(&ws->ws_rtstate, event);
		/*
		 * Check for escapes.  Can have single escape trigger
		 * multiple actions.
		 */
		if (ws_usr_async_check(ws, &ws_flush_default, event))
			async_triggered++;
		if (ws_usr_async_check(ws, &ws->ws_break, event))
			async_triggered++;
		if (ws_usr_async_check(ws, &ws->ws_stop, event))
			async_triggered++;
		if (async_triggered)
			/*
			 * Toss trigger event (leaving first event in queue)
			 * This has the unpleasant side affect of messing up
			 * the maintainence of the user time vuid state.
			 * TODO: Perhaps we should always flush the input queue
			 * which syncs the real time and user time input states?
			 */
			return;
		}
	}
	if (orig_x != dtop_loc->dt_rt_x || orig_y != dtop_loc->dt_rt_y)
	    /*
	     * Update the shared memory mouse x, y
	     */
	    win_shared_update_mouse_xy(dtop_loc->shared_info,
		dtop_loc->dt_rt_x, dtop_loc->dt_rt_y);

#ifdef WINSVJ
	if (ws->ws_flags&WSF_RECORD_EVENT)
		(void) svj_consume_event(ws, event);
#endif

	/* Enqueue event on input queue */
	if (vq_put(&ws->ws_q, event) == VUID_Q_OVERFLOW)
#ifdef WINDEVDEBUG
		printf("Window input queue unexpectedly full!\n");
#else
		;
#endif

#ifdef WINDEVDEBUG
if (ws_qput_debug) printf("q put id %D value %D avail %D sec %D usec %D\n",
    event->id, event->value, vq_avail(&ws->ws_q),
    event->time.tv_sec, event->time.tv_usec);
#endif
}

int
ws_usr_async_check(ws, wua, fe)
	Workstation *ws;
	register Ws_usr_async *wua;
	register Firm_event *fe;
{
	if (wua->flags & WUA_1ST_HAPPENED) {
		wua->flags &= ~WUA_1ST_HAPPENED;
		/* This is second event; see if matches */
		if ((fe->id == wua->second_id) &&
		    (fe->value == wua->second_value)) {
			wua->action(ws);
			return (-1);
		}
	} else {
		/* This is first event; see if matches */
		if ((fe->id == wua->first_id) &&
		    (fe->value == wua->first_value)) {
			if (wua->flags & WUA_IGNORE_2ND) {
				wua->action(ws);
				return (-1);
			} else
				wua->flags |= WUA_1ST_HAPPENED;
		}
	}
	return (0);
}

ws_loc_is_still(ws)
	Workstation *ws;
{
	Firm_event fe;

	fe.id = LOC_STILL;
	fe.pair_type = FE_PAIR_NONE;
	fe.pair = 0;
	fe.value = 1;
	fe.time = time;
	ws_consume_event(ws, &fe);
}

void
ws_break_handler(ws)
	Workstation *ws;
{
	if (ws->ws_flags & WSF_LOCKED_EVENT) {
		wlok_forceunlock(&ws->ws_eventlock);
#ifdef WINDEVDEBUG
		if (ws_break_msg)
			printf("Event lock broken by user\n");
#endif
	}
}

void
ws_flush_handler(ws)
	Workstation *ws;
{
	ws_flush_input(ws);
#ifdef WINDEVDEBUG
	if (ws_flush_msg)
		printf("Window input queue flushed by user!\n");
#endif
}

void
ws_stop_handler(ws)
	register Workstation *ws;
{
	Firm_event fe;
	Window *w;

	fe.id = WIN_STOP;
	fe.pair_type = FE_PAIR_NONE;
	fe.pair = 0;
	fe.value = 1;
	fe.time = time;
	if ((ws->ws_loc_dtop != DESKTOP_NULL) &&
	    ((w = ws->ws_loc_dtop->dt_cursorwin) != WINDOW_NULL)) {
		/* Break event lock so that this gets through right away */
		wlok_forceunlock(&ws->ws_eventlock);
		/* Send stop event to real time pick window right now */
		(void) win_send_one(w, &fe, &w->w_pickmask, INPUTMASK_NULL);
		/* Send a urgent io signal */
		winsignal(w, SIGURG);
#ifdef WINDEVDEBUG
		if (ws_stop_msg)
			printf("Stop sent\n");
#endif
	}
}


/*
 * ws_scale_event returns the scale factor of a locator motion.
 *	It assumes the locator sample shows motion (is a delta, not an
 *	absolute, coordinate), whose value fits in 16 bits.  (The current
 *	mouse protocol implies the value is in the range +/- 127.)
 *
 *	The old decision not to scale on the first motion after some
 *	time with the locator still is applied before ws_scale_event
 *	is called; however, it's now possible to leave an identity scaling
 *	for small values of motion in any case.
 *
 *	It also scales X and Y independently, which may be considered an
 *	error.  The alternative is to return a scale factor, given a
 *	dx and dy (or the sum of their squares, to do it right).  But
 *	that assumes dx and dy are being handled together, which isn't
 *	currently the case.  Getting that would require beefing up the
 *	vuid queue & state. 
 */
int
ws_scale_event(sample)
    register int   *sample;
{
    register short  value;
    register Ws_scale *scale;

    value = *sample;
    if (value < 0)
	value = -value;
    scale = ws_scaling;
    while (value > scale->ceiling
    )
	scale++;
    *sample *= scale->factor;
}

/*
 * Check if in playback mode.  If in playback, consume by svj routine.
 */
int
ws_consume_input_event(ws, event)
	register Workstation *ws;
	register Firm_event *event;
{

#ifdef WINSVJ
	if (ws->ws_flags & WSF_PLAY_BACK) 	/* In playback mode */
		return(svj_consume_input_event(ws, event));
#endif
	ws_consume_event(ws, event);		/* Normal processing */
	return(1);
}
