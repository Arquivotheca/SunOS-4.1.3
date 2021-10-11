#ifndef lint
static	char sccsid[] = "@(#)wintree.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * SunWindows window tree primitives.
 */

#include <sunwindowdev/wintree.h>
#include <sunwindow/win_ioctl.h>
#include <sys/vmmeter.h>
#include <sys/kmem_alloc.h>

#define	MAX_RNSDAMAGED	20

static void	wt_bump_clippingid();


wt_install(win)
	register struct window *win;
{
	register struct window *parent;
	struct window *youngest;

	if (win->w_flags & WF_INSTALLED)
		return (EBUSY);
	if (wt_badchildren(win))
		return (EINVAL);
	parent = win->w_link[WL_PARENT];
	if (parent == NULL)
		return (EINVAL);
TryAgain:
	if (parent->w_link[WL_OLDESTCHILD] == NULL &&
	    parent->w_link[WL_YOUNGESTCHILD] == NULL) {
		if (win->w_link[WL_OLDERSIB] || win->w_link[WL_YOUNGERSIB]) {
#ifdef	HARSH_CHECKING
			return (EINVAL);
#else	HARSH_CHECKING
			/* Adjust windows siblings */
			win->w_link[WL_OLDERSIB] = win->w_link[WL_YOUNGERSIB] =
			    NULL;
#endif	HARSH_CHECKING
		}
		parent->w_link[WL_OLDESTCHILD] =
		    parent->w_link[WL_YOUNGESTCHILD] = win;
	} else if (win->w_link[WL_OLDERSIB] == NULL) {
		win->w_link[WL_YOUNGERSIB] = parent->w_link[WL_OLDESTCHILD];
		win->w_link[WL_YOUNGERSIB]->w_link[WL_OLDERSIB] = win;
		parent->w_link[WL_OLDESTCHILD] = win;
	} else if (win->w_link[WL_YOUNGERSIB] == NULL) {
            /* 
	    *  check WUF_WMGR4 of top window to see if current
	    *  window can go on top of it.
	    */
	    youngest = parent->w_link[WL_YOUNGESTCHILD];
	    if (youngest->w_userflags & WUF_WMGR4) {
		win->w_link[WL_YOUNGERSIB] = youngest;
		win->w_link[WL_OLDERSIB] = youngest->w_link[WL_OLDERSIB];
		if (youngest->w_link[WL_OLDERSIB])
		    youngest->w_link[WL_OLDERSIB]->w_link[WL_YOUNGERSIB] = win;
		youngest->w_link[WL_OLDERSIB] = win;
	    }
	    else 
	    {
		win->w_link[WL_OLDERSIB] = parent->w_link[WL_YOUNGESTCHILD];
		win->w_link[WL_OLDERSIB]->w_link[WL_YOUNGERSIB] = win;
		parent->w_link[WL_YOUNGESTCHILD] = win;
	    }
	} else {
		if ((win->w_link[WL_OLDERSIB]->w_flags & WF_INSTALLED) == 0 ||
		    (win->w_link[WL_YOUNGERSIB]->w_flags & WF_INSTALLED) == 0 ||
		    win->w_link[WL_YOUNGERSIB]->w_link[WL_OLDERSIB] !=
			win->w_link[WL_OLDERSIB] ||
		    win->w_link[WL_OLDERSIB]->w_link[WL_YOUNGERSIB] !=
			win->w_link[WL_YOUNGERSIB]) {
#ifdef	HARSH_CHECKING
			return (EINVAL);
#else	HARSH_CHECKING
			/* Just place it on the bottom of sibling pile */
			win->w_link[WL_YOUNGERSIB] = win->w_link[WL_OLDERSIB] =
			    NULL;
			goto TryAgain;
#endif	HARSH_CHECKING
		}
		win->w_link[WL_OLDERSIB]->w_link[WL_YOUNGERSIB] = win;
		win->w_link[WL_YOUNGERSIB]->w_link[WL_OLDERSIB] = win;
	}
	win->w_flags |= WF_INSTALLED;
	{
		int wt_initclipping();
		wt_enumeratechildren(wt_initclipping, win, (struct rect *)0);
	}
	return (0);
}

