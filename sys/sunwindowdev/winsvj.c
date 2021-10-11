#ifndef lint
static	char sccsid[] = "@(#)winsvj.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

/*
 * SunWindows routines that deal with journaling.
 * New In 4.1 starting with Alpha 9.
 * These are the configurable routines.
 *
 */

#include <sys/param.h>
#include <sys/uio.h>	/* for uio_resid */
#include <sunwindowdev/wintree.h>
#include <sunwindow/win_enum.h>
#include <sunwindow/win_ioctl.h>
#include <sys/kernel.h>	/* for time */
#include <sys/kmem_alloc.h>

Winplay_intr	svj_play_ctrl;		/* Playback interrrupt ctrl */
struct timeval	svj_rec_lasttime;	/* Last recorded event time */
u_short		svj_recplay_sync_id;    /* Sync point event id */

void svj_playrec_sync_set();

int
svjwrite(dev, uio)
	dev_t	dev;
	struct	uio *uio;
{
	struct window *w = winfromdev(dev);
	Workstation *ws;
	int	error = 0;
	int	tot_events = 0;
	int	pri;
	int	sleep_time;
	short	count;
	short	event_size = sizeof (Firm_event);
	Firm_event *eventptr;
	u_char  event[sizeof (Firm_event)];
	struct timeval	timestamp;		/* Timestamp for events */
	extern struct timeval	time;		/* Global System Time */
	extern int 	hz;			/* System Clock frequency */

	int svjwrite_timer();

	pri = spl_timeout();

	if ((w == NULL) || (w->w_desktop == NULL)) {
		error = ENXIO;
	} else {
		ws = w->w_desktop->dt_ws;

		if (!(ws->ws_flags & WSF_PLAY_BACK)) {
			error = EACCES;
		} else if (uio->uio_resid % event_size) {
			/* Not a multiple of events */
			error = EINVAL;
		} else {
			tot_events = uio->uio_resid/event_size;
			eventptr = (Firm_event *)event;
		}
	}
	timestamp = time;
	while (!error && tot_events) {
		tot_events--;
		/* Check that playback mode is on */
		if (!(ws->ws_flags & WSF_PLAY_BACK)) {
			error = EINTR;
		} else {
			for (count = 0; count < event_size; count++) {
				event[count] = uwritec(uio);
			}
			/* Calculate wait time before event release */
			sleep_time = (hz*eventptr->time.tv_sec +
			    ((hz*eventptr->time.tv_usec + hz/2) / 1000000));
			/* Check if there is a wait before release */
			if (sleep_time > 0) {
				timeout(svjwrite_timer, (caddr_t)eventptr,
					sleep_time);
				(void) sleep((caddr_t)eventptr, pri);
				timestamp = time;
			}
			/*
			 * Set the current time in event timestamp for proper
			 * input queue event sequencing.
			 */
			eventptr->time = timestamp;
			ws_consume_event(ws, eventptr);
		}
	}
	(void) splx(pri);
	return (error);
}

/* Wakeup routine for writes during playback, controlling event release */
svjwrite_timer(event)
	caddr_t	event;
{
	wakeup(event);
}

void
svjsendevent(code, ws, time_ptr)
	u_short	code;
	Workstation *ws;
	struct	timeval *time_ptr;
{
	static Firm_event fe = {0, FE_PAIR_NONE, 0, 1, {0, 0}};

	if ((code == LOC_DRAG) || (code == LOC_TRAJECTORY)) {
		fe.id = code;
		fe.time = *time_ptr;
		if (vq_put(&ws->ws_rec_q, &fe) == VUID_Q_OVERFLOW)
			ws->ws_flags &= ~WSF_RECORD_EVENT;
	}
	wakeup((caddr_t)&ws->ws_rec_q);	/* wakeup any rec event wait */
}

