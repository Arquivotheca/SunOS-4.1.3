#ifndef lint
static	char sccsid[] = "@(#)win.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * SunWindows routines that deal with the UNIX device nature of the driver,
 * i.e., open, close, read, select.  ioctls handled in winioctl.c.
 */

#include <sys/param.h>
#include <sys/mman.h>
#include <sys/kmem_alloc.h>
#include <vm/hat.h>
#include <vm/as.h>
#include <sunwindowdev/wintree.h>
#include <sunwindow/win_ioctl.h>
#include <pixrect/pr_util.h>	/* for mpr_static support */
#include <pixrect/pr_dblbuf.h>	/* for double buffering support */
#include <sys/uio.h>		/* for uio_resid */
#include <sys/file.h>		/* for FREAD and FWRITE */
#include <machine/pte.h>
#include <machine/mmu.h>
#include <machine/cpu.h>
#include <sunwindow/pw_dblbuf.h> /* For double buffering */

/* TODO: Be able to mask non-standard vuid events. */

int	win_disable_shared_locking = 0;/* turn off shared locking if set to 1*/

int	win_send_one_debug;	/* Temp: Debugging message for win_send_one */
int	win_send_one_other_debug; /* Temp: Debug other_mask in win_send_one */

int	wintossmsg;	/* Message with toss input for lack of clist space */
int	winpsnotkill;	/* Reach around communication from */
			/* WINSCREENDESTROY to winkill */
int	win_infinite_redirect;	/* See how many events tossed due */
				/* to infinite redirection loop. */

extern	void dtop_choose_dblbuf();
extern	void dtop_changedisplay();

/*
 * Number of windows open.
 */
static	int	winsopen;
static	int	winclosebusy;
/*
 * Max number of characters that window system can have in all its clists.
 */
extern	int	winclistcharsmax;
/*
 * Current number of characters that window system has in all its clists.
 */
int	winclistchars;

short	wincursordefaultimage[16] = {
	0x0000, 0x7fe0, 0x7f80, 0x7e00,
	0x7e00, 0x7f00, 0x7f80, 0x67c0,
	0x63e0, 0x41f0, 0x40f8, 0x007c,
	0x003e, 0x001f, 0x000e, 0x0004
};
mpr_static(wincursordefaultmpr, 16, 16, 1, wincursordefaultimage);
struct	cursor wincursordefault =
	{ 0, 0, PIX_SRC|PIX_DST, &wincursordefaultmpr};

int	winkill();

struct window *
winfromdev(dev)
	dev_t	dev;
{
	register struct window *w;
	register int wnum = minor(dev);

	if (wnum >= win_nwindows) {
		return (0);
	}
	w = winbufs[wnum];
	if (!w) {
		w = (struct window *)
			new_kmem_zalloc(sizeof (struct window), KMEM_SLEEP);
		winbufs[wnum] = w;
	}
	return (w);
}

struct window *
winfromopendev(dev)
	dev_t	dev;
{

	register int	wnum = minor(dev);

	if (wnum >= win_nwindows) {
		return (0);
	}
	return (winbufs[wnum]);
}

/*
 * All windows can be opened more than once.
 * If flag&WIN_EXCLOPEN then this must be the first time being opened.
 */
int
winopen(dev, flag)
	dev_t	dev;
{
	register struct window *w = winfromdev(dev);

	if (!w)
		return (ENXIO);
	if ((flag&WIN_EXCLOPEN) && (w->w_flags&WF_OPEN))
		return (EACCES);
	if (w->w_flags&WF_OPEN)
		return (0);
	/*
	 * Initialize window data structure
	 */
	w->w_flags = WF_OPEN;
	w->w_cursor = wincursordefault;
	w->w_cursormpr = wincursordefaultmpr;
	w->w_cursordata = *((struct mpr_data *)(wincursordefaultmpr.pr_data));
	bcopy((caddr_t)wincursordefaultimage, (caddr_t)w->w_cursorimage,
	    CUR_MAXIMAGEBYTES);
	winfixupcursor(w);
	w->w_pid = u.u_procp->p_pid;
	winsopen++;
	w->w_dbl_rdstate = PW_DBL_BACK;
	w->w_dbl_wrstate = PW_DBL_BOTH;
	w->w_wid_dbl_info.dbl_wid.wa_count = 0;
	w->w_wid_dbl_info.dbl_wid.wa_type = -1;
	w->w_wid_dbl_info.dbl_wid.wa_index = -1;
	return (0);
}