wt_badchildren(win)
	register struct window *win;
{
	register struct window *child;

	if (child = win->w_link[WL_OLDESTCHILD])
		if ((child->w_flags & WF_INSTALLED) == 0 ||
		    child->w_link[WL_PARENT] != win ||
		    child->w_link[WL_OLDERSIB])
			return (-1);
	if (child = win->w_link[WL_YOUNGESTCHILD])
		if ((child->w_flags & WF_INSTALLED) == 0 ||
		    child->w_link[WL_PARENT] != win ||
		    child->w_link[WL_YOUNGERSIB])
			return (-1);
	return (0);
}

wt_remove(win)
	register struct window *win;
{
	register struct window *parent = win->w_link[WL_PARENT];

	if ((win->w_flags & WF_INSTALLED) == 0)
		return (EINVAL);
	win->w_flags &= ~WF_INSTALLED;
	if (parent->w_link[WL_OLDESTCHILD] == win)
		parent->w_link[WL_OLDESTCHILD] = win->w_link[WL_YOUNGERSIB];
	if (parent->w_link[WL_YOUNGESTCHILD] == win)
		parent->w_link[WL_YOUNGESTCHILD] = win->w_link[WL_OLDERSIB];
	if (win->w_link[WL_OLDERSIB])
		win->w_link[WL_OLDERSIB]->w_link[WL_YOUNGERSIB] =
		    win->w_link[WL_YOUNGERSIB];
	if (win->w_link[WL_YOUNGERSIB])
		win->w_link[WL_YOUNGERSIB]->w_link[WL_OLDERSIB] =
		    win->w_link[WL_OLDERSIB];
	{
		int wt_nullclipping();
		wt_enumeratechildren(wt_nullclipping, win, (struct rect *)0);
	}
	return (0);
}

/*
 * Walk the tree (including win) in layering order direction.
 * Note: Currently not called from anywhere, so commented out.
 */
wt_enumerate(function, direction, win)
	int	(*function)();
	int	direction;
	struct	window *win;
{
	if (direction==WT_BOTTOMTOTOP) {
		(*function)(win);
		if (win->WL_BOTTOMCHILD!=NULL)
			wt_enumerate(function, direction, win->WL_BOTTOMCHILD);
		if (win->WL_COVERING!=NULL)
			wt_enumerate(function, direction, win->WL_COVERING);
	} else {
		/*
		 * Assume WT_TOPTOBOTTOM
		 */
		if (win->WL_TOPCHILD!=NULL)
			wt_enumerate(function, direction, win->WL_TOPCHILD);
		(*function)(win);
		if (win->WL_COVERED!=NULL)
			wt_enumerate(function, direction, win->WL_COVERED);
	}
}

/*
 * Enumerate the children of win (including win).
 */
wt_enumeratechildren(function, win, rect)
	int	(*function)();
	struct	window *win;
	struct	rect *rect;
{
	register struct window *wchild;

	if (win)
		(*function)(win, rect);
	else
		return;
	for (wchild = win->w_link[WL_YOUNGESTCHILD]; wchild;
	    wchild = wchild->w_link[WL_OLDERSIB]) {
		if (rect)
			rect_passtochild(wchild->w_rect.r_left,
			    wchild->w_rect.r_top, rect);
		wt_enumeratechildren(function, wchild, rect);
		if (rect)
			rect_passtoparent(wchild->w_rect.r_left,
			    wchild->w_rect.r_top, rect);
	}
}

/* Utilities */

wt_numbertowindow(number, aw)
	int	number;
	struct	window **aw;
{

	if (number == WIN_NULLLINK) {
		*aw = NULL;
		return (0);
	}
	if ((unsigned)number >= win_nwindows) {
		return (-1);
	}
	*aw = winfromdev(number);
	if (!((*aw)->w_flags & WF_OPEN))
		return (-1);
	return (0);
}

wt_windowtonumber(w, anumber)
	register struct window *w;
	register int *anumber;
{
	register int wi;

	*anumber = WIN_NULLLINK;
	if (w == NULL)
		return;
	for (wi = 0; wi < win_nwindows; wi++) {
		/*  Linear search - hope we're not called often! */
		if (w == winbufs[wi]) {
			*anumber = wi;
			break;
		}
	}
}