int
svjioctl(dev, cmd, data, dtop, ws, pri, error)
	dev_t dev;
	int cmd;
	caddr_t data;
	struct desktop *dtop;
	Workstation *ws;
	int pri;
	int error;
{
	extern int	ws_no_collapse;		/* Inp que evnt collapse flag */

	if (dtop == NULL || ws == NULL)
		return (ESPIPE);

	switch (cmd) {

	case WINNEXTFREE: {
		struct	winlink *winlink = (struct winlink *)data;

		if (winlink->wl_link == WIN_NULLLINK)
			(void) svj_playrec_sync_set(ws, WIN_SYNC_ERR,
				minor(dev), cmd);
		else
			(void) svj_playrec_sync_set(ws, WIN_SYNC_SET,
				winlink->wl_link, cmd);
		break;
		}

	case WINSETMOUSE:
		(void) svj_playrec_sync_set(ws, WIN_SYNC_SET,
			minor(*(int *)data), cmd);
		if (ws->ws_flags&WSF_RECORD_EVENT) {
			(void) svj_playrec_sync_set(ws, WIN_SYNC_WARPX,
				minor(*(int *)data), dtop->dt_rt_x);
			(void) svj_playrec_sync_set(ws, WIN_SYNC_WARPY,
				minor(*(int *)data), dtop->dt_rt_y);
		}
		break;

	case WINSETKBDFOCUS:
		(void) svj_playrec_sync_set(ws, WIN_SYNC_SET,
				minor(*(int *)data), cmd);
		break;

	/*
	 * Record/playback utilities
	 */
	case WINSETRECQUE: {
		Winrecq_setup	*setup = (Winrecq_setup *)data;
		int		ws_init_rec_q();
		void		ws_dealloc_rec_q();

		if (setup->recq_cmd == WIN_RECQ_CRE) {
			ws->ws_rec_qbytes = sizeof (Firm_event);
			if (setup->recq_size) {
				ws->ws_rec_qbytes *= setup->recq_size;
			} else {
				ws->ws_rec_qbytes *= WIN_RECQ_DEF;
			}
			if (ws_init_rec_q(ws) == -1) {
				return (win_errno);
			}
			svj_recplay_sync_id = WINSYNCIDDEF;
		} else if (setup->recq_cmd == WIN_RECQ_DEL) {
			/* Deallocate any event recording queue */
			(void) ws_dealloc_rec_q(ws);
		} else {
			return (EINVAL);
		}
		break;
		}

	case WINSETRECORD:
		if (ws->ws_flags&WSF_PLAY_BACK) {
			return (EACCES);
		}
		if (*(int *)data == WIN_SETREC_ON) {
			if (!(ws->ws_flags&WSF_RECORD_QUEUE)) {
				return (EINVAL);
			}
			if (!(ws->ws_flags&WSF_RECORD_EVENT)) {
				svj_rec_lasttime = time;
				ws->ws_flags |= WSF_RECORD_EVENT;
				ws_no_collapse = 1;
			}
		} else if (*(int *)data == WIN_SETREC_OFF) {
			ws->ws_flags &= ~WSF_RECORD_EVENT;
			ws_no_collapse = 0;
			wakeup((caddr_t)&ws->ws_rec_q);
		} else {
			return (EINVAL);
		}
		break;

	case WINREADRECQ: {
		Winrecq_readbuf	*ubuf = (Winrecq_readbuf *)data;
		Firm_event		fe;
		u_int		fe_size = sizeof (Firm_event);
		int		time_savsec;
		int		time_savusec;

		if (!(ws->ws_flags&WSF_RECORD_QUEUE)) {
			return (EACCES);
		}
		for (ubuf->total=0; ubuf->total < ubuf->trans; ubuf->total++) {
			while (vq_get(&ws->ws_rec_q, &fe) == VUID_Q_EMPTY) {
				if ((ws->ws_flags&WSF_RECORD_EVENT) ||
				    (ws->ws_flags&WSF_PLAY_BACK)) {
					(void) sleep((caddr_t)&ws->ws_rec_q,
							pri);
				} else {
					if (ubuf->total) {
						return (0);
					} else {
						return (EWOULDBLOCK);
					}
				}
			}
			/* Convert event time to delta time from last event */
			time_savsec = fe.time.tv_sec;
			time_savusec = fe.time.tv_usec;
			fe.time.tv_sec -= svj_rec_lasttime.tv_sec;
			fe.time.tv_usec -= svj_rec_lasttime.tv_usec;
			if (fe.id != svj_recplay_sync_id) {
				svj_rec_lasttime.tv_sec = time_savsec;
				svj_rec_lasttime.tv_usec = time_savusec;
			}
			if (fe.time.tv_usec < 0) {
				fe.time.tv_sec--;
				fe.time.tv_usec += 1000000;
			}
			if (copyout((char *)&fe, (char *)&ubuf->fe[ubuf->total],
					fe_size))
				return (EFAULT);
		}
		break;
		}

	case WINSETPLAYBACK:
		if (ws->ws_flags&WSF_RECORD_EVENT) {
			return (EACCES);
		}
		if (*(int *)data == WIN_SETPLAY_ON) {
			ws->ws_flags |= WSF_PLAY_BACK;
			svj_rec_lasttime = time;
			ws_no_collapse = 1;
			svj_play_ctrl.intr_flags = WIN_PLAY_INTR_DEF;
		} else if (*(int *)data == WIN_SETPLAY_OFF) {
			ws->ws_flags &= ~WSF_PLAY_BACK;
			ws_no_collapse = 0;
			wakeup((caddr_t)&ws->ws_rec_q);
		} else {
			return (EINVAL);
		}
		break;

	case WINSETPLAYINTR: {
		Winplay_intr	*intr = (Winplay_intr *)data;

		if (ws->ws_flags&WSF_PLAY_BACK) {
			svj_play_ctrl.intr_flags = intr->intr_flags;
			svj_play_ctrl.intr_feid  = intr->intr_feid;
		} else {
			return (EACCES);
		}
		break;
		}

	case WINSETSYNCPT: {
		Winrecplay_syncbuf	*sync = (Winrecplay_syncbuf *)data;

		/* Check if sync point set flag doesn't match journal state */
		if ((!(sync->sync_flag&WINSYNCREC &&
		    ws->ws_flags&WSF_RECORD_EVENT)) &&
		    (!(sync->sync_flag&WINSYNCPLAY &&
		    ws->ws_flags&WSF_PLAY_BACK)))
			return (0);

		switch (sync->sync_cmd) {
		case WINSYNCDEFAULT:
			(void) svj_playrec_sync_set(ws, WIN_SYNC_SET,
				(minor(dev)), WINSETSYNCPT);
			break;
		case WINSYNCUSRSTD:
			(void) svj_playrec_sync_set(ws, sync->sync_type,
				(minor(dev)), WINSETSYNCPT);
			break;
		case WINSYNCUSRDFN:
			(void) svj_playrec_sync_set(ws, sync->sync_type,
				sync->sync_pair, sync->sync_value);
			break;
		case WINSETSYNCID:
			svj_recplay_sync_id = sync->sync_value;
			break;
		default:
			return (EINVAL);
		}
		break;
		}

	case WININSERT:
	case WINREMOVE:
	case WINGRABIO:
	case WINRELEASEIO:
		if (!error)
			(void) svj_playrec_sync_set(ws, WIN_SYNC_SET,
				minor(dev), cmd);
		else
			(void) svj_playrec_sync_set(ws, WIN_SYNC_ERR,
				minor(dev), cmd);
		break;

	default:
		return (EINVAL);
	}

	return (0);
}

