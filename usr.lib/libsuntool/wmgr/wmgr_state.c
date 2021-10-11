#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)wmgr_state.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Window mgr open/close and top/bottom.
 */

#include <stdio.h>
#include <errno.h>
#include <sys/file.h>
#include <suntool/tool_hs.h>
#include <sunwindow/win_ioctl.h>
#include <suntool/fullscreen.h>
#include <suntool/wmgr.h>

static	int wmgr_delayed_state_change;

/*
 * Hack: Call if have called win_release_event_lock() before doing a
 * close.  See usage/comments on wmgr_delayed_state_change below.
 */
/* ARGSUSED */
wmgr_background_close(windowfd)
	int		 windowfd;
{
	wmgr_delayed_state_change = 1;
}

static void
wmgr_swap_rects(windowfd, rect, savedrect)
	register int	 windowfd;
	struct rect	*rect, *savedrect;
{
	(void)win_getrect(windowfd, savedrect);
	(void)win_getsavedrect(windowfd, rect);
	(void)win_setsavedrect(windowfd, savedrect);
	(void)win_setrect(windowfd, rect);
}

static void
wmgr_explode(pixwin, rectto, rectfrom, op)
	register struct	pixwin *pixwin;
	register struct	rect *rectto, *rectfrom;
	register int	op;
{
	(void)fullscreen_pw_vector(pixwin, rectfrom->r_left, rectfrom->r_top,
	    rectto->r_left, rectto->r_top, op, 0);
	(void)fullscreen_pw_vector(pixwin, rect_right(rectfrom), rectfrom->r_top,
	    rect_right(rectto), rectto->r_top, op, 0);
	(void)fullscreen_pw_vector(pixwin, rectfrom->r_left, rect_bottom(rectfrom),
	    rectto->r_left, rect_bottom(rectto), op, 0);
	(void)fullscreen_pw_vector(pixwin,
	    rect_right(rectfrom), rect_bottom(rectfrom),
	    rect_right(rectto), rect_bottom(rectto), op, 0);
}

void
wmgr_changestate(windowfd, rootfd, close)
	register int	windowfd;
	int	rootfd;
	int	close;
{
	struct	rect rect, savedrect;
	register int	flags;

	(void)win_lockdata(windowfd);
	/*
	 * if in desired state then return
	 */
	flags = win_getuserflags(windowfd);
	if ((close && flags&WMGR_ICONIC) || (!close && (~flags)&WMGR_ICONIC))
		goto Unlock;
	if (close)  {
		(void)wmgr_set_icon_rect(rootfd, windowfd, (struct rect *)NULL);
		wmgr_swap_rects(windowfd, &rect, &savedrect);
	} else {
		wmgr_swap_rects(windowfd, &rect, &savedrect);
		(void)wmgr_set_tool_rect(rootfd, windowfd, (struct rect *)NULL);
	}
	/*
	 * Hack to avoid problem of input being sucked up by the root
	 * window process during fullscreen access after having released
	 * event lock (see win_release_event_lock(windowfd).
	 */
	if (!wmgr_delayed_state_change) {
		int delay;
		register struct	fullscreen *fs;

		/*
		 * Takeover screen
		 */
		fs = fullscreen_init(rootfd);
		/*
		 * Provide feedback as to how rect changed.
		 */
		wmgr_explode(fs->fs_pixwin, &rect, &savedrect,PIX_NOT(PIX_DST));
		for (delay = 0;delay < 30000;delay++) {}
		wmgr_explode(fs->fs_pixwin, &rect, &savedrect,PIX_NOT(PIX_DST));
		/*
		 * Release screen
		 */
		(void)fullscreen_destroy(fs);
	}
	wmgr_delayed_state_change = 0;
	/*
	 * Toggle iconic flags.
	 */
	(void)win_setuserflags(windowfd, flags ^= WMGR_ICONIC);
Unlock:
	(void)win_unlockdata(windowfd);
}