/*
 * This routine should initially be called when winthis is a root.
 * This routine can't generally be initially be called with winthis at
 * any level of the window tree.  However, a special case allows the
 * first level in the window heirarchy to be used as the initial
 * winthis.
 */
wt_setclipping(winthis, rlaffected)
	register struct	window *winthis;
	register struct	rectlist *rlaffected; /* Self relative */
{
	register struct	window *w;
	register struct rectnode *rn;
	struct	rect rparent, rintersect;
	struct	rectlist rlfix;
	int	rnsdamaged;

	rlfix = rl_null;
	/*
	 * Save the current clipping and init clipping to null.
	 */
	rl_copy(&winthis->w_rlexposed, &winthis->w_rlexposedold);
	rl_free(&winthis->w_rlexposed);
	if (winthis->w_link[WL_PARENT] == NULL) {
		/*
		 * Initialize the root clipping with its entire rectangle
		 */
		rl_initwithrect(&winthis->w_rect, &winthis->w_rlexposed);
		winthis->w_screenx =
		    winthis->w_desktop->dt_screen.scr_rect.r_left;
		winthis->w_screeny =
		    winthis->w_desktop->dt_screen.scr_rect.r_top;
		/*
		 * Initialize the root fixup with entire rlaffected
		 */
		rl_copy(rlaffected, &rlfix);
	} else {
		/*
		 * Make data parent relative
		 */
		rl_passtoparent(winthis->w_rect.r_left, winthis->w_rect.r_top,
		    rlaffected);
		rparent = winthis->w_link[WL_PARENT]->w_rect;
		rparent.r_left = rparent.r_top = 0;
		/*
		 * Initialize the window clipping to intersection of parent
		 * window and self.
		 */
		rect_intersection(&winthis->w_rect, &rparent, &rintersect);
		rl_initwithrect(&rintersect, &winthis->w_rlexposed);
		winthis->w_screenx = winthis->w_link[WL_PARENT]->w_screenx+
		    winthis->w_rect.r_left;
		winthis->w_screeny = winthis->w_link[WL_PARENT]->w_screeny+
		    winthis->w_rect.r_top;
		/*
		 * Remove ansector opaque portions from clipping.
		 * At this point the parent's clipping only includes the
		 * parent's ansector and sibling clipping, no child clipping.
		 */
		if (winthis->w_link[WL_PARENT]->w_link[WL_PARENT] != NULL)
			/*
			 * Don't need to do this computation if one level down.
			 */
			rl_intersection(&winthis->w_rlexposed,
			    &winthis->w_link[WL_PARENT]->w_rlexposed,
			    &winthis->w_rlexposed);
		/*
		 * Remove covering siblings from clipping.
		 */
		for (w = winthis->WL_COVERING; w != NULL; w = w->WL_COVERING) {
			rl_rectdifference(&w->w_rect, &winthis->w_rlexposed,
			    &winthis->w_rlexposed);
		}
		/*
		 * Initialize the window fixup with intersection of initial
		 * clipping and rlaffected.
		 */
		rl_rectintersection(&rintersect, rlaffected, &rlfix);
		/*
		 * Make data self relative
		 */
		rl_passtochild(winthis->w_rect.r_left, winthis->w_rect.r_top,
		    rlaffected);
		rl_passtochild(winthis->w_rect.r_left, winthis->w_rect.r_top,
		    &winthis->w_rlexposed);
		rl_passtochild(winthis->w_rect.r_left, winthis->w_rect.r_top,
		    &rlfix);
	}
	/*
	 * Make sure clipping and fixup have 0 offsets
	 */
	rl_normalize(&winthis->w_rlexposed);
	rl_normalize(&rlfix);
	/*
	 * Allow children to find clipping.
	 * Make rlaffected relative to w before call wt_setclipping.
	 */
	for (w = winthis->WL_BOTTOMCHILD; w != NULL; w = w->WL_COVERING) {
		rl_passtochild(w->w_rect.r_left, w->w_rect.r_top, rlaffected);
		wt_setclipping(w, rlaffected);
		rl_passtoparent(w->w_rect.r_left, w->w_rect.r_top, rlaffected);
	}
	/*
	 * Remove covering children.
	 */
	for (w = winthis->WL_BOTTOMCHILD; w != NULL; w = w->WL_COVERING) {
		rl_rectdifference(&w->w_rect, &winthis->w_rlexposed,
		    &winthis->w_rlexposed);
	}
	rl_coalesce(&winthis->w_rlexposed);
	if (!(winthis->w_flags&WF_DTOPPOSCHANGED)) {
		/*
		 * Remove what was already exposed from what needs to be
		 * repaired.
		 */
		rl_difference(&rlfix, &winthis->w_rlexposedold, &rlfix);
	} else {
		/*
		 * Don't try to optimize because bits on dtop must move.
		 */
	}
	/*
	 * Add what needs to be repaired this time to what was leftover from
	 * previous clipping recomputations.
	 */
	rl_union(&rlfix, &winthis->w_rlfixup, &rlfix);
	/*
	 * Remember if had unfinished fixup in case try to do partial repair.
	 */
	if (!rl_empty(&winthis->w_rlfixup))
		winthis->w_flags |= WF_PREVIOUSDAMAGED;
	else
		winthis->w_flags &= ~WF_PREVIOUSDAMAGED;
	/*
	 * Reduce what needs to be repaired by intersecting with what's exposed.
	 */
	rl_intersection(&rlfix, &winthis->w_rlexposed, &rlfix);
	/*
	 * Simplify rls as much as possible.
	 */
	rl_coalesce(&rlfix);
	/*
	 * If damaged area is "too" complex then replace by total exposed area.
	 * This is mainly to prevent too much damage from accumulating.
	 * Exposed could still have more parts then damaged,
	 * but this is the degenerate case.
	 */
	rnsdamaged = 0;
	for (rn = rlfix.rl_head; rn; rn = rn->rn_next)
		rnsdamaged++;
	if (rnsdamaged >= MAX_RNSDAMAGED) {
		rl_free(&rlfix);
		rl_copy(&winthis->w_rlexposed, &rlfix);
	}
	/*
	 * Bump clippingid if exposed part has changed.
	 */
	if (!rl_equal(&winthis->w_rlexposed, &winthis->w_rlexposedold)) {
		wt_bump_clippingid(winthis);
#ifndef	PRE_FLAMINGO
		if (winthis->w_flags & WF_NOTIFYALLCHANGES)
			winthis->w_flags |= WF_CLIPPINGCHANGED;
#endif
	}
	/*
	 * Bump clippingid if fixup different.
	 * OK to bump this second time.
	 */
	if (!rl_equal(&rlfix, &winthis->w_rlfixup)) {
		rl_free(&winthis->w_rlfixup);
		rl_copy(&rlfix, &winthis->w_rlfixup);
		wt_bump_clippingid(winthis);
	}
	rl_free(&rlfix);
}