void
svj_playrec_sync_set(ws, sync_type, sync_pair, sync_value)
	register Workstation *ws;
	register int sync_type;
	register int sync_pair;
	register int sync_value;
{
	Firm_event	sync_event;


	if ((ws->ws_flags&WSF_RECORD_EVENT) || (ws->ws_flags&WSF_PLAY_BACK)) {
		sync_event.time = time;
		sync_event.id = svj_recplay_sync_id;
		sync_event.pair_type = (u_char) sync_type;
		sync_event.pair = (u_char) sync_pair;
		sync_event.value = sync_value;
		if (vq_put(&ws->ws_rec_q, &sync_event) == VUID_Q_OVERFLOW) {
			if (ws->ws_flags&WSF_RECORD_EVENT) {
				ws->ws_flags &= ~WSF_RECORD_EVENT;
			} else {
				ws->ws_flags &= ~WSF_PLAY_BACK;
			}
			printf("Window record queue unexpectedly full!\n");
		}
		if (ws->ws_flags&WSF_PLAY_BACK)
			wakeup((caddr_t)&ws->ws_rec_q);
	}
	return;
}

svj_consume_event(ws, event)
	register Workstation *ws;
	register Firm_event *event;
{
	register Desktop *dtop_loc = ws->ws_loc_dtop;
	extern struct timeval	time;		/* Global System Time */

	/* Serialize event sequencing for journaling */
	event->time = time;
	/* Ignore Loc Stills, as they are synthesized */
	if (event->id != LOC_STILL) {
		/*
		 * If a mouse movement was made, convert delta
		 * to absolute. This is to assure proper mouse
		 * location on playback, as mouse will not be
		 * at exact location it was on rec start.
		 */
		if (event->id == LOC_X_DELTA) {
			event->id = LOC_X_ABSOLUTE;
			event->pair_type = FE_PAIR_SET;
			event->value = dtop_loc->dt_rt_x;
		} else if (event->id == LOC_Y_DELTA) {
			event->id = LOC_Y_ABSOLUTE;
			event->pair_type = FE_PAIR_SET;
			event->value = dtop_loc->dt_rt_y;
		}
		/* Enqueue event on record queue. */
		if (vq_put(&ws->ws_rec_q, event) == VUID_Q_OVERFLOW) {
			ws->ws_flags &= ~WSF_RECORD_EVENT;
			printf("Window record queue full!\n");
		}
	}
}