/*
 * Force lock closed if owned by w or we are closing a root window
 * and there is a reference to the roots desktop in the lock.
 * We are try to avoid dangling references in the lock.
*/
void
wincloselock(wlock, dtop, w)
	Winlock *wlock;
	Desktop *dtop;
	Window *w;
{
	if (wlock->lok_pid == w->w_pid ||
	    (wlock->lok_client == (caddr_t)dtop && w->w_flags & WF_ROOTWINDOW))
		wlok_forceunlock(wlock);
}

/*ARGSUSED*/
int
winclose(dev, flag)
	dev_t	dev;
{
	register struct window *w;
	register struct desktop *dtop;
	Workstation *ws;
	register struct window *wchild;
	int set_focus = 0;
	int pri;

	/*
	 * Protect access to close routine.
	 */
	while (winclosebusy)
		(void) sleep((caddr_t)&winclosebusy, PZERO-1);
	/*
	 * Can't have interrupts going off during closing.
	 */
	pri = spl_timeout();
	winclosebusy = 1;
	w = winfromdev(dev);
	dtop = w->w_desktop;
	ws = w->w_ws;
	if (dtop && !(dtop->dt_flags & DTF_PRESENT))
		dtop = DESKTOP_NULL;
	if (ws && !(ws->ws_flags & WSF_PRESENT))
		ws = WORKSTATION_NULL;
	winsopen--;
	if (ws) {
		/* Release any locks held */
		wincloselock(&ws->ws_eventlock, dtop, w);
		if (ws->ws_pre_grab_kbd_focus == w)
			ws->ws_pre_grab_kbd_focus = WINDOW_NULL;
		if (ws->ws_pre_grab_kbd_focus_next == w)
			ws->ws_pre_grab_kbd_focus_next = WINDOW_NULL;
		if (ws->ws_pre_grab_pick_focus == w)
			ws->ws_pre_grab_pick_focus = WINDOW_NULL;
		if (ws->ws_pre_grab_pick_focus_next == w)
			ws->ws_pre_grab_pick_focus_next = WINDOW_NULL;
		wincloselock(&ws->ws_iolock, dtop, w);
		/* Reset any workstation references to w */
		if (ws->ws_kbdfocus == w) {
			ws->ws_kbdfocus = WINDOW_NULL;
			set_focus = 1;
		}
		if (ws->ws_pickfocus == w) {
			ws->ws_pickfocus = WINDOW_NULL;
			set_focus = 1;
		}
		if (ws->ws_pickfocus_next == w)
			ws->ws_pickfocus_next = WINDOW_NULL;
		if (ws->ws_kbdfocus_next == w)
			ws->ws_kbdfocus_next = WINDOW_NULL;
		if (ws->ws_favor_pid == w->w_pid)
			ws_favor(ws, WINDOW_NULL);
	}
	if (dtop) {
		/*
		 * Release any locks held
		 */
		wincloselock(&dtop->dt_datalock, dtop, w);
		dtop_validate_shared_lock(dtop, &dtop->dt_mutexlock);
		wincloselock(&dtop->dt_mutexlock, dtop, w);
		dtop_validate_shared_lock(dtop, &dtop->dt_displaylock);
		wincloselock(&dtop->dt_displaylock, dtop, w);

		/*
		 * Release shared input queue, if owner
		 */
		if (dtop->dt_sharedq_owner == w) {
			dtop->dt_sharedq_owner = 0;
			dtop->shared_info->shared_eventq.head = 0;
			dtop->shared_info->shared_eventq.tail = 0;
		}

		/*
		* if the window was the last window to have the
		* double buffer access reset the go_to_kernel and
		* set write to both.  If the count is greater than
		* zero and the window was the current bufferer then
		* choose another double bufferer.
		*/
		if (w->w_flags & WF_DBLBUF_ACCESS)
		{
			if (--dtop->dt_dblcount)
			{
				if (dtop->dt_curdbl == w)
					dtop_choose_dblbuf(dtop);
			} else {
				dtop_changedisplay(dtop, dtop->dt_dbl_bkgnd,
					dtop->dt_dbl_frgnd, FALSE);
				dtop->shared_info->go_to_kernel = 0;
                                if ( pr_dbl_set(dtop->dt_pixrect,
						PR_DBL_READ,dtop->dt_dbl_bkgnd,
						PR_DBL_WRITE, PR_DBL_BOTH, 0))
#ifdef WINDEVDEBUG
					printf("Error dbl_set in winclose\n");
#else
					;
#endif
			}
		}

		/*
		 * Remove from tree and fixup screen
		 */
		if (w->w_flags & WF_INSTALLED)
			(void) dtop_removewin(dtop, w);
		/*
		 * Reset any desktop references to w
		 */
		if (dtop->dt_rootwin==w)
			dtop->dt_rootwin = NULL;
		if (dtop->dt_cursorwin == w)
			dtop_set_cursor(dtop);
		/*
		 * Free any resources of w that are sharable with other
		 * windows on dtop (if not currently being shared).
		 */
		dtop_cmsfree(dtop, w);
	}
	/* Set new input focus, after removed from tree */
	if (set_focus)
		ws_set_focus(ws, FF_PICK_CHANGE);
	/*
	 * Free resources allocated to window
	 */
	rl_free(&w->w_rlexposed);
	rl_free(&w->w_rlexposedold);
	rl_free(&w->w_rlfixup);
	/*
	 * Flush input buffer
	 * Note: Should redistribute this to surviving windows.
	 * Note: if eventq was shared, c_cc should be zero.
	 */
	while (w->w_input.c_cc) {
		(void) getc(&w->w_input);
		winclistchars--;
		continue;
	}
	/*
	 * Remove exiting children's references to w.
	 * Kill controlling processes of descendent windows
	 */
	for (wchild = w->w_link[WL_YOUNGESTCHILD]; wchild;
	    wchild = wchild->w_link[WL_OLDERSIB]) {
		wchild->w_link[WL_PARENT] = NULL;
		wt_enumeratechildren(winkill, wchild, (struct rect *)0);
	}
	/*
	 * If w is rootwindow then close desktop as well.
	 */
	if (w->w_flags & WF_ROOTWINDOW && dtop != NULL) {
		dtop_close(dtop);
	}
	bzero((caddr_t)w, sizeof (*w));
	if (!winsopen) {
		int	wi;

		/*
		 * Just deallocate when no more windows around
		 */
		for (wi = 0; wi < win_nwindows; wi++) {
			if (winbufs[wi] != NULL) {
				kmem_free((caddr_t)winbufs[wi],
						sizeof (struct window));
				winbufs[wi] = NULL;
			}
		}
		/*
		 * Zero global char count
		 */
		winclistchars = 0;
	}
	winclosebusy = 0;
	(void) splx(pri);
	wakeup((caddr_t)&winclosebusy);
	return (0);
}