wt_partialrepair(window, rectok)
	struct	window	*window;
	struct	rect	*rectok;
{

	if (!(window->w_flags&WF_PREVIOUSDAMAGED)) {
		rl_rectdifference(
		    rectok, &window->w_rlfixup, &window->w_rlfixup);
	}
}

/*ARGSUSED*/
wt_positionchanged(window, rectnotused)
	struct	window	*window;
	struct	rect *rectnotused;
{

	window->w_flags |= WF_DTOPPOSCHANGED;
}

bool
wt_isindisplaytree(window)
	register struct window *window;
{
	register struct window *w, *wparent, *wchild;

	/*
	 * window is in a display tree if the there are full links from window
	 * up the tree to its root.
	 */
	for (w = window; w; w = w->w_link[WL_PARENT]) {
		wparent = w->w_link[WL_PARENT];
		if (wparent==NULL) {
			if (w==window->w_desktop->dt_rootwin) {
				/*
				 * Made it up the correct display tree OK
				 */
				return (TRUE);
			} else
				break;
		}
		/*
		 * w must be among parent's children
		 */
		for (wchild = wparent->w_link[WL_OLDESTCHILD]; wchild;
		    wchild = wchild->w_link[WL_YOUNGERSIB]) {
			if (w==wchild)
				break;
		}
		/*
		 * See if broke out of above loop or didn't find w
		 */
		if (wchild==NULL)
			return (FALSE);
	}
	return (FALSE);
}