/*
 * Check if in playback mode and input stops playback.  If in playback,
 * flush the event and do not consume it, regardless of its effect on
 * playback.
 */
int
svj_consume_input_event(ws, event)
	register Workstation *ws;
	register Firm_event *event;
{
	short  intr_flag = 0;

#define	ESC	27			/* Keyboard <ESC> */

	/* Check if interrupt on default events activated */
	if (svj_play_ctrl.intr_flags & WIN_PLAY_INTR_DEF) {
		switch (event->id) {
		case MS_LEFT:
		case MS_MIDDLE:
		case MS_RIGHT:
		case ESC:
			if (event->value == 0)	/* Up transition */
				intr_flag = 1;
			break;
		}
	}
	/* Check if interrupt on user defined event activated */
	if (svj_play_ctrl.intr_flags & WIN_PLAY_INTR_USER) {
		if ((event->id == svj_play_ctrl.intr_feid) &&
		    (event->value == 0))
			intr_flag = 1;
	}
	if (intr_flag) {
		if (svj_play_ctrl.intr_flags & WIN_PLAY_INTR_SYNC)
			(void) svj_playrec_sync_set(ws, WIN_SYNC_INTR,
				0, WINSETSYNCPT);
		else
			ws->ws_flags &= ~WSF_PLAY_BACK;
		wakeup((caddr_t)&ws->ws_rec_q);	/* wake any read wait */
	}
	return (0);
}

/*
 * This routine initalizes the queue used for storing events during
 * capture recording.
 */
int
ws_init_rec_q(ws)
	register Workstation *ws;
{
	extern u_int   ws_vq_node_bytes;	/* Default event queue size */

	if (ws->ws_flags&WSF_RECORD_QUEUE) {
		win_errno = EACCES;
		return (-1);
	}
	/* Allocate the event recording q */
	if (!ws->ws_rec_qbytes)
		ws->ws_rec_qbytes = ws_vq_node_bytes;
	ws->ws_rec_qdata = new_kmem_zalloc(
			    (u_int)ws->ws_rec_qbytes, KMEM_SLEEP);
	if (ws->ws_rec_qdata == NULL) {
		printf("Couldn't allocate %D byte record event buffer bytes\n",
		    ws->ws_rec_qbytes);
		win_errno = ENOMEM;
		return (-1);
	}
	vq_initialize(&ws->ws_rec_q, ws->ws_rec_qdata, ws->ws_rec_qbytes);
	ws->ws_flags |= WSF_RECORD_QUEUE;
	return (0);
}

/* This routine is used to deallocate the recording queue */
void
ws_dealloc_rec_q(ws)
	register Workstation *ws;
{
	if ((ws->ws_rec_qdata != NULL) && (ws->ws_flags&WSF_RECORD_QUEUE)) {
		ws->ws_flags &= ~WSF_RECORD_QUEUE;
		kmem_free(ws->ws_rec_qdata, (u_int)ws->ws_rec_qbytes);
	}
	return;
}
