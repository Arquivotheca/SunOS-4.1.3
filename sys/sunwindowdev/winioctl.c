#ifndef lint
static  char sccsid[] = "@(#)winioctl.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

/*
 * SunWindows ioctls, see sunwindow/win_ioctl.h.
 */

#include <sunwindowdev/wintree.h>
#include <sunwindow/cursor_impl.h> /* for cursor access macros */
#include <sunwindow/win_enum.h>
#include <sunwindow/win_ioctl.h>
#include <pixrect/pr_util.h>		/* for mpr support */
#include <pixrect/pr_dblbuf.h> /* for double buffering support */
#include <pixrect/pr_planegroups.h>	/* for plane groups */
#include <sys/kernel.h>	/* for time */
#include <sunwindow/pw_dblbuf.h>
#include <sun/fbio.h>

#ifdef WINDEVDEBUG
int	winprintall;	/* Temp: Debugging message when call winioctl */
int	winprintexits;	/* Temp: Debugging message when return from winioctl */
int	winfilterlockscreen;	/* Temp: Don't print debugging messages about
				WINLOCKSCREEN and WINUNLOCKSCREEN calls */
#endif
int	winnodisplaylock;	/* Temp: Don't actually lock if WINLOCKSCREEN */

/*
 * Scaling is accomplished by finding the first  entry in the following
 *	table whose ceiling >= the current motion, and multiplying that
 *	motion by the corresponding factor.
 */
Ws_scale	ws_scaling[WS_SCALE_MAX_COUNT+1] = {
    {WS_SCALE_MAX_CEILING, 2}
};

extern int	MS_DEBUG;

extern int	ws_button_order;	/*  current button mapping  */

extern	void ws_set_dtop_waiting();
extern	void dtop_putcolormap();

extern	struct window *wt_intersected();
extern	int winkill(), wt_positionchanged(), wt_partialrepair(),
	    wt_sizechanged();
extern	int winpsnotkill;	/* Reach around communication from
				   WINSCREENDESTROY to winkill */

extern	struct proc *pfind();
extern	bool wt_isindisplaytree();
extern	caddr_t dtop_alloccmapdata();
extern	void dtop_update_enable();
extern	void dtop_choose_dblbuf();
extern	void dtop_changedisplay();
extern	void dtop_flip_display();
extern  void dtop_copy_dblbuffer();

extern	Workstation *ws_indev_match_name();
extern	Workstation *ws_open();
extern	Wsindev *ws_open_indev();

extern	int copyin(), copyout();

static	int	check_crosshair_info(),
		wt_gettreelayer();

static	void	wt_copy_layer_node(),
		wt_count_layer();

static	void	win_prepare_surface();
static	void	win_return_cms();
static	void	win_pick_plane_group();
static	void	win_initialize_plane_group();
static	void	win_clear_cursor_plane_groups();

int	win_errno;	/* Set from calls out of module (same values as errno)*/
int	win_exit_seconds = 1000;
int     print_waiting = TRUE;
int     print_still_waiting = TRUE;

int	always_signal = 1;