int
winread(dev, uio)
	dev_t	dev;
	struct	uio *uio;
{
	register struct window *w = winfromdev(dev);
	register struct win_shared_eventqueue *q = NULL;
	register int	error = 0;
	register char	c;
	int sharedq_owner = FALSE;
	int pri;

	pri = spl_timeout();
	/* Try to nudge next event off workstation's input queue */
	ws_window_nudge(w);
	if (w->w_desktop && w->w_desktop->shared_info) {
		q = &w->w_desktop->shared_info->shared_eventq;
		sharedq_owner = w->w_desktop->dt_sharedq_owner == w;
	}
	win_sharedq_shift(w);
	while (sharedq_owner ?
		(q->tail == q->head) :
		(w->w_input.c_cc < sizeof (struct inputevent)))
		if (w->w_flags&WF_NBIO) {
			(void) splx(pri);
			return (EWOULDBLOCK);
		} else {
			w->w_flags |= WF_WANTINPUT;
			(void) sleep((caddr_t)&w->w_input, STIPRI);
			win_sharedq_shift(w);
		}
	if (sharedq_owner) {
		struct inputevent *events = w->w_desktop->dt_sharedq;
		while (!error && q->tail != q->head && uio->uio_resid) {
			error = uiomove((caddr_t) (events + q->head),
				sizeof (struct inputevent),
				UIO_READ, uio);
			if (q->head == w->w_desktop->dt_sharedq_size - 1)
				q->head = 0;
			else
				q->head++;
		}
	} else {
		while (!error && w->w_input.c_cc && uio->uio_resid) {
			c = getc(&w->w_input);
			winclistchars--;
			error = ureadc(c, uio);
		}
	}
	(void) splx(pri);
	return (error);
}

/*ARGSUSED*/
int
winwrite(dev, uio)
	dev_t	dev;
	struct	uio *uio;
{
#ifdef WINSVJ
	return (svjwrite(dev, uio));
#else
	return (0);
#endif
}

/*
 * Send fe to w if mask allows it.
 * Return -1 if don't, 0 otherwise.
 */