int
wmgr_iswindowopen(windowfd)
	int	windowfd;
{
	int	flags = win_getuserflags(windowfd);

	if (!(flags&WMGR_ICONIC)) {
		return(TRUE);
	} else {
		return(FALSE);
	}
}

void
wmgr_changelevelonly(windowfd, parentfd, top)
	register int	windowfd;
	int	parentfd;
	int	top;
{
	register struct	pixwin *pixwin;
	struct	rectlist rloriginal, rlnew;
	struct	rect rectvalidate, rectnew;
	rloriginal = rl_null;
	rlnew = rl_null;

	(void)win_lockdata(windowfd);
	/*
	 * Get current exposed rl.
	 */
	if((pixwin = pw_open(windowfd))) {
		wmgr_winandchildrenexposed(pixwin, &rloriginal);
		/*
		 * Change level and compute new clipping.
		 */
		wmgr_changelevel(windowfd, parentfd, top);
		(void)win_computeclipping(windowfd);
		/*
		 * Get new exposed rl.
		 */
		wmgr_winandchildrenexposed(pixwin, &rlnew);
		/*
		 * Intersection of original and new clipping are the only bits saved.
		 */
		(void)rl_intersection(
		    &rloriginal, &rlnew, &pixwin->pw_clipdata->pwcd_clipping);
		/*
		 * Validate areas that can preserve (dealing in windowfd coordinate
		 * space).
		 */
		(void)win_getsize(windowfd, &rectnew);
		pw_begincliploop(pixwin, &rectnew, &rectvalidate);
			(void)win_partialrepair(windowfd, &rectvalidate);
		pw_endcliploop();
		/*
		 * Cleanup
		 */
		(void)rl_free(&rloriginal);
		(void)rl_free(&rlnew);
		(void)pw_close(pixwin);
	}
	(void)win_unlockdata(windowfd);
}

void
wmgr_winandchildrenexposed(pixwin, rl)
	register struct	pixwin *pixwin;
	register struct	rectlist *rl;
{
	int 	windowfd;
	int	childnumber;
	register int	childfd;
	char	childname[WIN_NAMESIZE];
	struct	rect childrect;
	struct	rectlist rltemp;
	rltemp = rl_null;

	if (!pixwin)
	    return;

	windowfd = pixwin->pw_clipdata->pwcd_windowfd;

	/*
	 * Find union of rl and windowfds exposed rectlist.
	 */
	(void)pw_exposed(pixwin);
	(void)rl_union(&pixwin->pw_clipdata->pwcd_clipping, rl, &rltemp);
	(void)rl_copy(&rltemp, rl);
	(void)rl_free(&rltemp);
	/*
	 * Recursively add child's exposed rectlist to rl.
	 */
	childnumber = win_getlink(windowfd, WL_OLDESTCHILD);
	while (childnumber!=WIN_NULLLINK) {
		(void)win_numbertoname(childnumber, childname);
		/*
		 * Open child window
		 */
		if ((childfd = open(childname, O_RDONLY, 0)) < 0) {
			(void)fprintf(stderr,
				"%s (child) would not open\n", childname);
			perror("wmgr");
			return;
		}
		pixwin->pw_clipdata->pwcd_windowfd = childfd;
		(void)win_getrect(childfd, &childrect);
		rl_passtochild(childrect.r_left, childrect.r_top, rl);
		wmgr_winandchildrenexposed(pixwin, rl);
		rl_passtoparent(childrect.r_left, childrect.r_top, rl);
		childnumber = win_getlink(childfd, WL_YOUNGERSIB);
		(void)close(childfd);
	}
	/*
	 * If completely exposed this will simplify rl.
	 */
	(void)rl_coalesce(rl);
	pixwin->pw_clipdata->pwcd_windowfd = windowfd;
}