/*ARGSUSED*/
int
winioctl(dev, cmd, data, flag)
	dev_t dev;
	register caddr_t data;
{
	register struct window *w = winfromdev(dev);
	register struct desktop *dtop = w->w_desktop;
	register Workstation *ws = NULL;
	register int i;
	int pri;
	register int error = 0;
#ifdef WINSVJ
	register short win_playrec_sync_flg = 1; /* Flag for syncing type */
#endif
	extern winnotify_inorder;

#define	returnerror(e)	{error = (e); goto done;}
#ifdef WINDEVDEBUG
	if (winprintall) {
		if (winfilterlockscreen && (cmd == WINLOCKSCREEN ||
		    cmd == WINUNLOCKSCREEN))
			{}
		else
		  printf("winioctl dev: %X, cmd: %X, data: %X [%X,%X,%X...]\n",
		      dev, cmd, data, *(int *)data, *(int *)(data+4),
		      *(int *)(data+8));
	}
#endif
	/* Block out timeouts */
	pri = spl_timeout();
	if (dtop == NULL) {
		/*
		 * All but the following operations need dtop not NULL because
		 * so much of the code assumes a not NULL dtop.
		 * Note: the list of allowable operations should be expanded.
		 */
		switch (cmd) {
		case WINSCREENNEW:
		case WINSETLINK: 
		case WINSETRECT: 
		case WINGETRECT:
		case WINSETSAVEDRECT:
		case WINGETSAVEDRECT:
		case WINNEXTFREE:
		case WINGETBUTTONORDER:
		case WINSETBUTTONORDER:
		case WINGETSCALING:
		case WINSETSCALING:
		case WINGETUSERFLAGS:
		case WINSETUSERFLAGS:
#ifndef PRE_FLAMINGO
		case WINGETNOTIFYALL:
		case WINSETNOTIFYALL:
#endif
		case _IO(g, 100):
			break;
		default:
			returnerror(ESPIPE);
		}
	} else {
		ws = dtop->dt_ws;
		if (!dtop->shared_info) /* Has happened but don't know why */
			returnerror(ESPIPE);
		dtop->shared_info->waiting++;
		if (wlok_waitunlocked(&dtop->dt_datalock)) {
			if (dtop->shared_info)
				dtop->shared_info->waiting--;
			goto restart;
		}
		dtop->shared_info->waiting--;
		switch (cmd) {
		/*
		 * Don't tamper with display lock while locked by another.
		 */
		case WINLOCKSCREEN:
		case WINUNLOCKSCREEN:
		case WINGRABIO:
		/*
		 * Any operation that involes double buffering waits
		 * while the display is locked.
		 *
		 */
		case WINDBLCURRENT:
		case WINDBLACCESS:
		case WINDBLRLSE:
		case WINDBLFLIP:
		case WINDBLABSORB:
		case WINDBLSET:
		case WINDBLGET:
		/*
		 * Any operation that might invoke a clipping change while the
		 * display is locked must wait until the lock is released.
		 * An exception to this is if the calling user process is
		 * the same as the original locker.
		 */
		case WININSERT:
		case WINREMOVE: 
		case WINSETRECT:
		case WINSETOWNER:
		case WINUNLOCKDATA:
		case WINSCREENDESTROY:
		/*
		 * Don't tamper with colormap while display locked by another.
		 */
		case WINSETCMS:
		case WINRELEASEIO:
		/*
		 * Don't tamper with plane group while display locked
		 * by another.
		 */
		case WINSETPLANEGROUP:
		case WINGETDAMAGEDRL:
			/* Make sure dt still around if waited before */
			if (!(dtop->dt_flags & DTF_PRESENT))
				returnerror(EACCES);
			/* Bump the waiting count and wait for the
			 * display lock to be released, or just
			 * go on through if we have the display lock.
			 * Note that we look at the shared lock only,
			 * since we can't trust the internal wlok after
			 * we sleep.
			 */
			dtop->shared_info->waiting++;
			dtop->display_waiting++;
			while (win_lock_display_locked(dtop->shared_info) &&
			       (dtop->shared_info->display.pid != u.u_procp->p_pid))
			    if (sleep((caddr_t)dtop->shared_info,
				LOCKPRI|PCATCH)) {
				if (dtop->shared_info)
					dtop->shared_info->waiting--;
				dtop->display_waiting--;
				goto restart;
			    }
			/* Now make sure the internal display lock
			 * is valid.  We have to do this here because
			 * we don't know what went on while we were sleeping.
			 */
			dtop_validate_shared_lock(dtop, &dtop->dt_displaylock);
			dtop->shared_info->waiting--;
			dtop->display_waiting--;
		}
		switch (cmd) {
		/*
		 * Don't tamper with colormap while io rights locked by another.
		 */
		case WINSETCMS:
		case WINSETPLANEGROUP:
		/*
		* Don't tamper with io right lock while locked by another.
		*/
		case WINGRABIO:
		case WINRELEASEIO:
		case WINDBLCURRENT:
		case WINDBLACCESS:
		case WINDBLRLSE:
		case WINDBLFLIP:
		case WINDBLABSORB:
		case WINDBLSET:
		case WINDBLGET:
		/*
		* Don't acquire data lock while io rights locked by another.
		* If we did, one could get into a situation in which the
		* data lock holder might try to access the screen but be
		* prevented by the other process having the io lock.  Also,
		* the io lock holder couldn't access the screen because
		* the data lock is more powerful.  Deadlock.
		*/
		case WINLOCKDATA:
			if (ws == NULL)
				returnerror(ESPIPE);
			/* Make sure ws still around if waited before */
			if (!(ws->ws_flags & WSF_PRESENT))
				returnerror(EACCES);
			ws_set_dtop_waiting(ws, 1);
			if (wlok_waitunlocked(&ws->ws_iolock)) {
				ws_set_dtop_waiting(ws, -1);
				goto restart;
			}
			ws_set_dtop_waiting(ws, -1);
		}
		/* Make sure ws & dt still around if waited before */
		if ((!(dtop->dt_flags & DTF_PRESENT)) ||
		    (!(ws->ws_flags & WSF_PRESENT)))
			returnerror(EACCES);
	}

	if (ws == NULL) {
		/*
		 * Same drill as above, except this time for ws -- avoid
		 * operations that assume it's nonnull.  This is a gross hack;
		 * the code leading to this point should have insured that ws
		 * is nonnull, but unfortunately does not always do so.
		 */
		switch (cmd) {
		case WINSETMOUSE:
		case WINSCREENDESTROY:
		case WINSETKBD:
		case WINSETMS:
		case WINSETINPUTDEV:
		case WINGETINPUTDEV:
		case WINGETVUIDVALUE:
		case WINGETFOCUSEVENT:
		case WINSETFOCUSEVENT:
		case WINGETSWALLOWEVENT:
		case WINSETSWALLOWEVENT:
		case WINGETKBDFOCUS:
		case WINSETKBDFOCUS:
		case WINGETEVENTTIMEOUT:
		case WINSETEVENTTIMEOUT:
		case WINUNLOCKEVENT:
		case WINREFUSEKBDFOCUS:
			returnerror(ESPIPE);
		}
	}

	switch (cmd) {

	/*
	 * Display management operations
	 * (placed at beginning for faster access[?] because high frequency).
	 */
	case WINLOCKSCREEN: {
		struct	winlock *winlock = (struct winlock *)data;
		int	new_id;

		if (winnodisplaylock)
			goto done;
		new_id = dtop_lockdisplay(dev, &winlock->wl_rect);
		if (new_id < 0)
			goto restart;

		winlock->wl_clipid = new_id;
		break;
		}

	case WINUNLOCKSCREEN:
		if (winnodisplaylock)
			goto done;
		wlok_unlock(&dtop->dt_displaylock);
		break;

	case FIONREAD:
		/* return # of CHARACTERS (not events) immediately available */
		win_sharedq_shift(w);
		if (dtop && w->w_desktop->dt_sharedq_owner == w) {
			if (dtop->shared_info->shared_eventq.tail >=
			    dtop->shared_info->shared_eventq.head) {
				*(int *)data =
				dtop->shared_info->shared_eventq.tail -
				dtop->shared_info->shared_eventq.head;
			} else {
				*(int *)data =
				dtop->shared_info->shared_eventq.tail +
				dtop->shared_info->shared_eventq.size -
				dtop->shared_info->shared_eventq.head;
			}
			*(int *)data *= sizeof (struct inputevent);
			*(int *)data += w->w_input.c_cc;
		} else
			*(int *)data = w->w_input.c_cc;
		break;

	case FIOASYNC:

		/*
		 * Note: If ignore FIOASYNC then
		 * need to explicitely ignore it due to way that fcntl is
		 * implemented in ../sys/kern_descrip.c.
		 */
		if (*(int *)data) {
			w->w_flags |= WF_ASYNC;
			w->w_aproc = u.u_procp;
			w->w_apid = u.u_procp->p_pid;
		} else {
			w->w_flags &= ~WF_ASYNC;
			w->w_aproc = 0;
			w->w_apid = 0;
		}
		break;

	case FIONBIO:
		if (*(int *)data)
			w->w_flags |= WF_NBIO;
		else
			w->w_flags &= ~WF_NBIO;
		break;

	/*
	 * Tree operations
	 */
	case WINGETLINK: {
		struct	winlink *winlink = (struct winlink *)data;

		if ((unsigned)winlink->wl_which >= WIN_LINKS)
			returnerror(EINVAL);
		wt_windowtonumber(w->w_link[winlink->wl_which],
		    &winlink->wl_link);
		break;
		}

	case WINSETLINK: {
		struct	winlink *winlink = (struct winlink *)data;
		struct	window *windowlink;

		if ((unsigned)winlink->wl_which >= WIN_LINKS)
			returnerror(EINVAL);
		if (wt_numbertowindow(winlink->wl_link, &windowlink) < 0)
			returnerror(EINVAL);
		if (w->w_flags & WF_INSTALLED || (windowlink == w))
			returnerror(EINVAL);
		w->w_link[winlink->wl_which] = windowlink;
		if (winlink->wl_which==WL_PARENT && w->w_link[WL_PARENT]) {
			w->w_desktop = w->w_link[WL_PARENT]->w_desktop;
			if (w->w_desktop) {
				/* Make sure dt still around before reference */
				if (!(w->w_desktop->dt_flags & DTF_PRESENT))
					returnerror(EACCES);
				w->w_ws = w->w_desktop->dt_ws;
				win_pick_plane_group(w->w_desktop, w);
			}
		}
		break;
		}

	case WININSERT:
		if (dtop->dt_flags & DTF_EXITING) {
			winsignal(w, SIGKILL);
			returnerror(EACCES);
		}
		if ((error = wt_install(w)) != 0)
			returnerror(error);
		if (wt_isindisplaytree(w))
			dtop_invalidateclipping(
			    dtop, w->w_link[WL_PARENT], &w->w_rect);
		break;

	case WINREMOVE:
		if ((error = dtop_removewin(dtop, w)))
			returnerror(error);
		break;

	case WINNEXTFREE: {
		struct	winlink *winlink = (struct winlink *)data;
		struct	window *win;

		winlink->wl_link = WIN_NULLLINK;
#if NWIN > 0
		for (i = 0; win = winfromdev(i); i++)
			if (!(win->w_flags&WF_OPEN)) {
				wt_windowtonumber(win, &winlink->wl_link);
				break;
			}
#endif
		break;
		}

	case WINGETTREELAYER:
		if (dtop->dt_flags & DTF_EXITING) {
			winsignal(w, SIGKILL);
			returnerror(EACCES);
		}
		if ((error = wt_gettreelayer(w, (Win_tree_layer	*)data)))
			returnerror(error);
		break;

	/*
	 * Mouse cursor operations.
	 */
	case WINSETMOUSE: {
		struct	mouseposition *mp = (struct mouseposition *)data;
		Firm_event fe;
		int delta;
		Desktop *dtop_old = ws->ws_loc_dtop;

		if (!dtop_old) {
			returnerror(ENOENT);
		}
		if (dtop != dtop_old) {
			void dtop_change_loc_dtop();

			dtop_change_loc_dtop(dtop_old, dtop,
			    mp->msp_x+w->w_screenx, mp->msp_y+w->w_screeny);
		} else {
			/*
			 * Apply the absolute to the user state.  Apply the
			 * difference between the original user state and the
			 * new user state to the real time state.
			 */
			delta = mp->msp_x + w->w_screenx - dtop->dt_ut_x;
			dtop->dt_ut_x = mp->msp_x + w->w_screenx;
			dtop->dt_rt_x += delta;
			delta = mp->msp_y + w->w_screeny - dtop->dt_ut_y;
			dtop->dt_ut_y = mp->msp_y + w->w_screeny;
			dtop->dt_rt_y += delta;
		}
		/*
		 * Update the shared memory mouse x, y
		 */
		win_shared_update_mouse_xy(dtop->shared_info,
					   dtop->dt_rt_x, dtop->dt_rt_y);
		/*
		 * We enqueue a bogus LOC_MOVE at the user time end of
		 * the queue so that LOC_WINENTERs, EXITs and STILLs
		 * are generated.
		 */
		if (vq_is_full(&ws->ws_q))
			break;
		if (vq_peek(&ws->ws_q, &fe) == VUID_Q_EMPTY)
			/* Use the current time */
			fe.time = time;
		/* else use the time at the top of the queue */
		fe.id = LOC_MOVE;
		fe.value = 1; /* Set to one because event has no neg */
		fe.pair = 0;
		fe.pair_type = FE_PAIR_NONE;
		if (vq_putback(&ws->ws_q, &fe) != VUID_Q_OK)
#ifdef WINDEVDEBUG
			printf("putback error\n");
#else
			;
#endif
		/* Update the locator (real time end of queue) */
		ws_track_locator(ws);
		/* Update pick focus if changed */
		ws_set_focus(ws, FF_PICK_CHANGE);
		break;
		}

	case WINSETLOCATOR:
		/* check the validity of the
		 * crosshair info.
		 */
		if (error = check_crosshair_info((struct cursor *) data))
			returnerror(error);

		/* and fall through ... */

	case WINSETCURSOR: {
		struct	cursor *cursor = (struct cursor *)data;
		struct	pixrect mpr;
		struct	mpr_data mpr_data;
		short	image[CUR_MAXIMAGEWORDS];
		int	empty;

		if (error = winio_getusercursor(
		    cursor, &mpr, &mpr_data, image, &empty))
			returnerror(error);

		if (empty)
			/*
			 * dtop_drawcursor knows 0 depth is nop cursor.
			 */
			w->w_cursormpr.pr_depth = 0;
		else {
			/*
			 * Now that have reasonable cursor, copy to w.
			 */
			/* this if-else will go away when our
			 * kernel is released and the SETLOCATOR
			 * and GETLOCATOR code is removed.
			 */
			if (cmd == WINSETCURSOR) {
				w->w_cursor.cur_xhot = cursor->cur_xhot;
				w->w_cursor.cur_yhot = cursor->cur_yhot;
				w->w_cursor.cur_function = cursor->cur_function;
				w->w_cursor.cur_shape = cursor->cur_shape;
				w->w_cursor.flags = 0;
			} else
				w->w_cursor = *cursor;
			w->w_cursormpr = mpr;
			w->w_cursordata = mpr_data;
			bcopy((caddr_t)image, (caddr_t)w->w_cursorimage,
			    CUR_MAXIMAGEBYTES);
			/*
			 * Fixup cursor internal pointers.
			 */
			winfixupcursor(w);

                        /*
                         * Allocate crosshair cursor pixrect storage
                         * if necessary.
                         */
                        if (show_horiz_hair(cursor) ||
                                show_vert_hair(cursor))
                                dtop_setup_crosshairs(dtop);
		}
		/*
		 * Replace new for old on screen if old was up.
		 * Do it now incase client is counting on cursor state.
		 */
		if (dtop->dt_cursorwin == w) {
			dtop_cursordown(dtop);
			/* update the shared cusor */
			win_shared_update_cursor(dtop);
			dtop_cursorup(dtop);
		}
		break;
		}

	case WINGETLOCATOR:
	case WINGETCURSOR: {
		struct	cursor *cursor = (struct cursor *)data;
		struct	pixrect mpr, *mprsave;
		struct	mpr_data mpr_data, *mpr_datasave;
		struct	pixrectops *mpr_opssave;
		short	image[CUR_MAXIMAGEWORDS], *imagesave;
		int	empty;

		/*
		 * Get user cursor that will copyout to.
		 */
		if (error = winio_getusercursor(
		    cursor, &mpr, &mpr_data, image, &empty))
			returnerror(error);
		if (empty)
			returnerror(EINVAL);
		/*
		 * Save the user pointers that are needed as destinations
		 * of copyouts.
		 */
		mprsave = cursor->cur_shape;
		mpr_datasave = (struct mpr_data *)mpr.pr_data;
		mpr_opssave = mpr.pr_ops;
		imagesave = mpr_data.md_image;
		/*
		 * Copy cursor info and fixup data pointer.
		 * (Copyout happens by ioctl mechanism)
		 */
		/* this if-else will go away when our
		 * kernel is released and the SETLOCATOR
		 * and GETLOCATOR code is removed.
		 */
		if (cmd == WINGETCURSOR) {
			cursor->cur_xhot = w->w_cursor.cur_xhot;
			cursor->cur_yhot = w->w_cursor.cur_yhot;
			cursor->cur_function = w->w_cursor.cur_function;
		} else
			*cursor = w->w_cursor;
		cursor->cur_shape = mprsave;
		/*
		 * Set pixrect info, fixup pointers and copyout.
		 */
		mpr = w->w_cursormpr;
		mpr.pr_data = (caddr_t)mpr_datasave;
		mpr.pr_ops = mpr_opssave;
		if (error = copyout((caddr_t)&mpr, (caddr_t)mprsave,
		    sizeof(struct pixrect)))
			returnerror(error);
		/*
		 * Set pixrect info, fixup pointers and copyout.
		 */
		mpr_data = w->w_cursordata;
		mpr_data.md_image = imagesave;
		if (error = copyout((caddr_t)&mpr_data, (caddr_t)mpr_datasave,
		    sizeof(struct mpr_data)))
			returnerror(error);
		/*
		 * Copyout image.
		 */
		if (error = copyout((caddr_t)w->w_cursorimage,
		    (caddr_t)imagesave, CUR_MAXIMAGEBYTES))
			returnerror(error);
		break;
		}

	case WINFINDINTERSECT: {
		struct winintersect *winintersect = (struct winintersect *)data;
		struct window *window;

		window = wt_intersected(w, winintersect->wi_x + w->w_screenx,
		    winintersect->wi_y + w->w_screeny);
		wt_windowtonumber(window, &winintersect->wi_link);
		break;
		}

	/*
	 * Geometry operations.
	 */
	case WINGETRECT:
		*(struct rect *)data = w->w_rect;
		break;

	case WINSETRECT: {
		struct	rect *rect = (struct rect *)data;
		bool	ilocked = FALSE;
		struct	window *winparent;

		if (dtop == NULL) {
			w->w_rect = *rect;
			break;
		}
		if (w==dtop->dt_rootwin)
			winparent = w;
		else
			winparent = w->w_link[WL_PARENT];
		if (!win_lock_data_locked(dtop->shared_info)) {
			if (dtop_lockdata(dev))
				goto restart;
			ilocked = TRUE;
		}
		if (wt_isindisplaytree(w))
			dtop_invalidateclipping(dtop, winparent, &w->w_rect);
		if ((w->w_rect.r_width != rect->r_width) ||
		    (w->w_rect.r_height != rect->r_height))
			w->w_flags |= WF_SIZECHANGED;
		if ((w->w_rect.r_left != rect->r_left) ||
		    (w->w_rect.r_top != rect->r_top))
			wt_enumeratechildren(wt_positionchanged, w,
			    (struct rect *)0);
		w->w_rect = *rect;
		/*
		 * Let blanket windows pick up parent's new size.
		 */
		wt_enumeratechildren(wt_sizechanged, w, (struct rect *)0);
		if (wt_isindisplaytree(w))
			dtop_invalidateclipping(dtop, winparent, &w->w_rect);
		if (ilocked)
			wlok_unlock(&dtop->dt_datalock);
		break;
		}

	case WINSETSAVEDRECT:
		w->w_rectsaved = *(struct rect *)data;
		break;

	case WINGETSAVEDRECT:
		*(struct rect *)data = w->w_rectsaved;
		break;

	/*
	 * Misc operations.
	 */
	case WINGETUSERFLAGS:
		*(int *)data = w->w_userflags;
		break;

	case WINSETUSERFLAGS:
		w->w_userflags = *(int *)data;
		break;

	case WINGETOWNER: {
		struct	proc *p = pfind(w->w_pid);

		/*
		 * Return 0 if can't find current owner process
		 * Can't rely on kill(pid, 0) to tell user if processes
		 * exists because will say doesn't exist if, say, owner is root.
		 */
		*(int *)data = (p == 0)? 0: w->w_pid;
		break;
		}

	case WINSETOWNER: {
		int	pid = *(int *)data;
		struct	proc *p = pfind(pid);

		if (p==0)
			returnerror(EINVAL);
		w->w_pid = pid;
		rl_copy(&w->w_rlexposed, &w->w_rlfixup);
		winsignal(w, SIGWINCH);
		break;
		}

#ifndef PRE_FLAMINGO
	case WINGETNOTIFYALL:
		*(int *)data = (w->w_flags&WF_NOTIFYALLCHANGES) != 0;
		break;

	case WINSETNOTIFYALL:
		if (*(int *)data)
			w->w_flags |= WF_NOTIFYALLCHANGES;
		else
			w->w_flags &= ~WF_NOTIFYALLCHANGES;
		break;
#endif

	/*
	 * Input control.
	 */
	case WINGETINPUTMASK: {
		struct inputmaskdata *inputmaskdata =
		    (struct inputmaskdata *)data;

		inputmaskdata->ims_set = w->w_pickmask;
		wt_windowtonumber(w->w_inputnext,
		    &inputmaskdata->ims_nextwindow);
		break;
		}

	case WINSETINPUTMASK: {
		struct inputmaskdata *inputmaskdata =
		    (struct inputmaskdata *)data;
		struct	window *windownext;
		int	flushit = 0;

		if (wt_numbertowindow(inputmaskdata->ims_nextwindow,
		    &windownext) < 0)
			returnerror(EINVAL);
		/*
		 * Flush input buffer
		 * Note: For now flush entire queue if any thing in ims_flush
		 */
        	for (i = 0; i < IM_CODEARRAYSIZE; i++)
                	if (inputmaskdata->ims_flush.im_inputcode[i] != 0) {
				flushit = 1;
				break;
			}
		if (flushit || inputmaskdata->ims_flush.im_flags) {
			if (w->w_desktop->dt_sharedq_owner
			&&  w->w_desktop->dt_sharedq_owner == w) {
				w->w_desktop->shared_info->shared_eventq.tail =
				w->w_desktop->shared_info->shared_eventq.head;
			}
			while (getc(&w->w_input) >= 0)
				continue;
		}
		/*
		 * Set new mask
		 */
		w->w_pickmask = inputmaskdata->ims_set;
		w->w_inputnext = windownext;
		break;
		}

	/*
	 * Operations applying globally to a screen.
	 */
	case WINLOCKDATA:
		if (dtop_lockdata(dev))
			goto restart;
		break;

	case WINCOMPUTECLIPPING:
		if (!win_lock_data_locked(dtop->shared_info))
			returnerror(EINVAL);
		dtop_computeclipping(dtop, FALSE);
		break;

	case WINPARTIALREPAIR: {
		struct	rect *rectok = (struct rect *)data;

		if (!win_lock_data_locked(dtop->shared_info))
			returnerror(EINVAL);
		wt_enumeratechildren(wt_partialrepair, w, rectok);
		break;
		}

	case WINUNLOCKDATA:
		wlok_unlock(&dtop->dt_datalock);
		break;

	case WINGRABIO:
		if (ws_lockio(dev))
			goto restart;
		break;

	case WINRELEASEIO:
		wlok_unlock(&ws->ws_iolock);
		break;

	case WINGETDAMAGEDRL:
		if (dtop->dt_flags & DTF_MULTI_PLANE_GROUPS &&
		    !(w->w_flags & WF_PLANE_GROUP_SET))
			/*
			 * Prepare surface for this application because it
			 * doesn't know how to play multiple planes.
			 */
			win_prepare_surface(w);

		/* Fall through */

	case WINGETEXPOSEDRL: {
		struct	winclip *winclip = (struct winclip *)data;

		winclip->wc_clipid = w->w_clippingid;
		rect_construct(&winclip->wc_screenrect, w->w_screenx,
			w->w_screeny, w->w_rect.r_width, w->w_rect.r_height);
		returnerror(wincopyoutrl(
		    cmd == WINGETDAMAGEDRL ? &w->w_rlfixup : &w->w_rlexposed,
		    winclip));
		}

	case WINDONEDAMAGED: {
		int	userclippingid = *((int *)data);

		if (userclippingid==w->w_clippingid) {
			rl_free(&w->w_rlfixup);
			w->w_flags &= ~WF_PREVIOUSDAMAGED;
		}
		break;
		}

	/*
	 * Colormap sharing mechanism.
	 */
	case WINSETCMS: {
		register struct cmschange *cmschange = (struct cmschange *)data;
		register struct window *wmatch;
		struct	window *dtop_cmsmatch();
		int	to_offset;

		/*
		 * Cmschange->cc_cms.cms_addr describes the destination
		 * within the window's colormap segment.
		 * Always reading from cmschange->cc_map at [0]th slot.
		 */
		to_offset = cmschange->cc_cms.cms_addr;
		cmschange->cc_cms.cms_addr = 0;
		if (cmschange->cc_cms.cms_size == 0 ||
		    (cmschange->cc_map.cm_red == 0 &&
		    cmschange->cc_map.cm_green == 0 &&
		    cmschange->cc_map.cm_blue == 0)) {
			/*
			 * Releasing color map segment.
			 */
			dtop_cmsfree(dtop, w);
			/*
			 * If had semantic that color map addr could change like
			 * clipping then could retry allocate any CMSHOGs that
			 * we were not able to allocate in past and notify
			 * window with sigwinch.
			 */
		} else {
			/*
			 * If new and existing cms are same then bypass
			 * allocation phase.
			 */
			if ((cmschange->cc_cms.cms_size <= w->w_cms.cms_size)&&
			    (strncmp(cmschange->cc_cms.cms_name,
			    w->w_cms.cms_name, CMS_NAMESIZE) == 0)) {
				if (w->w_flags & WF_CMSHOG) {
					goto loadcmshog;
				} else {
					goto loadcmsstd;
				}
			}
			/*
			 * Start using a color map segment.
			 */
			if ((wmatch = dtop_cmsmatch(
			    dtop, w, &cmschange->cc_cms))) {
				/*
				 * Using an existing color map segment.
				 * Sanity check: if trying to replace
				 * the current cms with a smaller sized 
				 * cms, then return error. 
				 */
				if (w->w_cms.cms_size > wmatch->w_cms.cms_size) 
				       returnerror(EINVAL);
				w->w_cms = wmatch->w_cms;
				w->w_cmap = wmatch->w_cmap;
				w->w_cmapdata = wmatch->w_cmapdata;
				if (wmatch->w_flags & WF_CMSHOG) {
					w->w_flags |= WF_CMSHOG;
					goto loadcmshog;
				} else {
					goto loadcmsstd;
				}
			} else {
				/*
				 * Free existing cms if not shared.
				 */
				dtop_cmsfree(dtop, w);
				/*
				 * Try allocating a new color map segment.
				 */
				w->w_cms = cmschange->cc_cms;
				if ((w->w_cms.cms_addr = dtop_cmsalloc(dtop,
				    (long)w->w_cms.cms_size)) != -1) {
loadcmsstd:
					/*
					 * Allocation succeded.  Load std map.
					 */
					if (error = winioctl_cmapcopy(
					    &w->w_cms, &dtop->dt_cmap,
					    to_offset,
					    &cmschange->cc_cms,
					    &cmschange->cc_map, 0, copyin)) {
						bzero((caddr_t)&w->w_cms,
						  sizeof (struct colormapseg));
						returnerror(error);
					}
					dtop_stdizecmap(dtop, &dtop->dt_cmap,
					    w->w_cms.cms_addr,
					    w->w_cms.cms_size);
				} else {
					/*
					 * Allocation failed.  Make hog.
					 */
					w->w_cms.cms_addr = 0;
					w->w_cmapdata = dtop_alloccmapdata(dtop,
					    &w->w_cmap, w->w_cms.cms_size);
					if (w->w_cmapdata == 0) {
						returnerror(ENXIO);
					}
loadcmshog:
					if (error = winioctl_cmapcopy(
					    &w->w_cms, &w->w_cmap, to_offset,
					    &cmschange->cc_cms,
					    &cmschange->cc_map, 0, copyin)) {
						cms_freecmapdata(w->w_cmapdata,
						    &w->w_cmap,
						    w->w_cms.cms_size);
						w->w_cmapdata = 0;
						bzero((caddr_t)&w->w_cms,
						  sizeof (struct colormapseg));
						returnerror(error);
					}
					dtop_stdizecmap(dtop, &w->w_cmap,
					    w->w_cms.cms_addr,
					    w->w_cms.cms_size);
					w->w_flags |= WF_CMSHOG;
				}
			}
		}
		if (dtop->dt_cursorwin == w) {
			dtop->dt_flags |= DTF_NEWCMAP;
			dtop_set_cursor(dtop);
                } else if (dtop->dt_cursorwin &&
                    (strncmp(dtop->dt_cursorwin->w_cms.cms_name,
                    w->w_cms.cms_name, CMS_NAMESIZE) == 0)) {
                        /* Case when w is hog and cms is same as cursor win */
                        dtop->dt_flags |= DTF_NEWCMAP;
                        dtop_set_cursor(dtop);
		} else {
			/*
			 * Update color map with std if not being hogged on
			 * this desktop and change was to the std map.
			 */
			if (!(w->w_flags & WF_CMSHOG)) {
				if (dtop->dt_cursorwin &&
			            (dtop->dt_cursorwin->w_flags & WF_CMSHOG))
					{}
				else
				    dtop_putcolormap(dtop, dtop->dt_cmsize,
				        &dtop->dt_cmap);
			}
		}
		win_return_cms(w, &cmschange->cc_cms);
		break;
		}
	case WINGETCMS: {
		register struct cmschange *cmschange = (struct cmschange *)data;
		int from_offset;

		if (cmschange->cc_map.cm_red != 0 &&
		    cmschange->cc_map.cm_green != 0 &&
		    cmschange->cc_map.cm_blue != 0) {
			/*
			 * cmschange->cc_cms.cms_size is the count of the
			 * section of the color map wanted.
			 */
			if (cmschange->cc_cms.cms_size > w->w_cms.cms_size)
				returnerror(EINVAL);
			/*
			 * Cmschange->cc_cms.cms_addr describes the source
			 * within the window's colormap segment.
			 * Always loading into cmschange->cc_map at [0]th slot.
			 */
			from_offset = cmschange->cc_cms.cms_addr;
			cmschange->cc_cms.cms_addr = 0;
			if (w->w_flags & WF_CMSHOG)
				error = winioctl_cmapcopy(
				    &cmschange->cc_cms, &cmschange->cc_map, 0,
				    &w->w_cms, &w->w_cmap, from_offset,
				    copyout);
			else
				error = winioctl_cmapcopy(
				    &cmschange->cc_cms, &cmschange->cc_map, 0,
				    &w->w_cms, &dtop->dt_cmap, from_offset,
				    copyout);
			if (error)
				returnerror(error);
		}
		win_return_cms(w, &cmschange->cc_cms);
		break;
		}

	case WINSETPLANEGROUP: {
		int new_plane_group = *(int *)data;

		/* Validate plane group */
		if (new_plane_group >= DTOP_MAX_PLANE_GROUPS ||
		    dtop->dt_plane_groups_available[new_plane_group] == 0)
			returnerror(EINVAL);
		if (w->w_plane_group != new_plane_group) {
			w->w_plane_group = new_plane_group;
			/* Potentially new colormap usage (hog vs std) */
			if (dtop->dt_cursorwin == w) {
				dtop->dt_flags |= DTF_NEWCMAP;
				dtop_set_cursor(dtop);
			}
			/*
			 * Replace cursor if up because it will be in another
			 * plane group.  Do it now incase client is counting
			 * on cursor state.
			 */
			if ((dtop->dt_cursorwin == w) && 
		(!(dtop->dt_plane_groups_available[PIXPG_CURSOR]))) {
				dtop_cursordown(dtop);
				/* update the shared cusor */
				win_shared_update_cursor(dtop);
				dtop_cursorup(dtop);
			}
		}
		/* Notice that window owner knows about plane groups */
		w->w_flags |= WF_PLANE_GROUP_SET;
		break;
		}

	case WINGETPLANEGROUP:
		*(int *)data = w->w_plane_group;
		break;

	case WINGETAVAILPLANEGROUPS:
		bcopy((caddr_t)dtop->dt_plane_groups_available,
		    (caddr_t)((struct win_plane_groups_available
			*)data)->plane_groups_available,
		    WIN_MAX_PLANE_GROUPS);
		break;

	/*
	 * Screen creation, inquiry and deletion.
	 */
	case WINSCREENNEW: {
		struct usrdesktop *udtop = (struct usrdesktop *)data;
		register Desktop *dt;

		dtop = NULL;
		/* Allocate an unused desktop */
		for (dt = desktops; dt < &desktops[ndesktops]; dt++) {
			if (!dtop && !(dt->dt_flags&DTF_PRESENT))
				dtop = dt;
			if (dt->dt_screen.scr_fbname[0] != '\0' &&
			    (strncmp(udtop->udt_scr.scr_fbname,
			    dt->dt_screen.scr_fbname, SCR_NAMESIZE) == 0))
				returnerror(EBUSY);
		}
		if (!dtop || dtop->dt_rootwin)
			returnerror(EBUSY);
		/* Check window that is to become root for cleanliness */
		if ((w->w_flags & WF_INSTALLED) ||
		    w->w_link[WL_OLDERSIB] || w->w_link[WL_YOUNGERSIB] ||
		    wt_badchildren(w)) {
			returnerror(EINVAL);
		}
		/*
		 * Set up w and rootwin now so that dtop_close gets called
		 * if encounter error during opening of dtop.
		 */
		dtop->dt_rootwin = w;
		dtop->dt_exit_seconds = 0;
		w->w_desktop = dtop;
		w->w_flags |= WF_ROOTWINDOW;
		/* Initialize cursor data. */
		dtopcursorfixup(&dtop->dt_cursor);
		/* Copy screen. */
		dtop->dt_screen = udtop->udt_scr;
		/* Open frame buffer. */
		if (error = dtop_openfb(dtop, udtop->udt_fbfd)) {
			returnerror(error);
		}
		/* Initialize root windows plane group */
		win_pick_plane_group(dtop, w);
		/* Initialize root window's rect. */
		w->w_rect = dtop->dt_screen.scr_rect;
		/* Set cursor in middle of screen */
		dtop->dt_rt_x = dtop->dt_ut_x = dtop->dt_screen.scr_rect.r_left+
		    (dtop->dt_screen.scr_rect.r_width/2);
		dtop->dt_rt_y = dtop->dt_ut_y = dtop->dt_screen.scr_rect.r_top+
		    (dtop->dt_screen.scr_rect.r_height/2);
		/*
		 * Update the shared memory mouse x, y
		 */
		win_shared_update_mouse_xy(dtop->shared_info,
		    dtop->dt_rt_x, dtop->dt_rt_y);
		/*
		 * Install on desktop so that if input turn on fails then
		 * dtop_close will clean up.
		 */
		w->w_flags |= WF_INSTALLED;
		dtop->dt_flags |= DTF_PRESENT;

		/*
		 * See if input devices that window wants to use are in service.
		 * If so then use that workstation else open a new one.
		 */
		if ((ws = ws_indev_match_name(udtop->udt_scr.scr_kbdname,
		    (Wsindev **)0)) != WORKSTATION_NULL)
			goto UseWs;
		if ((ws = ws_indev_match_name(udtop->udt_scr.scr_msname,
		    (Wsindev **)0)) != WORKSTATION_NULL)
			goto UseWs;
		/* Open new workstation */
		if ((ws = ws_open()) == WORKSTATION_NULL)
			returnerror(EBUSY);
		/* Place locator on first dtop */
		ws->ws_loc_dtop = dtop;
		win_shared_update_cursor_active(dtop->shared_info, TRUE);
		ws->ws_pick_dtop = dtop;
		/* Initialize restricted plane groups */
		if (dtop->dt_screen.scr_flags & SCR_8BITCOLORONLY ||
		    dtop->dt_screen.scr_flags & SCR_OVERLAYONLY)
			dtop_update_enable(dtop, DESKTOP_NULL, 1);
UseWs:
		/*
		 * Install desktop with workstation so that if input open
		 * fails then ws_close will clean up.
		 */
		dtop->dt_next = ws->ws_dtop;
		ws->ws_dtop = dtop;
		dtop->dt_ws = ws;
		w->w_ws = ws;
		/* Invalidate clipping so that SIGWINCH generated */
		dtop_invalidateclipping(dtop, w, &w->w_rect);
		/* Open kbd & ms input devices if not already open */
		if ((udtop->udt_scr.scr_kbdname[0] != NULL) &&
		    (ws_indev_match_name(udtop->udt_scr.scr_kbdname,
		    (Wsindev **)0) == WORKSTATION_NULL)) {
			if (ws_open_indev(ws, udtop->udt_scr.scr_kbdname,
			    udtop->udt_kbdfd) == WSINDEV_NULL)
				returnerror(u.u_error);
		}
		if ((udtop->udt_scr.scr_msname[0] != NULL) &&
		    (ws_indev_match_name(udtop->udt_scr.scr_msname,
		    (Wsindev **)0) == WORKSTATION_NULL)) {
			if (ws_open_indev(ws, udtop->udt_scr.scr_msname,
			    udtop->udt_msfd) == WSINDEV_NULL)
				returnerror(u.u_error);
		}
		/* Set focuses and cursor window */
		ws_set_focus(ws, FF_PICK_CHANGE);
		dtop_set_cursor(dtop);
		break;
		}

	case WINSCREENGET:
		*(struct screen *)data = dtop->dt_screen;
		break;

	case WINSCREENDESTROY: {
		extern int hz;
		int win_check_destroy();
		int wnum;
		Window *w_probe;

		/* Don't let any other processes install on dtop */
		dtop->dt_flags |= DTF_EXITING;
		/* Stop reading input if workstation going away */
		if ((ws->ws_dtop == dtop) && (dtop->dt_next == DESKTOP_NULL))
			ws->ws_flags |= WSF_EXITING;
		/* Tell everyone, other than the caller, to terminate */
		winpsnotkill = u.u_procp->p_pid;
		wt_enumeratechildren(winkill, dtop->dt_rootwin,
		    (struct rect *)0);
		winpsnotkill = 0;
		/* Setup timeout to do wakeup for following wait */
		timeout(win_check_destroy, (caddr_t)dtop, hz);
		/* Wait for all children of root to go away */
		while ((dtop->dt_flags & DTF_PRESENT) &&
		    (dtop->dt_exit_seconds < win_exit_seconds) &&
		    (dtop->dt_rootwin != WINDOW_NULL) &&
		    (dtop->dt_rootwin->w_link[WL_OLDESTCHILD] != WINDOW_NULL) &&
		    (dtop->dt_rootwin->w_link[WL_YOUNGESTCHILD] != WINDOW_NULL)) {
			(void) sleep((caddr_t)&dtop->dt_flags, LOCKPRI);
                        if (dtop->dt_exit_seconds > 300 && print_waiting) {
                              printf("Waiting for tools to terminate...\n");
                              print_waiting = FALSE;
                        } else {
                              if (dtop->dt_exit_seconds > 700 &&
                                            print_still_waiting) {
                                 printf("Still waiting...\n");
                                 print_still_waiting = FALSE;
                              }
                        }
                }
		/* Kill processes that wouldn't go away */
		if (dtop->dt_flags & DTF_PRESENT) {
			/* Look for any window that is connected to dtop */
			for (wnum = 0; wnum < win_nwindows; wnum++) {
				w_probe = winbufs[wnum];
				if ((w_probe != WINDOW_NULL) &&
				    (w_probe->w_flags & WF_OPEN) &&
				    (w_probe->w_desktop == dtop) &&
				    (w_probe->w_pid != u.u_procp->p_pid))
					winsignal(w_probe, SIGKILL);
			}
		}
		/* Calling process is responsible for cleaning self up */
                print_waiting = TRUE;
                print_still_waiting = TRUE;
		break;
		}

	case WINSCREENPOSITIONS: {
		int     *neighbors = (int *)data;
		struct	window *window;

		for (i = 0; i < SCR_POSITIONS; i++) {
			if (neighbors[i] == WIN_NULLLINK) {
				dtop->dt_neighbors[i] = NULL;
				continue;
			}
			if (wt_numbertowindow(neighbors[i], &window) < 0)
				returnerror(EINVAL);
			dtop->dt_neighbors[i] = window->w_desktop;
		}
		break;
		}

	case WINGETSCREENPOSITIONS: {
		int     *neighbors = (int *)data;

		for (i = 0; i < SCR_POSITIONS; i++) {
			if (dtop->dt_neighbors[i] == NULL) {
				neighbors[i] = WIN_NULLLINK;
				continue;
			}
			wt_windowtonumber(dtop->dt_neighbors[i]->dt_rootwin,
			    &neighbors[i]);
		}
		break;
		}

	case WINSETKBD: {
		struct usrdesktop *udtop = (struct usrdesktop *)data;

		/* Open kbd input device if not already open */
		if ((udtop->udt_scr.scr_kbdname[0] != NULL) &&
		    (ws_indev_match_name(udtop->udt_scr.scr_kbdname,
		    (Wsindev **)0) == WORKSTATION_NULL)) {
			if (ws_open_indev(ws, udtop->udt_scr.scr_kbdname,
			    udtop->udt_kbdfd) == WSINDEV_NULL)
				returnerror(u.u_error);
		}
		break;
		}

	case WINSETMS: {
		struct usrdesktop *udtop = (struct usrdesktop *)data;

		/* Open ms input device if not already open */
		if ((udtop->udt_scr.scr_msname[0] != NULL) &&
		    (ws_indev_match_name(udtop->udt_scr.scr_msname,
		    (Wsindev **)0) == WORKSTATION_NULL)) {
			if (ws_open_indev(ws, udtop->udt_scr.scr_msname,
			    udtop->udt_msfd) == WSINDEV_NULL)
				returnerror(u.u_error);
		}
		break;
		}

	case WINSETINPUTDEV: {
		struct input_device *ind = (struct input_device *)data;
		Wsindev *indev;

		if (ind->id == -1) {
			/* See if given device in open */
			if ((ind->name[0] != NULL) &&
			    (ws_indev_match_name(ind->name, &indev) !=
			    WORKSTATION_NULL)) {
				/* Close given device */
				(void) ws_close_indev(ws, indev);
			}
		} else {
			/* Open input device if not already open */
			if ((ind->name[0] != NULL) &&
			    (ws_indev_match_name(ind->name,
			    (Wsindev **)0) == WORKSTATION_NULL)) {
				if (ws_open_indev(ws, ind->name, ind->id) ==
				    WSINDEV_NULL)
					returnerror(u.u_error);
			}
		}
		break;
		}

	case WINGETINPUTDEV: {
		struct input_device *ind = (struct input_device *)data;
		int	n;
		Wsindev *indev;

		if (ind->id == -1) {
			/* See if given device in open */
			if ((ind->name[0] != NULL) &&
			    (ws_indev_match_name(ind->name, &indev) !=
			    WORKSTATION_NULL)) {
				returnerror (0);
			} else {
				returnerror (ENODEV);
			}
		} else {
			/* Get given numbered device */
			for (indev = ws->ws_indev, n = 0; indev != WSINDEV_NULL;
			    indev = indev->wsid_next, n++) {
				if (n == ind->id) {
					for (i = 0;i < SCR_NAMESIZE; i++)
					    ind->name[i] = indev->wsid_name[i];
					returnerror (0);
				}
			}
			returnerror (ENODEV);
		}
		/* NOTREACHED */
		}

	case WINGETKBDMASK: {
		struct inputmask *im = (struct inputmask *)data;

		*im = w->w_kbdmask;
		break;
		}

	case WINGETPICKMASK: {
		struct inputmask *im = (struct inputmask *)data;

		*im = w->w_pickmask;
		break;
		}

	case WINSETKBDMASK: {
		struct inputmask *im = (struct inputmask *)data;

		w->w_kbdmask = *im;
		w->w_flags |= WF_KBD_MASK_SET;
		break;
		}

	case WINSETPICKMASK: {
		struct inputmask *im = (struct inputmask *)data;

		w->w_pickmask = *im;
		break;
		}

	case WINSETNEXTINPUT: {
		struct	window *windownext;

		if (wt_numbertowindow(*(int *)data, &windownext) < 0)
			returnerror(EINVAL);
		w->w_inputnext = windownext;
		break;
		}

	case WINGETNEXTINPUT:
		wt_windowtonumber(w->w_inputnext, (int *)data);
		break;

	case WINGETBUTTONORDER:
		*(u_int *)data = ws_button_order;
		break;

	case WINSETBUTTONORDER:
		if (*(u_int *)data > 5) {
			returnerror(EINVAL);
		} else {
			ws_button_order = *(u_int *)data;
		}
		break;

	case WINGETSCALING: {
		register Ws_scale	*buffer = (Ws_scale *)data;
		register Ws_scale	*scales = ws_scaling;
		register int		 count;

		for (count = 0; count < WS_SCALE_MAX_COUNT; count++) {
			*buffer++ = *scales;
			if (scales->ceiling == WS_SCALE_MAX_CEILING)
				break;
			scales++;
		}		
		break;
	    }

	case WINSETSCALING: {
		register Ws_scale	*buffer = (Ws_scale *)data;
		register Ws_scale	*scales = ws_scaling;
		register int		 count;

		for (count = 0; count < WS_SCALE_MAX_COUNT; count++) {
			*scales = *buffer++;
			if (scales->ceiling == WS_SCALE_MAX_CEILING)
				break;
			scales++;
		}
		if (count == WS_SCALE_MAX_COUNT) {
			scales->ceiling = WS_SCALE_MAX_CEILING;
			scales->factor = 1;
		}
		break;
	    }


	case WINGETVUIDVALUE: {
		Firm_event *fe = (Firm_event *)data;

		switch (fe->id) {
		case LOC_X_ABSOLUTE:
			fe->value = dtop->dt_ut_x - w->w_screenx;
			break;
		case LOC_Y_ABSOLUTE:
			fe->value = dtop->dt_ut_y - w->w_screeny;
			break;
		default:
			fe->value = vuid_get_value(ws->ws_instate, fe->id);
		}
		break;
		}

	case WINGETFOCUSEVENT: {
		Focus_event *foe = ((Focus_event *)data);

		foe->id = ws->ws_kbd_focus_pt.id;
		foe->value = ws->ws_kbd_focus_pt.value;
		foe->shifts = ws->ws_kbd_focus_pt.shifts;
		break;
		}

	case WINSETFOCUSEVENT: {
		Focus_event *foe = ((Focus_event *)data);

		ws->ws_kbd_focus_pt.id = foe->id;
		ws->ws_kbd_focus_pt.value = foe->value;
		ws->ws_kbd_focus_pt.shifts = foe->shifts;
		break;
		}

	case WINGETSWALLOWEVENT: {
		Focus_event *foe = ((Focus_event *)data);

		foe->id = ws->ws_kbd_focus_sw.id;
		foe->value = ws->ws_kbd_focus_sw.value;
		foe->shifts = ws->ws_kbd_focus_sw.shifts;
		break;
		}

	case WINSETSWALLOWEVENT: {
		Focus_event *foe = ((Focus_event *)data);

		ws->ws_kbd_focus_sw.id = foe->id;
		ws->ws_kbd_focus_sw.value = foe->value;
		ws->ws_kbd_focus_sw.shifts = foe->shifts;
		break;
		}

	case WINSETKBDFOCUS: {
		Window *windowfocus;

		if (wt_numbertowindow(*(int *)data, &windowfocus) < 0)
			returnerror(EINVAL);
		/* Update kbd focus */
		if ((ws->ws_kbdfocus_next != windowfocus) &&
		    ((ws->ws_kbd_focus_sw.id != LOC_WINENTER) ||
		    (ws->ws_kbd_focus_pt.id != LOC_WINENTER))) {
			ws->ws_kbdfocus_next = windowfocus;
			ws->ws_flags |= WSF_SWALLOW_FOCUS_EVENT;
			/*
			 * Send directly unless in middle of focus change
			 * sequence, then will catch the request next
			 * time get to SEND_Q_TOP.
			 */
			if (ws->ws_focus_state == SEND_Q_TOP)
				ws->ws_focus_state = SEND_DIRECT_REQUEST;
		}
		break;
		}

	case WINGETKBDFOCUS: {
		int winnumber;

		wt_windowtonumber(ws->ws_kbdfocus, &winnumber);
		*(int *)data = winnumber;
		break;
		}

	case WINGETEVENTTIMEOUT: {
		struct timeval *tv = (struct timeval *)data;

		*tv = ws->ws_eventtimeout;
		break;
		}

	case WINSETEVENTTIMEOUT: {
		struct timeval *tv = (struct timeval *)data;

		ws->ws_eventtimeout = *tv;
		break;
		}

	case WINUNLOCKEVENT:
		/*
		 * Explicitly check that w is lock holder because some parameter
		 * configurations (ws_eventtimeout == 0,0) cause the event
		 * lock to be very loosely acquired and released.  Thus, we
		 * don't want to accidently release another window's lock.
		 */
		if ((ws->ws_flags & WSF_LOCKED_EVENT) &&
		    (ws->ws_event_consumer == w))
			wlok_forceunlock(&ws->ws_eventlock);
		break;

	case WINREFUSEKBDFOCUS:
		/*
		 * Turning off the WSF_KBD_REQUEST_PENDING flag says to
		 * not give ws_kbdfocus_next the kbd focus.
		 */
		ws->ws_flags &= ~WSF_KBD_REQUEST_PENDING;
		break;

	case WINDBLACCESS:
		/*
		* Turnon the WF_DBL_ACCESS flag.  If this was the first
		* Double bufferer, pick background to be PR_DBL_A (arbitrary)
		*/

		if ((dtop->dt_flags & DTF_DBLBUFFER )  && 
				!(w->w_flags & WF_DBLBUF_ACCESS)) {
			w->w_flags |= WF_DBLBUF_ACCESS;
			if ( dtop->dt_dblcount == 0 ) {
				dtop->dt_curdbl = w;
				dtop->shared_info->go_to_kernel = 1;
				w->w_dbl_rdstate = PW_DBL_BACK;
				w->w_dbl_wrstate = PW_DBL_BACK;
				/* 
				* Increment the count only after 
				* taking down the cursor and redrawing it in
				* the foreground.
				*/
				dtop_changedisplay(dtop, PR_DBL_A, 
					PR_DBL_B, TRUE);
			}
			else dtop->dt_dblcount++;
		}
		break;

	case WINDBLRLSE:
		/*
		 * Release the double buffer and if there is any
		 * other double bufferer choose it.
		 */
		if (( dtop->dt_flags & DTF_DBLBUFFER ) && 
					(w->w_flags & WF_DBLBUF_ACCESS)) {
			dtop_copy_dblbuffer(dtop, w);
			w->w_flags &= ~WF_DBLBUF_ACCESS;
			w->w_dbl_wrstate = PW_DBL_BOTH;
			w->w_dbl_rdstate = PW_DBL_BACK;
			if (--dtop->dt_dblcount == 0) {
				dtop->shared_info->go_to_kernel = 0;
				if (pr_dbl_set(dtop->dt_pixrect,
					       PR_DBL_READ, w->w_dbl_rdstate,
					       PR_DBL_WRITE, w->w_dbl_wrstate,
					       0))
#ifdef WINDEVDEBUG
					printf("Error in WINDRLSE\n");
#else
					;
#endif
				dtop->dt_curdbl = WINDOW_NULL;
			}
			else dtop_choose_dblbuf(dtop);
		}
		break;

	case WINDBLFLIP:
		/*
		 * Flip the display and choose another double bufferer
		 * (dt_curdbl field in the dtop struct) if the cursor is
		 * not in the current bufferer.
		 */
		if (dtop->dt_flags & DTF_DBLBUFFER)
		    dtop_flip_display(dtop, w);
		break;

	case WINDBLSET: {
		/* 
		* Set display, write or read control bits. Converts
		* Values like PW_DBL_FORE, PW_DBL_BACK to PR_DBL_A
		* or PR_DBL_B. (Allowed only when the window is the current
		* double bufferer).  
		*/
		register struct pwset *list = (struct pwset *)data;

		if (!dtop->dt_dblcount)
			break;

		/* Keep track of what the read/write setting is */
		if (list->attribute == PR_DBL_WRITE)
			w->w_dbl_wrstate = list->value;
		else if (list->attribute == PR_DBL_READ)
			w->w_dbl_rdstate = list->value;
		if ((dtop->dt_flags & DTF_DBLBUFFER) && 
		    ((w == dtop->dt_curdbl) || (dtop->dt_displaygrabber == w))){
			int value;

			if (list->value == PW_DBL_BOTH) 
				value = PR_DBL_BOTH;
			else if (list->value == PW_DBL_FORE) 
				value = dtop->dt_dbl_frgnd;
			else if (list->value == PW_DBL_BACK)
				value = dtop->dt_dbl_bkgnd;
			else returnerror(0);
			if (pr_dbl_set(dtop->dt_pixrect, list->attribute,
				 value, 0))
#ifdef WINDEVDEBUG
				printf("error in ioctl SET for dbl_set\n");
#else
				;
#endif
		}
		break;
	}

	case WINDBLGET: {
		register struct pwset *list = (struct pwset *)data;

		if (dtop->dt_flags & DTF_DBLBUFFER ) {
			if (list->attribute == PR_DBL_AVAIL) 
				list->value = TRUE;
			else if (list->attribute == PR_DBL_READ ) 
				list->value = w->w_dbl_rdstate;
			else if (list->attribute == PR_DBL_WRITE)
				list->value = w->w_dbl_wrstate;
			else list->value = PW_DBL_ERROR; 	/* Error ? */
		}
		else list->value = FALSE;
		break;
	}

	case WINDBLCURRENT:{
	 /*
	  *  Returns the rect of the dbl bufferer in relation to the
	  * current pixwin
	  */
	  register struct rect *rectobj = (struct rect *)data;
	  if (dtop->dt_curdbl && dtop == dtop->dt_curdbl->w_desktop){
	    rectobj->r_left = dtop->dt_curdbl->w_screenx - w->w_screenx;
	    rectobj->r_top = dtop->dt_curdbl->w_screeny - w->w_screeny;
	    rectobj->r_width = dtop->dt_curdbl->w_rect.r_width;
	    rectobj->r_height = dtop->dt_curdbl->w_rect.r_height;
	  }
	  break;
	}

	case WINWIDSET:
	  w->w_wid_dbl_info = *(struct fb_wid_dbl_info *) data;
	  break;

	case WINWIDGET:
	  *(struct fb_wid_dbl_info *) data = w->w_wid_dbl_info;
	  break;

	case WINSHAREQUEUE:
		if (dtop->dt_sharedq_owner) {
			returnerror(EADDRINUSE);
		} else if (dtop->shared_info->shared_eventq.size <= 0) {
			returnerror(0);
		} else {
			dtop->dt_sharedq_owner = w;
		}
		break;
		
	case WINCLEARCURSORPLANES:
	{
	    register struct rect *rectobj = (struct rect *)data;
	    
	    if ((dtop->dt_plane_groups_available[PIXPG_CURSOR])
		&& (dtop->dt_plane_groups_available[PIXPG_CURSOR_ENABLE])) {
		win_clear_cursor_plane_groups(w, rectobj);
	    }
	    break;
	 }   
	/*
	 * Record/playback utilities
	 */
#ifdef WINSVJ
	case WINSETRECQUE:
	case WINSETRECORD:
	case WINREADRECQ:
	case WINSETPLAYBACK:
	case WINSETPLAYINTR:
	case WINSETSYNCPT:
		error = svjioctl(dev, (int) cmd, data, dtop, ws, pri, error);
		win_playrec_sync_flg = 0;
		break;
#endif

	default:
		returnerror(ENOTTY); /* Note: ?? */
	}
done:
	if (always_signal) {
		/*
		 * Clear go_to_kernel field.
		 */
		if (winnotify_inorder) {
			if (dtop && dtop->shared_info)
				dtop->shared_info->go_to_kernel = 0;
			wt_sigwnch_nextwin();	/* notify next window */
		}
	}
#ifdef WINSVJ
	if (((ws != NULL) && (ws->ws_flags&WSF_RECORD_QUEUE)) && 
	    (win_playrec_sync_flg))
		(void) svjioctl(dev, (int) cmd, data, dtop, ws, pri, error);
#endif
	(void) splx(pri);
	return (error);
restart:
	u.u_eosys = RESTARTSYS;
	returnerror(EINTR);
}