int
win_send_one(w, fe, mask, other_mask)
	register Window *w;
	register Firm_event *fe;
	register struct inputmask *mask;
	struct inputmask *other_mask;
{
	register int flags;
	extern u_char kb_numlock_table[];

#ifdef WINDEVDEBUG
if (win_send_one_debug)
	printf("win_send_one w=%X, id=%D, value=%D, mask=%X",
		w, fe->id, fe->value, mask);
#endif
	/* Determine if event should be thought of as negative */
	if ((fe->value == 0) &&
	    ((fe->pair_type == FE_PAIR_NONE) || (fe->pair_type == FE_PAIR_SET)))
		flags = IE_NEGEVENT;
	else
		flags = 0;
	/*
	 * See if event id falls outside input mask domain.
	 * We don't yet have a mechanism for masking non-standard
	 * vuid events.  Thus, every application gets all non-standard
	 * vuid events, both positive and negative.
	 */
	if (!(((((short)fe->id) >= ASCII_FIRST) && (fe->id <= TOP_LAST)) ||
	    ((fe->id >= VKEY_FIRST) && (fe->id <= VKEY_LAST)))) {
		winsendevent(w, fe->id, flags, &fe->time);
#ifdef WINDEVDEBUG
if (win_send_one_debug) printf(" other\n");
#endif
		return (0);
	}
	if ((((short)fe->id) >= ASCII_FIRST) && (fe->id <= ASCII_LAST)) {
		/*
		 * Event is ASCII key.  If IM_{NEG}EUC or IM_{NEG}ASCII is set,
		 * send it up.
		 */
		if ((mask->im_flags & IM_EUC) && (flags == 0)) {
			winsendevent(w, fe->id, flags, &fe->time);
if (win_send_one_debug) printf(" euc (lower half)\n");
			return (0);
		}
		if ((mask->im_flags & IM_NEGEUC) && (flags == IE_NEGEVENT)) {
			winsendevent(w, fe->id, flags, &fe->time);
if (win_send_one_debug) printf(" neg euc (lower half)\n");
			return (0);
		}
		if ((mask->im_flags & IM_ASCII) && (flags == 0)) {
			winsendevent(w, fe->id, flags, &fe->time);
#ifdef WINDEVDEBUG
if (win_send_one_debug) printf(" ascii\n");
#endif
			return (0);
		}
		if ((mask->im_flags & IM_NEGASCII) && (flags == IE_NEGEVENT)) {
			winsendevent(w, fe->id, flags, &fe->time);
#ifdef WINDEVDEBUG
if (win_send_one_debug) printf(" neg ascii\n");
#endif
			return (0);
		}
		goto ignore;
	}
	if ((fe->id >= META_FIRST) && (fe->id <= META_LAST)) {
		/*
		 * Event is either an EUC character event in the "upper half"
		 * or a META event.  You can tell the difference by checking
		 * wheether the META bit is in the shift mask.
		 *
		 * If it's an EUC event, send it up as is only if IM_{NEG}EUC
		 * is set, and send it up in backwards-compatible form if
		 * IM_ISO is set.  (Map the event to the ISO range for
		 * backwards compatibility.)  (Note that there is no IM_NEGISO
		 * bit.  Is this a bug?)
		 *
		 * If it's a META event, send it up only if IM_{NEG}META
		 * is set.
		 */
		if (!(w->w_ws->ws_shiftmask & META_SHIFT_MASK)) {
			/*
			 * "Upper half of EUC" event.  If requesting EUC
			 * events, send it up as is; if requesting ISO events,
			 * send it up translated; otherwise, ignore it.
			 */
			if ((mask->im_flags & IM_EUC) && (flags == 0)) {
				winsendevent(w, fe->id, flags, &fe->time);
if (win_send_one_debug) printf(" euc (upper half)\n");
				return (0);
			}
			if ((mask->im_flags & IM_NEGEUC) && (flags == IE_NEGEVENT)) {
				winsendevent(w, fe->id, flags, &fe->time);
if (win_send_one_debug) printf(" neg euc (upper half)\n");
				return (0);
			}
			if ((mask->im_flags & IM_ISO) && (flags == 0)) {
				fe->id += (ISO_FIRST - ASCII_FIRST);
				winsendevent(w, fe->id, flags, &fe->time);
if (win_send_one_debug) printf(" iso\n");
				return (0);
			}
		} else {
			/*
			 * "Meta" event.  If requesting META events, send it up
			 * as is; otherwise, ignore it.
			 */
			if ((mask->im_flags & IM_META) && (flags == 0)) {
				winsendevent(w, fe->id, flags, &fe->time);
#ifdef WINDEVDEBUG
if (win_send_one_debug) printf(" meta\n");
#endif
				return (0);
			}
			if ((mask->im_flags & IM_NEGMETA) && (flags == IE_NEGEVENT)) {
				winsendevent(w, fe->id, flags, &fe->time);
#ifdef WINDEVDEBUG
if (win_send_one_debug) printf(" neg meta\n");
#endif
				return (0);
			}
		}
		goto ignore;
	}
	/* Try to send top */
	if ((fe->id >= TOP_FIRST) && (fe->id <= TOP_LAST)) {
		if ((mask->im_flags & IM_TOP) && (flags == 0)) {
			winsendevent(w, fe->id, flags, &fe->time);
#ifdef WINDEVDEBUG
if (win_send_one_debug) printf(" top\n");
#endif
			return (0);
		}
		if ((mask->im_flags & IM_NEGTOP) && (flags == IE_NEGEVENT)) {
			winsendevent(w, fe->id, flags, &fe->time);
#ifdef WINDEVDEBUG
if (win_send_one_debug) printf(" neg top\n");
#endif
			return (0);
		}
		goto ignore;
	}
	/*
	 * Try to send escape sequence if asked for ascii or EUC but not
	 * given function key.  Only send if the "other" input mask not
	 * interested in the function key, i.e., function keys take
	 * priority over escape sequences.
	 */
	if ((fe->id >= KEY_LEFTFIRST) && (fe->id <= KEY_BOTTOMLAST) &&
	    (!win_getinputcodebit(mask, fe->id)) &&
	    (mask->im_flags & (IM_ASCII|IM_EUC)) && (flags == 0)) {
		char	buf[10], *strsetwithdecimal() /* from kbd.c */;
		char	*cp;
		u_int	entry;
		struct	timeval to;

		/*
		 * Only send if the "other" input mask not interested in the
		 * function key, i.e., function keys take priority over escape
		 * sequences.
		 */
#ifdef WINDEVDEBUG
if (win_send_one_other_debug && other_mask)
	printf("other_mask %X, bit %D\n",
		other_mask, win_getinputcodebit(other_mask, fe->id));
#endif
		if ((other_mask != INPUTMASK_NULL) &&
		    (win_getinputcodebit(other_mask, fe->id)))
			goto ignore;
#ifdef WINDEVDEBUG
if (win_send_one_debug) printf(" escape sequence\n");
#endif
		/* Get string going to send */
		entry = fe->id - KEY_LEFTFIRST + 192;
		cp = strsetwithdecimal(&buf[0], entry, sizeof (buf) - 1);
		/* Turn off event lock for entire escape sequence */
		to = w->w_ws->ws_eventtimeout;
		timerclear(&w->w_ws->ws_eventtimeout);
		/* Send escape sequence */
		win_send_unlocked(w, (u_short)'\033', flags, &fe->time); /*Esc*/
		win_send_unlocked(w, (u_short)'[', flags, &fe->time);
		while (*cp != '\0') {
			win_send_unlocked(w, (u_short)*cp, flags, &fe->time);
			cp++;
		}
		/* Restore event lock for last character in escape sequence */
		w->w_ws->ws_eventtimeout = to;
		winsendevent(w, (u_short)'z', flags, &fe->time);
		/* Surpress sending up for this function key */
		win_setinputcodebit(&w->w_ws->ws_surpress_mask, fe->id);
		return (0);
	}