wt_passrltoancestor(rl, winfrom, winto)
	struct	rectlist *rl;
	struct	window *winfrom, *winto;
{
	register struct window *w;

	for (w = winfrom; w != winto; w = w->w_link[WL_PARENT]) {
		rl_passtoparent(w->w_rect.r_left, w->w_rect.r_top, rl);
	}

}

struct winnotify wnlist;	/* window notify list */
#ifdef WINDEVDEBUG
int winnotify_debug = 0;
#endif
int winnotify_inorder = 1;
int winnotify_starttimer = 1;

/*ARGSUSED*/
wt_add_to_notifyqueue(w, rectnotused)
	register struct window *w;
	struct	rect *rectnotused;
{
	register struct winnotify *wp;
	register struct winnotify *newwp;

	int s, pos_ch = 0;

	extern wtsigwnchtimeout;
	extern wtsigwnchtimeoutpending;
	extern wt_sigwnch_timeout();

	if (w->w_flags&WF_DTOPPOSCHANGED) {
		wt_bump_clippingid(w);
		if (w->w_flags & WF_NOTIFYALLCHANGES)
			pos_ch = 1;
	}
	if (w->w_flags&WF_SIZECHANGED)
		wt_bump_clippingid(w); /* OK to bump more than once */

	s = spl6();	/* this is probably not needed */

	if ((w->w_flags&WF_SIZECHANGED) || (!rl_empty(&w->w_rlfixup))
							 || (pos_ch)) {
		for (wp = &wnlist; wp->wn_next != NULL; wp = wp->wn_next) {
			if (wp->wn_window &&
			    wp->wn_window->w_pid == w->w_pid) {
#ifdef WINDEVDEBUG
				if (winnotify_debug)
					printf("Ignoring 0x%x\n", w);
#endif
				(void) splx(s);
				return;
			}
		}
		newwp = (struct winnotify *)new_kmem_alloc(
				sizeof (struct winnotify), KMEM_SLEEP);
		newwp->wn_next = NULL;
		newwp->wn_window = w;
#ifdef WINDEVDEBUG
		if (winnotify_debug)
		    printf("Queue for 0x%x after 0x%x\n", w, (wp)->wn_window);
#endif
		(wp)->wn_next = newwp;
		w->w_flags &= ~WF_SIZECHANGED;
	}

	/*
	 * Start the timer if it's not going already.
	 */
	if (winnotify_starttimer && wtsigwnchtimeoutpending == 0) {
		wtsigwnchtimeoutpending++;
		timeout(wt_sigwnch_timeout, (caddr_t)0, wtsigwnchtimeout);
	}
	(void)splx(s);
}

int wtsigwnchtimeoutpending;
int wtsigwnchtimeout = 5;		/* timeout in ticks */
int maxtimeout = 10;			/* do it anyway after 20 ticks */
int totaltimeout;			/* time we've waited so far */
int maxdiskwaiters = 1;			/* max allowable disk waiters */
int wttoomuchiocount;
int wtsigwnchtimeoutcount;
#ifdef WINDEVDEBUG
int wtsigwnchdebug = 0;
#endif

/*ARGSUSED*/
wt_sigwnch_timeout()
{
	wtsigwnchtimeoutpending--;
	wt_sigwnch_nextwin();		/* do next in the queue */
	wtsigwnchtimeoutcount++;

#ifdef WINDEVDEBUG
	if (wtsigwnchdebug) {
		printf("wt_sigwnch_timeout called %d. times\n",
			wtsigwnchtimeoutcount);
	}
#endif
}

wt_sigwnch_untimeout()
{
	register struct winnotify *wp;
	register int s;

	s = spl6();

	if (wtsigwnchtimeoutpending) {
		wtsigwnchtimeoutpending = 0;
		untimeout(wt_sigwnch_timeout, (caddr_t)0);
	}

	while ((wp = wnlist.wn_next) != NULL) {
		wnlist.wn_next = wp->wn_next;	/* remove from the queue */
		kmem_free((caddr_t)wp, sizeof (struct winnotify));
	}

	(void)splx(s);
}