static	void
win_return_cms(w, cms)
	register Window *w;
	struct colormapseg *cms;
{
	*cms = w->w_cms;
	/*
	 * Adjust addr if window is in the overlay plane.
	 * Note: This is a hack to get around the fact that
	 * we haven't provided a separate colormap allocation
	 * mechanism for each plane group.  We are allocating
	 * from a single colormap instead.  If this situation
	 * is fixed in a future release then the following
	 * code could be removed.
	 */
	if (w->w_plane_group == PIXPG_OVERLAY ||
	    (w->w_desktop->dt_flags & DTF_MULTI_PLANE_GROUPS &&
	    !(w->w_flags & WF_PLANE_GROUP_SET)))
		cms->cms_addr = 0;
}

static	void
win_pick_plane_group(dtop, w)
	Desktop *dtop;
	Window *w;
{
	if (dtop->dt_pixrect) {
		if (!(w->w_flags & WF_PLANE_GROUP_SET)) {
			w->w_plane_group = pr_get_plane_group(dtop->dt_pixrect);
		}
	}
}

/*ARGSUSED*/
wt_sizechanged(window, rectnotused)
	register struct	window	*window;
	struct	rect	*rectnotused;
{
	register struct	window *winparent = window->w_link[WL_PARENT];
	struct	rect rect;