void
wmgr_changelevel(windowfd, parentfd, top)
	register int	windowfd;
	int	parentfd;
	int	top;
{
	int	topchildnumber, bottomchildnumber;

	(void)win_lockdata(windowfd);
	/*
	 * Don't try to optimize by not doing anything if already at
	 * desired level.  Doing so messes up the fixup list because
	 * callers of this routine do partial repair which incorrectly
	 * removes some stuff from the damage list if a
	 * win_remove/win_insert pair hasn't been done.
	 */
	/*
	 * Remove from tree
	 */
	(void)win_remove(windowfd);
	/*
	 * Set new links
	 */
	if (top) {
		topchildnumber = win_getlink(parentfd, WL_TOPCHILD);
		(void)win_setlink(windowfd, WL_COVERED, topchildnumber);
		(void)win_setlink(windowfd, WL_COVERING, WIN_NULLLINK);
	} else {
		bottomchildnumber = win_getlink(parentfd, WL_BOTTOMCHILD);
		(void)win_setlink(windowfd, WL_COVERING, bottomchildnumber);
		(void)win_setlink(windowfd, WL_COVERED, WIN_NULLLINK);
	}
	/*
	 * Insert into tree
	 */
	(void)win_insert(windowfd);
	(void)win_unlockdata(windowfd);
	return;
}

void
wmgr_setlevel(window, prev, next)
	register int	window;
	int	prev, next;	/* window numbers (links) to neighbors	*/
{
	(void)win_fdtonumber(window);
	(void)win_remove(window);
	(void)win_setlink(window, WL_COVERED, prev);
	(void)win_setlink(window, WL_COVERING, next);
	(void)win_insert(window);
}


/*
 * Toggle back and forth between full height and normal open size.
 * Opens an iconic tool and brings an open tool to the top.
 */
void
wmgr_full(tool, rootfd)

	register Tool 	*tool; 
	int	rootfd;
{
	Rect	oldopenrect, fullrect;
	int	iconic = tool_is_iconic(tool),
		full = tool->tl_flags&TOOL_FULL;
			
	if (iconic) (void)win_getsavedrect(tool->tl_windowfd, &oldopenrect);
	else (void)win_getrect(tool->tl_windowfd, &oldopenrect);
	(void)win_getrect(rootfd, &fullrect);
	fullrect.r_left = oldopenrect.r_left;
	fullrect.r_width = oldopenrect.r_width;
	(void)win_lockdata(tool->tl_windowfd);
	
	if (iconic) {
		if (full) {	/* unzoom and open */
			tool->tl_flags &= ~TOOL_FULL; 	 /* turn off zoom */
			tool->tl_flags &= ~TOOL_LASTPRIV;/* turn off fullscreen */
			(void)win_setsavedrect(tool->tl_windowfd, &tool->tl_openrect);
			(void)tool_layoutsubwindows(tool);
			wmgr_open(tool->tl_windowfd,rootfd);
		} else {	/* make full and open */
			tool->tl_flags |= TOOL_FULL;
			tool->tl_openrect = oldopenrect;
			(void)win_setsavedrect(tool->tl_windowfd, &fullrect);
			(void)tool_layoutsubwindows(tool);
			wmgr_open(tool->tl_windowfd,rootfd);
		}
	} else if (full) {	/* already full, return to normal */
		tool->tl_flags &= ~TOOL_FULL;		/* turn off zoom */
		tool->tl_flags &= ~TOOL_LASTPRIV;	/* turn off fullscreen */
		wmgr_top(tool->tl_windowfd,rootfd);		
		(void)win_setrect(tool->tl_windowfd, &tool->tl_openrect);
		(void)tool_layoutsubwindows(tool);
	} else  {			/* make full and top */
		tool->tl_flags |= TOOL_FULL;
		tool->tl_openrect = oldopenrect;
		wmgr_top(tool->tl_windowfd,rootfd);		
		(void)win_setrect(tool->tl_windowfd, &fullrect);
		(void)tool_layoutsubwindows(tool);
	}
			
	(void)win_unlockdata(tool->tl_windowfd);
}