	/*
	 * Try to send keypad character if asked for ascii or EUC but not
	 * given keypad key.  Only send if the "other" input mask not
	 * interested in the function key, i.e., keypad keys take
	 * priority over keypad characters.
	 */
	if ((fe->id >= VKEY_FIRSTPAD) && (fe->id <= VKEY_LASTPAD) &&
	    (!win_getinputcodebit(mask, fe->id)) &&
	    (mask->im_flags & (IM_ASCII|IM_EUC)) && (flags == 0)) {
		/*
		 * Only send if the "other" input mask not interested in the
		 * keypad key, i.e., keypad keys take priority over keypad
		 * characters.
		 */
if (win_send_one_other_debug && other_mask) {
	printf("other_mask %X, bit %D\n",
		other_mask, win_getinputcodebit(other_mask, fe->id));
}
		if ((other_mask != INPUTMASK_NULL) &&
		    (win_getinputcodebit(other_mask, fe->id)))
			goto ignore;
if (win_send_one_debug) printf(" keypad key\n");
		fe->id = kb_numlock_table[fe->id - VKEY_FIRSTPAD];
		if (!(mask->im_flags & IM_EUC) &&
		    w->w_ws->ws_shiftmask & META_SHIFT_MASK)
			fe->id |= 0x80;
		winsendevent(w, fe->id, flags, &fe->time);
		return (0);
	}