	/*
	 * Blanket windows should stay the same size as their parents.
	 */
	if ((window->w_userflags & WUF_BLANKET) && (winparent)) {
		/*
		 * Construct rect of what blanket window should be.
		 */
		rect_construct(&rect, 0, 0,
		    winparent->w_rect.r_width,
		    winparent->w_rect.r_height);
		/*
		 * Change only if different.
		 */
		if (!rect_equal(&rect, &window->w_rect)) {
			window->w_rect = rect;
			window->w_flags |= WF_SIZECHANGED;
		}
	}
}

wincopyoutrl(rl, winclip)
	register struct	rectlist *rl;
	register struct	winclip *winclip;
{
	int	bytesneeded;
	register struct rectnode *rn;
	struct	rectlist rluser;
	caddr_t	ptuser;

	bytesneeded = sizeof (struct rectlist);
	for (rn = rl->rl_head; rn; rn = rn->rn_next)
		bytesneeded += sizeof (struct rectnode);
	if (bytesneeded > winclip->wc_blockbytes) {
		winclip->wc_blockbytes = bytesneeded;
		return (EFBIG);
	}
	rluser = *rl;
	ptuser = (caddr_t)(winclip->wc_block + sizeof (struct rectlist));
	if (rl->rl_head != NULL)
		rluser.rl_head = (struct rectnode *)ptuser;
	for (rn = rl->rl_head; rn; rn = rn->rn_next) {
		struct	rectnode rnuser;

		rnuser = *rn;
		if (rn->rn_next)
			rnuser.rn_next = (struct rectnode *)
			    (ptuser + sizeof (struct rectnode));
		if (copyout((char *)&rnuser, ptuser, sizeof (struct rectnode)))
			return (EFAULT);
		if (rn == rl->rl_tail)
			rluser.rl_tail = (struct rectnode *)ptuser;
		ptuser += sizeof (struct rectnode);
	}
	if (copyout((caddr_t)&rluser, (caddr_t)winclip->wc_block,
	    sizeof (struct rectlist)))
		return (EFAULT);
	return (0);
}