/*ARGSUSED*/
wt_sigwnch_nextwin()
{
	register struct winnotify *wp;
	register int s;
	register struct proc *p;

	int dwaiters;

	extern hz;

	s = spl6();	/* this is probably not needed */

	if ((wp = wnlist.wn_next) != NULL) {
		dwaiters = 0;

		for (p = allproc; p != NULL; p = p->p_nxt) {
			if (p->p_flag & SSYS)
				continue;

			if (p->p_stat == SSLEEP || p->p_stat == SSTOP) {
				if ((p->p_flag & SLOAD) && p->p_pri <= PZERO) {
					if (++dwaiters > maxdiskwaiters)
						break;
				}
			}
		}

		if (dwaiters <= maxdiskwaiters || totaltimeout > maxtimeout) {
		    totaltimeout = 0;
		    wnlist.wn_next = wp->wn_next;	/* remove from queue */
		    wt_notifychanged(wp->wn_window, (struct rect *)NULL);
		    /*
		     * Need to set lock go_to_kernel field.
		     */
		    if (wp->wn_window->w_desktop &&
			wp->wn_window->w_desktop->shared_info)
			wp->wn_window->w_desktop->shared_info->go_to_kernel = 1;

		    kmem_free((caddr_t)wp, sizeof (struct winnotify));
		} else {
		    totaltimeout += wtsigwnchtimeout;
		    wttoomuchiocount++;
#ifdef WINDEVDEBUG
		    if (wtsigwnchdebug)
			printf("wt_sigwnch_nextwin not called.  too much io\n");
#endif
		}
	}
	/*
	 * If there are still requests on the list, then
	 * start a timer so that we'll eventually flush the queue.
	 */
	if (wnlist.wn_next != NULL) {
		if (wtsigwnchtimeoutpending == 0) {
			wtsigwnchtimeoutpending++;
			timeout(wt_sigwnch_timeout, (caddr_t)0, wtsigwnchtimeout);
		}
	} else if (wtsigwnchtimeoutpending) {
		wtsigwnchtimeoutpending = 0;
		untimeout(wt_sigwnch_timeout, (caddr_t)0);
	}
	(void) splx(s);
}

/*ARGSUSED*/
wt_notifychanged(w, rectnotused)
	register struct window *w;
	struct	rect *rectnotused;
{
	if (winnotify_inorder == 0) {
#ifndef PRE_FLAMINGO
		if (w->w_flags&WF_DTOPPOSCHANGED) {
			w->w_flags &= ~WF_DTOPPOSCHANGED;
			if (w->w_flags & WF_NOTIFYALLCHANGES)
				w->w_flags |= WF_CLIPPINGCHANGED;
			wt_bump_clippingid(w);
		}
		if (w->w_flags&WF_SIZECHANGED)
			wt_bump_clippingid(w); /* OK to bump more than once */
			if (w->w_flags & WF_NOTIFYALLCHANGES)
				w->w_flags |= WF_CLIPPINGCHANGED;
		if ((w->w_flags&WF_SIZECHANGED) || !rl_empty(&w->w_rlfixup)) {
			winsignal(w, SIGWINCH);
			w->w_flags &= ~WF_SIZECHANGED;
		}
	} else {
		/* If the client wants to be notified of all changes */
		/* set clipping changed if size or posion has changed */
		if ((w->w_flags & WF_DTOPPOSCHANGED ||
				w->w_flags&WF_SIZECHANGED) &&
		    (w->w_flags & WF_NOTIFYALLCHANGES)) {
			w->w_flags |= WF_CLIPPINGCHANGED;
			wt_bump_clippingid(w); /* OK to bump more than once */
		    }
		winsignal(w, SIGWINCH);
                w->w_flags &=
                        ~(WF_SIZECHANGED|WF_DTOPPOSCHANGED|WF_CLIPPINGCHANGED);
#ifdef WINDEVDEBUG
		if (winnotify_debug)
			printf("Deliver to 0x%x\n", w);
#endif  /* WINDEVDEBUG */

#else	/* !PRE_FLAMINGO */
		if (w->w_flags&WF_DTOPPOSCHANGED) {
			w->w_flags &= ~WF_DTOPPOSCHANGED;
			wt_bump_clippingid(w);
		}
		if (w->w_flags&WF_SIZECHANGED)
			wt_bump_clippingid(w); /* OK to bump more than once */
		if ((w->w_flags&WF_SIZECHANGED) || !rl_empty(&w->w_rlfixup)) {
			winsignal(w, SIGWINCH);
			w->w_flags &= ~WF_SIZECHANGED;
		}
	} else {
		winsignal(w, SIGWINCH);
#ifdef WINDEVDEBUG
		if (winnotify_debug)
			printf("Deliver to 0x%x\n", w);
#endif	/* WINDEVDEBUG */
#endif	/* PRE_FLAMINGO */
	}
}