	/*
	 * Try to send LOC_DRAG.  Do before general purpose
	 * send because want to send this variant on LOC_MOVE instead of
	 * LOC_MOVE if also on in mask.  LOC_DRAG is the same as
	 * LOC_MOVEWHILEBUTDOWN.
	 */
#ifndef lint
	if ((fe->id == LOC_MOVE) &&
	    (win_getinputcodebit(mask, LOC_DRAG))) {
		register Vuid_state is = w->w_desktop->dt_ws->ws_instate;

		if ((vuid_get_value(is, MS_LEFT)) ||
		    (vuid_get_value(is, MS_MIDDLE)) ||
		    (vuid_get_value(is, MS_RIGHT))) {
			winsendevent(w, LOC_DRAG, flags, &fe->time);
#ifdef WINDEVDEBUG
if (win_send_one_debug) printf(" LOC_DRAG\n");
#endif
			return (0);
		}
	}
#endif
	/*
	 * Try to send LOC_TRAJECTORY.  Do before general purpose
	 * send because want to send this variant on LOC_MOVE instead of
	 * LOC_MOVE if also on in mask.
	 */
#ifndef lint
	if ((fe->id == LOC_MOVE) &&
	    (win_getinputcodebit(mask, LOC_TRAJECTORY))) {
		winsendevent(w, LOC_TRAJECTORY, flags, &fe->time);
#ifdef WINDEVDEBUG
if (win_send_one_debug) printf(" LOC_TRAJECTORY\n");
#endif
		return (0);
	}
#endif
	/*
	 * Send only if mask indicates and the event is positive or
	 * the event is negative and the mask enables negative events.
	 */
	if ((win_getinputcodebit(mask, fe->id)) &&
	    ((flags == 0) ||
	    ((mask->im_flags & IM_NEGEVENT) && (flags == IE_NEGEVENT)))) {
		/* Don't send if ws_surpress_mask bit is set. */
		if (win_getinputcodebit(&w->w_ws->ws_surpress_mask, fe->id) &&		    (fe->id >= KEY_LEFTFIRST) && (fe->id <= KEY_BOTTOMRIGHT))
			goto ignore;
		winsendevent(w, fe->id, flags, &fe->time);
#ifdef WINDEVDEBUG
if (win_send_one_debug) printf(" vuid\n");
#endif
		return (0);
	}
	/*
	 * No longer supporting IM_UNENCODED (pure device codes).
	 *
	 * Never supported IM_POSASCII (no neg ASCII even if IM_NEGEVENT).
	 * Should invent new IM_NEGASCII & IM_NEGMETA options.
	 *
	 * Never supported IM_ANSI (ansi with funcs encoded in ESC[).
	 * It is the applications responsibility to turn function key
	 * events into escape sequences.
	 */
ignore:
#ifdef WINDEVDEBUG
if (win_send_one_debug) printf(" IGNORED\n");
#endif
	return (-1);
}

int
win_send_it(kbd_win, pick_win, fe)
	register Window *kbd_win;
	register Window *pick_win;
	Firm_event *fe;
{
	/* See if kbd window wants the event */
	if ((kbd_win != WINDOW_NULL) &&
	    (kbd_win->w_flags & WF_KBD_MASK_SET) &&
	    (win_send_one(kbd_win, fe, &kbd_win->w_kbdmask,
	    (pick_win != WINDOW_NULL)? &pick_win->w_pickmask :
			INPUTMASK_NULL) == 0))
		return (0);
	/* See if pick window wants the event */
	if ((pick_win != WINDOW_NULL) &&
	    (win_send_one(pick_win, fe, &pick_win->w_pickmask,
			INPUTMASK_NULL) == 0))
		return (0);
	return (-1);
}

win_send(ws, fe)
	Workstation *ws;
	register Firm_event *fe;
{
	register Window *w;
#define	WIN_PATH_MAX	15
	int count = 0;

	if (win_send_it(ws->ws_kbdfocus, ws->ws_pickfocus, fe) == 0)
		goto Done;
	/* Try to deliver event to pick focus' input link or its parent(s) */
	w = ws->ws_pickfocus;
	while (w != WINDOW_NULL) {
		/* Decide who to try next */
		if ((w->w_inputnext != WINDOW_NULL) &&
		    (w->w_inputnext->w_flags & WF_OPEN))
			w = w->w_inputnext;
		else
			w = w->w_link[WL_PARENT];
		if (win_send_it(w, w, fe) == 0)
			goto Done;
		if (count++ > WIN_PATH_MAX) {
			win_infinite_redirect++;
			break;
		}
	}
	/* Dropped event on the floor */
Done:
	/* Reenable function key on up if surpressed due to escape sequence */
	if ((fe->id >= KEY_LEFTFIRST) && (fe->id <= KEY_BOTTOMRIGHT) &&
	    (fe->value == 0))
		win_unsetinputcodebit(&ws->ws_surpress_mask, fe->id);
}