static int
check_crosshair_info(cursor)
	register struct cursor	*cursor;
{
	if ((cursor->horiz_hair_thickness < 0) ||
	    (cursor->horiz_hair_thickness > CURSOR_MAX_HAIR_THICKNESS))
		return (EINVAL);

	if ((cursor->vert_hair_thickness < 0) ||
	    (cursor->vert_hair_thickness > CURSOR_MAX_HAIR_THICKNESS))
		return (EINVAL);

	return (0);
}


winio_getusercursor(cursor, mpr, mpr_data, image, empty)
	register struct cursor *cursor;
	register struct pixrect *mpr;
	struct	mpr_data *mpr_data;
	short	*image;
	int	*empty;
{
	int	error, imagesize;

	*empty = 1;
	if (!cursor->cur_shape)
		return (0);
	if (error = copyin((caddr_t)cursor->cur_shape,
	    (caddr_t)mpr, sizeof(struct pixrect)))
		return (error);
	if (!mpr->pr_data)
		return (0);
	if (error = copyin((caddr_t)mpr->pr_data,
	   (caddr_t)mpr_data, sizeof(struct mpr_data)))
		return (error);
	if (!mpr_data->md_image)
		return (0);
        imagesize = mpr_data->md_linebytes * mpr->pr_height;
        if (imagesize > CUR_MAXIMAGEBYTES)
                return (E2BIG);
        bzero((caddr_t) image, CUR_MAXIMAGEBYTES);
	if (error = copyin((caddr_t)mpr_data->md_image,
	    (caddr_t)image, (u_int) imagesize))
		return (error);
	*empty = 0;
	return (0);
}