/*ARGSUSED*/
wt_initclipping(win, rectnotused)
	struct	window *win;
	struct	rect *rectnotused;
{
	/*
	 * This routines handles the case of a window
	 * being removed and then reinstalled into the tree when,
	 * at the time of removal, there was unresolved damage.
	 * At the user level this might be a move followed by a top command.
	 * Removal cleared the damage,
	 * installing generated a new exposed list and damaged list,
	 * wt_partialrepair then clears the damaged list because the
	 * WF_PREVIOUSDAMAGED flag is not on.
	 */
	if (win->w_flags & WF_PREVIOUSDAMAGED) {
		/*
		 * Since don't remember exactly want damage was there when
		 * removed, repaint the entire window.
		 * W_rlfixup will get trimmed to exposed area when set clipping.
		 */
		rl_initwithrect(&win->w_rect, &win->w_rlfixup);
	}
}

/*ARGSUSED*/
wt_nullclipping(win, rectnotused)
	struct	window *win;
	struct	rect *rectnotused;
{

	rl_free(&win->w_rlexposed);
	rl_free(&win->w_rlexposedold);
	rl_free(&win->w_rlfixup);
	/*
	 * So that window wouldn't write to screen when removed,
	 * bump clippingid.
	 */
	wt_bump_clippingid(win);
}

struct window *
wt_intersected(winthis, x, y)
	struct	window *winthis;
	coord	x, y;
{
	register struct window *w, *winintersect;
	struct	rect rect;

	rect = winthis->w_rect;
	rect.r_left = winthis->w_screenx;
	rect.r_top = winthis->w_screeny;
	if (!rect_includespoint(&rect, x, y))
		return (NULL);
	for (w = winthis->WL_TOPCHILD; w; w = w->WL_COVERED)
		if (winintersect = wt_intersected(w, x, y))
			return (winintersect);
	return (winthis);
}

#ifdef WINDEVDEBUG
wt_printwindow(w, tag)
	struct	window *w;
	char	*tag;
{
	printf(tag);
	if (w==NULL)
		return;
	printf("w: %X, Flags: %X ", w, w->w_flags);
	printf("parent: %X \n", w->w_link[WL_PARENT]);
	printf("link[WL_OLDERSIB]: %X ", w->w_link[WL_OLDERSIB]);
	printf("link[WL_YOUNGERSIB]: %X \n", w->w_link[WL_YOUNGERSIB]);
	printf("link[WL_OLDESTCHILD]: %X ", w->w_link[WL_OLDESTCHILD]);
	printf("link[WL_YOUNGESTCHILD]: %X \n", w->w_link[WL_YOUNGESTCHILD]);
	printf("Window Rect: ");
	rect_print(&w->w_rect);
	printf("\n");
	printf("Saved Rect: ");
	rect_print(&w->w_rectsaved);
	printf("\n");
	printf("ScreenOffsets: x=%d y=%d  ClippingID=%d\n",
		w->w_screenx, w->w_screeny, w->w_clippingid);
	rl_print(&w->w_rlexposed, "w_rlexposed");
	rl_print(&w->w_rlexposedold, "w_rlexposedold");
	rl_print(&w->w_rlfixup, "w_rlfixup");
}
#endif


static void
wt_bump_clippingid(w)
	register struct window *w;
{
	int	win_number;

	w->w_clippingid++;
	/*
	 * get the window number.
	 */
	wt_windowtonumber(w, &win_number);
	w->w_desktop->shared_info->clip_ids[win_number] = w->w_clippingid;
}