winwakeup(w)
	register struct window *w;
{

	w->w_flags &= ~WF_WANTINPUT;
	if (w->w_rsel) {
		selwakeup(w->w_rsel, w->w_flags & WF_RCOLL);
		w->w_flags &= ~WF_RCOLL;
		w->w_rsel = 0;
	}
	wakeup((caddr_t)&w->w_input);
}

int
winselect(dev, rw)
	dev_t	dev;
	int	rw;
{
	register struct window *w = winfromdev(dev);
	register Desktop *dtop = w->w_desktop;
	extern	int selwait;	/* see ../h/systm.h */
	int pri = spl_timeout();

	win_sharedq_shift(w);
	switch (rw) {

	case FREAD:
		/* Try to nudge next event off workstation's input queue */
		ws_window_nudge(w);
		if (dtop && dtop->dt_sharedq_owner == w &&
		    dtop->shared_info->shared_eventq.tail !=
		    dtop->shared_info->shared_eventq.head)
			goto win;
		if (w->w_input.c_cc >= sizeof (struct inputevent))
			goto win;
		w->w_flags |= WF_WANTINPUT;
		if (w->w_rsel && w->w_rsel->p_wchan == (caddr_t)&selwait)
			w->w_flags |= WF_RCOLL;
		else
			w->w_rsel = u.u_procp;
		break;

	case FWRITE:
		goto win;
	}
	(void) splx(pri);
	return (0);
win:
	(void) splx(pri);
	return (1);
}

/*
 * Utilities
 */
win_send_unlocked(w, code, flags, time_ptr)
	register struct	window *w;
	u_short	code;
	short	flags;
	struct	timeval *time_ptr;
{
	if ((w == WINDOW_NULL) || !(w->w_ws))
		return;
	winsendevent(w, code, flags, time_ptr);
	wlok_unlock(&w->w_ws->ws_eventlock);
}

winsendevent(w, code, flags, time_ptr)
	register struct	window *w;
	u_short	code;
	short	flags;
	struct	timeval *time_ptr;
{
	register int i, notmoved = 0, moved2sharedq = 0;
	struct	inputevent event;
	struct	desktop *dtop;
	Workstation *ws;
#ifdef WINSVJ
	void	svjsendevent();
#endif

	if ((w == WINDOW_NULL) || !(dtop = w->w_desktop) || !(ws = dtop->dt_ws))
		return;

#ifdef WINSVJ
	if (ws->ws_flags&WSF_RECORD_EVENT)
		(void) svjsendevent(code, ws, time_ptr);
#endif

	win_sharedq_shift(w);
	/*
	 * Try to keep window system overall usage to a limit
	 * but allow small clist consumers through.
	 */
	if ((winclistchars > winclistcharsmax) &&
	    (w->w_input.c_cc >= sizeof (event) * 10)) {
		if (wintossmsg)
			printf("Win sys tossing input!\n");
		goto Wakeup;
	}
	/* Construct struct input event to send */
	event.ie_code = code;
	event.ie_flags = flags;
	event.ie_shiftmask = ws->ws_shiftmask;
	event.ie_locx = dtop->dt_ut_x - w->w_screenx;
	event.ie_locy = dtop->dt_ut_y - w->w_screeny;
	event.ie_time = *time_ptr;
	/* Place on windows clist */
	if (dtop->dt_sharedq_owner == w) {
		notmoved = dtop_event_to_sharedq(dtop, &event);
		moved2sharedq = !notmoved;
	}
	if (!moved2sharedq &&
	    (notmoved = b_to_q((caddr_t)&event, sizeof (event), &w->w_input))) {
		for (i = sizeof (event) - notmoved; i; i--) {
			if (unputc(&w->w_input) == -1) {
#ifdef WINDEVDEBUG
				printf("Trouble unputc in winsendevent\n");
#endif
				break;
			}
		}
	}
#ifdef WINDEVDEBUG
	if (notmoved && wintossmsg)
		printf("Win sys tossing partial input!\n");
#endif
	if (!moved2sharedq)
		winclistchars += sizeof (event) - notmoved;
	/* Acquire event lock */
	ws_lock_event(ws, w);
Wakeup:
	if (w->w_flags & WF_ASYNC) {
		if (w->w_aproc->p_pid == w->w_apid)
			psignal(w->w_aproc, SIGIO);
	}
	if (w->w_flags & WF_WANTINPUT)
		winwakeup(w);
	return;
}