winioctl_cmapcopy(cmsto, cmapto, offsetto, cmsfrom, cmapfrom, offsetfrom, func)
	register struct	colormapseg *cmsto, *cmsfrom;
	register struct	cms_map *cmapto, *cmapfrom;
	int	(*func)(), offsetto, offsetfrom;
{
	register int	firstto = cmsto->cms_addr+offsetto;
	register int	firstfrom = cmsfrom->cms_addr+offsetfrom;
	register int	n;
	int	lastto = firstto+cmsto->cms_size-1;
	int	lastfrom = firstfrom+cmsfrom->cms_size-1;

	/*
	 * Test to see that source and destination data is consistent with self.
	 */
	if (firstto > lastto || firstto < 0 ||
	    firstfrom > lastfrom || firstfrom < 0 ||
	    !cmapto->cm_red || !cmapto->cm_green || !cmapto->cm_blue ||
	    !cmapfrom->cm_red || !cmapfrom->cm_green || !cmapfrom->cm_blue)
		return (EINVAL);
	/*
	 * Limit range so that destination not overrun.
	 */
	n = ((firstto+cmsfrom->cms_size-1) > lastto) ?
	    lastto-firstto+1 : cmsfrom->cms_size; 
	/*
	 * Move data.
	 */
	if ((*func)(&(cmapfrom->cm_red[firstfrom]),
		&(cmapto->cm_red[firstto]), n) ||
	    (*func)(&(cmapfrom->cm_green[firstfrom]),
		&(cmapto->cm_green[firstto]), n) ||
	    (*func)(&(cmapfrom->cm_blue[firstfrom]),
		&(cmapto->cm_blue[firstto]), n))
		return (EFAULT);
	return (0);
}

win_check_destroy(data)
	caddr_t data;
{
	register Desktop *dtop = (Desktop *)data;
	extern int hz;

	/* Wakeup destroy even if desktop not present */
	wakeup((caddr_t)&dtop->dt_flags);
	if (dtop->dt_flags & DTF_PRESENT) {
		dtop->dt_exit_seconds++;
		timeout(win_check_destroy, (caddr_t)dtop, hz);
	}
}

static int
wt_gettreelayer(parent, data)
	register Window		*parent;
	register Win_tree_layer	*data;
{
	register Window		*w;
	register Win_enum_node	*nodep;
	register int		 size;
	int			 installed, uninstalled;
	int			 w_num;

	if (parent == WINDOW_NULL)
		return EINVAL;
	wt_count_layer(parent, &installed, &uninstalled);
	size = (installed + uninstalled) * sizeof (struct win_enum_node);
	if (data->bytecount < size) {
		data->bytecount -= size;
		return EFBIG;
	} else
		data->bytecount = size;
	nodep = data->buffer;
	w = parent->w_link[WL_OLDESTCHILD];
	while (w != WINDOW_NULL) {
		wt_copy_layer_node(w, nodep++);
		w = w->w_link[WL_YOUNGERSIB];
	}
	for (w_num = 0; w = winfromopendev(w_num); w_num++) {
		if (w->w_link[WL_PARENT]  == parent  &&
		    !(w->w_flags & WF_INSTALLED))
			wt_copy_layer_node(w, nodep++);
	}
	return 0;
}