/*
 * Called from wt_enumeratechildren so don't change calling sequence
 */
/*ARGSUSED*/
winkill(w, rect)
	struct	window *w;
	struct	rect *rect;	/* Don't use for anything else (not caddr_t) */
{

	if (!(w->w_flags & WF_OPEN)) {
		return (-1);
	} else {
		if (w->w_pid != winpsnotkill)
			winsignal(w, SIGTERM);
	}
	return (0);
}

winsignal(w, sig)
	struct window *w;
	int sig;
{
	extern	struct proc *pfind();
	register struct proc *p = pfind(w->w_pid);

	if (p)
		psignal(p, sig);
}

win_freecmapdata(w)
	register struct window *w;
{
	cms_freecmapdata(w->w_cmapdata, &w->w_cmap, w->w_cms.cms_size);
	w->w_cmapdata = 0;
}


/* return the address of the window's shared lock block */
/*ARGSUSED*/
winmmap(dev, off, prot)
	dev_t	dev;
	off_t	off;
	int	prot;
{
	Window *w = winfromdev(dev);
	register caddr_t addr;
	u_int	hat_getkpfnum();

	if (!w || !w->w_desktop || win_disable_shared_locking)
		return (-1);

	addr = (caddr_t) w->w_desktop->shared_info;
	if (off > sizeof (Win_lock_block))
		return (-1);
	return ((int)hat_getkpfnum(addr + off));
}

/*
 * Set up the mapping to window's shared lock block
 * The only reason why we don't want to just use the
 * default spec_segmap() routine directly is that
 * we want the opportunity to pick the address ourselves.
 */
int
winsegmap(dev, off, as, addrp, len, prot, maxprot, flags, cred)
	dev_t dev;
	u_int off;
	struct as *as;
	addr_t *addrp;
	u_int len;
	u_int prot, maxprot;
	u_int flags;
	struct ucred *cred;
{

	if ((flags & MAP_FIXED) == 0) {
		Window *w = winfromdev(dev);
		int addr = (int)w->w_desktop->shared_info;

		/*
		 * Pick an address consistent with the current address.
		 */

#ifndef	sun3x
		off += (addr & (shm_alignment - 1));
#else
		/* no vac on a sun3x */
		off += addr;
#endif
		map_addr(addrp, len, (off_t)off, 1);
		if (*addrp == NULL)
			return (ENOMEM);
		flags |= MAP_FIXED;
	}

	/*
	 * Now that we selected the address ourselves, let spec_map
	 * handle the rest using our fixed virtual address.
	 */
	return (spec_segmap(dev, off, as, addrp, len, prot, maxprot, flags,
				cred));
}

win_sharedq_shift(w)
	register struct window *w;
{
	register struct	desktop *dtop;
	register struct	win_shared_eventqueue *q;
	register int tail;

#ifdef WINDEVDEBUG
	extern int win_sharedq_debug;
#endif

	if ((w == WINDOW_NULL) ||
	    !(dtop = w->w_desktop) ||
	    (dtop->dt_sharedq_owner != w))
		return;
	q = &dtop->shared_info->shared_eventq;
	/* check for corrupted queue */
	if (q->head < 0 || q->head >= dtop->dt_sharedq_size ||
	    q->tail < 0 || q->tail >= dtop->dt_sharedq_size) {
		q->head = q->tail = 0;
		q->size = dtop->dt_sharedq_size;
	}
	tail = q->tail + 1;
	if (tail >= dtop->dt_sharedq_size)
		tail = 0;
#ifdef WINDEVDEBUG
	if (win_sharedq_debug && w->w_input.c_cc > 0 && tail != q->head) {
		printf("win_sharedq_shift: shifting inputevents\n");
	}
#endif
	/*
	 * while non-empty overflow q and room on shared q,
	 * shift input over to shared q.
	 */
	while (w->w_input.c_cc > 0 && tail != q->head) {
		char *cp = (char *)(&dtop->dt_sharedq[tail]);
		int i = 0, c;
		for (; i < sizeof (struct inputevent); i++, cp++) {
			c = getc(&w->w_input);
			if (c >= 0) {
				*cp = (char) c;
			} else {
#ifdef WINDEVDEBUG
				if (win_sharedq_debug) {
					printf(
"win_sharedq_shift: non integer multiple of size of struct inputevent\n");
				}
#endif
				return;
			}
		}
		q->tail = tail;
		if (++tail >= dtop->dt_sharedq_size)
			tail = 0;
	}
}