static void
wt_copy_layer_node(w, nodep)
	register Window		*w;
	register Win_enum_node	*nodep;
{
	int			 w_num;
	Win_enum_node		 node;

	wt_windowtonumber(w, &w_num);
	node.me = w_num;
	wt_windowtonumber(w->w_link[WL_PARENT], &w_num);
	node.parent = w_num;
	wt_windowtonumber(w->w_link[WL_YOUNGERSIB], &w_num);
	node.upper_sib = w_num;
	wt_windowtonumber(w->w_link[WL_OLDESTCHILD], &w_num);
	node.lowest_kid = w_num;
	if (w->w_flags & WF_INSTALLED)
		node.flags = WIN_NODE_INSERTED;
	else
		node.flags = 0;
	if (w->w_userflags & WUF_WMGR1) {	/*  iconic  */
		node.icon_rect = w->w_rect;
		node.open_rect = w->w_rectsaved;
	} else {				/*  open  */
		node.flags |= WIN_NODE_OPEN;
		node.open_rect = w->w_rect;
		node.icon_rect = w->w_rectsaved;
	}
	(void) copyout((caddr_t)&node, (caddr_t)nodep,
		sizeof (struct win_enum_node));
}

static void
wt_count_layer(parent, in, out)
    register Window *parent;
    register int   *in, *out;
{
    /* register */ int    i;
    register Window *w;

    *in = 0;
    *out = 0;
    for (i = 0; w = winfromopendev(i); i++) {
	if (w->w_link[WL_PARENT] == parent) {
	    if (w->w_flags & WF_INSTALLED) {
		*in += 1;
	    } else {
		*out += 1;
	    }
	}
    }
}

static	void
win_prepare_surface(w)
	register Window *w;
{
	register Desktop *dtop = w->w_desktop;

	/* See if anything to do */
	if (rl_empty(&w->w_rlfixup))
		/* Assumes no side affects from calling this routine */
		return;
	/* Dont take the cursor down if there are cursor planes */
	if ((dtop->dt_cursorwin == w) && 
	    (!dtop->dt_plane_groups_available[PIXPG_CURSOR]))
		dtop_cursordown(dtop);
	if (dtop->dt_plane_groups_available[PIXPG_OVERLAY_ENABLE]) {
	    /* NOTE: ADD HERE AS NEW ENABLE PLANES BECOME AVAILABLE */
		/* Enable black and white overlay enable plane */
		win_initialize_plane_group(w, PIXPG_OVERLAY_ENABLE, 1);
#ifndef PRE_FLAMINGO
		/* Disable video if available */
		if (dtop->dt_plane_groups_available[PIXPG_VIDEO_ENABLE])
			win_initialize_plane_group(w, PIXPG_VIDEO_ENABLE, 0);
#endif
		/* Clear color plane group (for tidiness) */
		if (dtop->dt_plane_groups_available[PIXPG_8BIT_COLOR])
			win_initialize_plane_group(w, PIXPG_8BIT_COLOR, 0);
		else if (dtop->dt_plane_groups_available[PIXPG_24BIT_COLOR])
			win_initialize_plane_group(w, PIXPG_24BIT_COLOR, 0);
		
	}
	if (dtop->dt_plane_groups_available[PIXPG_CURSOR_ENABLE]) {	
	    win_initialize_plane_group(w, PIXPG_CURSOR_ENABLE, 0);
	    /* Color planes for tidines*/
	    if (dtop->dt_plane_groups_available[PIXPG_8BIT_COLOR])
			win_initialize_plane_group(w, PIXPG_8BIT_COLOR, 0);
		else if (dtop->dt_plane_groups_available[PIXPG_24BIT_COLOR])
			win_initialize_plane_group(w, PIXPG_24BIT_COLOR, 0);
	}	
	if ((dtop->dt_cursorwin == w) && 
		(!(dtop->dt_plane_groups_available[PIXPG_CURSOR])))
		dtop_cursordown(dtop);
}

static void
win_initialize_plane_group(w, plane_group, color)
	Window *w;
	int plane_group;
	int color;
{ 
	int plane_group_save;
	register struct	pixrect *pr = w->w_desktop->dt_pixrect;

	/* Remember original plane group (the planes should be full) */
	plane_group_save = pr_get_plane_group(pr);
	/* Set plane group */
	pr_set_plane_group(pr, plane_group);
	/* Set planes to enable writing offset, effectively clears */
	(void) dtop_rl_rop(pr, -w->w_screenx, -w->w_screeny, &w->w_rlfixup,
	    PIX_COLOR(color)|PIX_SRC|PIX_DONTCLIP, (struct pixrect *)0, 0, 0);
	/* Reset plane group */
	pr_set_plane_group(pr, plane_group_save);
}

static void
win_clear_cursor_plane_groups(w, rect)
	Window *w;
	struct rect *rect;
{ 
	int plane_group_save;
	register struct	pixrect *pr = w->w_desktop->dt_pixrect;

	/* Remember original plane group (the planes should be full) */
	plane_group_save = pr_get_plane_group(pr);

	pr_set_plane_group(pr, PIXPG_CURSOR_ENABLE);
	(void) pr_rop(pr, rect->r_left, rect->r_top,rect->r_width, 
	    rect->r_height, PIX_CLR|PIX_DONTCLIP, (struct pixrect *)0, 0, 0);

	pr_set_plane_group(pr, PIXPG_CURSOR);
	(void) pr_rop(pr, rect->r_left, rect->r_top,rect->r_width, 
	    rect->r_height, PIX_CLR|PIX_DONTCLIP, (struct pixrect *)0, 0, 0);

	/* Reset plane group */
	pr_set_plane_group(pr, plane_group_save);
}
