#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)wmgr_rect.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Window mgr move, stretch and refresh.
 */

#include <stdio.h>
#include <errno.h>
#include <sys/file.h>
#include <sunwindow/defaults.h>
#include <sunwindow/sun.h>
#include <sunwindow/window_hs.h>
#include <sunwindow/win_ioctl.h>
#include <suntool/alert.h>
#include <suntool/frame.h>
#include <suntool/wmgr.h>
#include <suntool/fullscreen.h>

#ifdef KEYMAP_DEBUG
#include "../../libsunwindow/win/win_keymap.h"
#else
#include <sunwindow/win_keymap.h>
#endif

#define	WMGR_ICON_GRID_X	4
#define	WMGR_ICON_GRID_Y	4

#define	STRETCH_BORDER	2
#define MIN_TOOL_WIDTH	STRETCH_BORDER*3
#define MIN_TOOL_HEIGHT	STRETCH_BORDER*3

void fullscreen_pw_copy();

extern int		 errno;

extern struct cursor	wmgr_move_cursor, wmgr_moveH_cursor, 
			wmgr_moveV_cursor, wmgr_stretchMID_cursor, 
			wmgr_stretchN_cursor, wmgr_stretchNE_cursor,
			wmgr_stretchE_cursor, wmgr_stretchSE_cursor, 
			wmgr_stretchS_cursor, wmgr_stretchSW_cursor, 
			wmgr_stretchW_cursor, wmgr_stretchNW_cursor;

extern char		*getenv();

extern int		 win_getsavedrect(),
			 win_setsavedrect();

extern int		 win_cursor_planes_available(),
			 win_clear_cursor_planes();

#define	xor_box(r) \
	(void)fullscreen_not_draw_box(fsglobal->fs_pixwin, (r), STRETCH_BORDER);

static WM_Direction	 grasps[3][3] = {
	WM_NW, WM_N,    WM_NE,
	WM_W,  WM_None, WM_E,
	WM_SW, WM_S,    WM_SE	
};

static	struct cursor *stretch_cursors[9] = {
		&wmgr_stretchMID_cursor,
		&wmgr_stretchN_cursor,
		&wmgr_stretchNE_cursor,
		&wmgr_stretchE_cursor,
		&wmgr_stretchSE_cursor,
		&wmgr_stretchS_cursor,
		&wmgr_stretchSW_cursor,
		&wmgr_stretchW_cursor,
		&wmgr_stretchNW_cursor
};

static void	wmgr_attach_cursor(),
		wmgr_constrain_point(),
		wmgr_set_cursor(),
		wmgr_move_box(),
		wmgr_stretch_box();

static int	wmgr_do_placeholders();
static Bool	wmgr_stretch_jump_cursor();

static struct fullscreen
	       *wmgr_grab_screen();

typedef enum { Done, Error, Valid, Stop }
	Event_return;

static Event_return
		 wmgr_get_event();

/*ARGSUSED*/
void
wmgr_changerect(feedbackfd, windowfd, event, move, accelerated)
	int		   feedbackfd,	/* This is IGNORED (remove later) */
			   windowfd;
	struct inputevent *event;
	int		   move, accelerated;
{
	struct rect	   rect, rectoriginal;
	Event		   alert_event;
	struct pr_pos	   old_origin;
	int		   flags = win_getuserflags(windowfd);
extern struct pixfont	  *pw_pfsysopen();
	int		   alert_used = 0;
	int		   result = 0;


	if ((!move) && (flags&WMGR_ICONIC))
		return;			/* Don't stretch if iconic */
	if (accelerated <= 0) {
	        result = (move) ? alert_prompt(
		    (Frame)0,
		    &alert_event,
		    ALERT_MESSAGE_STRINGS,
			"  Press and hold the middle mouse button",
			"near the side or corner you want to move",
			"and hold the button down while dragging",
			"the bounding box to the location you want;",
			"then release the mouse button.",
			"  Otherwise, push \"Cancel\" button.",
		        0,
		    ALERT_BUTTON_NO,			"Cancel",
		    ALERT_TRIGGER,			MS_MIDDLE,
		    ALERT_NO_IMAGE,			1,
		    ALERT_NO_BEEPING,			1,
		    0) :
	          alert_prompt(
		    (Frame)0,
		    &alert_event,
		    ALERT_MESSAGE_STRINGS,
			"  Press and hold the middle mouse button",
			"near the side or corner you want to resize",
			"and hold the button down while adjusting",
			"the bounding box to the shape and size you want;",
			"then release the mouse button.",
			"  Otherwise, push \"Cancel\" button.",
		        0,
		    ALERT_BUTTON_NO,			"Cancel",
		    ALERT_TRIGGER,			MS_MIDDLE,
		    ALERT_NO_IMAGE,			1,
		    ALERT_NO_BEEPING,			1,
		    0);
	        if (result == ALERT_FAILED)
		    return;
	        else alert_used = 1;
	}
	/*
	 * See if user wants to continue operation based on last action
	 */
	if ((alert_used == 1) && (result == ALERT_NO)) {
	    return; /* cancel action */
	}
	if (alert_used) event = &alert_event;
	if ( (event_action(event) != MS_LEFT
		&& event_action(event) != MS_MIDDLE)
		|| ! win_inputposevent(event)) {
	    return;
	}
	(void)win_getrect(windowfd, &rect);
	if (alert_used) {
	    event->ie_locx -= rect.r_left;
	    event->ie_locy -= rect.r_top;
	}
	rectoriginal = rect;
	old_origin.x = rect.r_left;  /*save position of tool in root */
	old_origin.y = rect.r_top;
	rect.r_left = rect.r_top = 0; /* and work in tool coords */
	wmgr_providefeedback(
			     windowfd, &rect, event, move, flags&WMGR_ICONIC,
			     wmgr_compute_grasp(&rect, event, move,
						flags&WMGR_ICONIC, 
						accelerated), (Rect *)0,
			     (int (*)()) 0, 0, (Rectlist *)0);
	rect.r_left += old_origin.x;	/*  restore root coords		*/
	rect.r_top += old_origin.y;
	if (!rect_equal(&rect, &rectoriginal))
		wmgr_completechangerect(windowfd, &rect, &rectoriginal, 0, 0);
}

void
wmgr_completechangerect(
    windowfd, rectnew, rectoriginal, parentprleft, parentprtop)
	int	windowfd;
	struct	rect *rectnew, *rectoriginal;
	int	parentprleft, parentprtop;
{
	struct	pixwin *pixwin;
	struct	rectlist rloriginal, rlnew, rlclip;
	struct	rect rectlock, rectloop;
	struct	rectnode *rn;
#ifdef PRE_IBIS
	int	fullplanes = 255;
#else ndef PRE_IBIS
	int	fullplanes = PIX_ALL_PLANES;
#endif else ndef PRE_IBIS

	rloriginal = rl_null;
	rlnew = rl_null;
	rlclip = rl_null;

	if ((rectnew->r_left == rectoriginal->r_left) &&
	    (rectnew->r_top == rectoriginal->r_top)) {
		/*
		 * Only saving bits if origin moves.
		 * Note: Need introduce concept of "gravity" which is direction
		 * that bits "fall" when the size changes.
		 */
		(void)win_setrect(windowfd, rectnew);
		return;
	}
	(void)win_lockdata(windowfd);
	/*
	 * Get current exposed rl.
	 */
	if (pixwin = pw_open(windowfd)) {
	wmgr_winandchildrenexposed(pixwin, &rloriginal);
	/*
	 * Change rect and recompute clipping.
	 */
	(void)win_setrect(windowfd, rectnew);
	(void)win_computeclipping(windowfd);
	/*
	 * Get new exposed rl.
	 */
	wmgr_winandchildrenexposed(pixwin, &rlnew);
	/*
	 * Intersection of original and new clipping are the only
	 * bits saved.
	 */
	(void)rl_intersection(&rloriginal, &rlnew, &rlclip);
	(void)rl_coalesce(&rlclip);
	if (rect_intersectsrect(rectnew, rectoriginal)) {
		struct	rect maxrect;
		int	testarea, maxarea;

		/*
		 * Only know how to move the biggest piece of the visible image.
		 * Another case could be move all the rects that don't
		 * intersect the src.
		 * Another case could be to see if on top, then move everything.
		 */
		maxarea = 0;
		maxrect = rect_null;
		for (rn = rlclip.rl_head; rn; rn = rn->rn_next) {
			rl_rectoffset(&rlclip, &rn->rn_rect, &rectloop);
			if ((testarea=rectloop.r_width*rectloop.r_height) >
			    maxarea) {
				maxarea = testarea;
				maxrect = rectloop;
			}
		}
		if (rect_isnull(&maxrect))
			goto NothingToMove;
		(void)rl_free(&rlclip);
		(void)rl_initwithrect(&maxrect, &rlclip);
	}
	/*
	 * Lock affected area
	 */
	rectlock = rect_bounding(rectnew, rectoriginal);
	rect_passtochild(rectnew->r_left, rectnew->r_top, &rectlock);
	/*
	 * Need to grabio when violating exposed area of window
	 */
	(void)win_grabio(windowfd);
	(void)pw_lock(pixwin, &rectlock);
	/*
	 * Fixup cms that pixwin was initially created with.
	 * Make pixrect able to access entire depth of screen.
	 */
#ifdef  planes_fully_implemented 
	(void)pr_putattributes(pixwin->pw_pixrect, &fullplanes);
#else
	(void)pw_full_putattributes(pixwin->pw_pixrect, &fullplanes);
#endif  planes_fully_implemented
	/*
	 * Move data
	 */
	for (rn = rlclip.rl_head; rn; rn = rn->rn_next) {
		rl_rectoffset(&rlclip, &rn->rn_rect, &rectloop);
		fullscreen_pw_copy(pixwin,
		    rectloop.r_left+rectnew->r_left+parentprleft,
		    rectloop.r_top+rectnew->r_top+parentprtop,
		    rectloop.r_width, rectloop.r_height, PIX_SRC,
		    pixwin,
		    rectloop.r_left+rectoriginal->r_left+parentprleft,
		    rectloop.r_top+rectoriginal->r_top+parentprtop);
	}
	(void)pw_unlock(pixwin);
	(void)win_releaseio(windowfd);
	/*
	 * Validate bits that preserved.
	 */
	for (rn = rlclip.rl_head; rn; rn = rn->rn_next) {
		rl_rectoffset(&rlclip, &rn->rn_rect, &rectloop);
		(void)win_partialrepair(windowfd, &rectloop);
	}
NothingToMove:
	(void)rl_free(&rlclip);
	(void)rl_free(&rloriginal);
	(void)rl_free(&rlnew);
	(void)pw_close(pixwin);
        }   

	(void)win_unlockdata(windowfd);
}

void
wmgr_refreshwindow(windowfd)
	int	windowfd;
{
	struct	rect rectoriginal, rectdif;
	int	marginchange = -1;

	(void)win_lockdata(windowfd);
	(void)win_getrect(windowfd, &rectoriginal);
	/*
	 * A position change is supposed to
	 * invoke a repaint of the entire window and its children.
	 * So, change position by 1,1 and then back to original position.
	 */
	rectdif = rectoriginal;
	if ((rectoriginal.r_width == 0) || (rectoriginal.r_height == 0))
		marginchange = 1;
	rect_marginadjust(&rectdif, marginchange);
	(void)win_setrect(windowfd, &rectdif);
	(void)win_setrect(windowfd, &rectoriginal);
	if (win_cursor_planes_available(windowfd))	
	    win_clear_cursor_planes(windowfd, &rectoriginal);
	(void)win_unlockdata(windowfd);
	return;
}

/*	The next three routines used to be static,
 *	but are now public for use in tool_bdry.c.
 */

void
wmgr_providefeedback(feedbackfd, r, event, is_move, is_iconic, grasp,
		     bounds_rect, adjust_position, swsp, obstacles)
	int		   feedbackfd;
	register Rect	  *r;
	register Event	  *event;
	int		   is_move, is_iconic;
	WM_Direction	   grasp;
	Rect		  *bounds_rect;
	int		 (*adjust_position)();
	int		   swsp;
	Rectlist	  *obstacles;
{
	register
	Event_return	   status;
	int		   old_x, old_y;
	int		   activebut = event_action(event);
	struct fullscreen *fs;
	struct rect	   bounds;
	struct pr_pos	   offset;
	Rect		   orig_r, old_r;

#ifdef KEYMAP_DEBUG	
	/*	Keymap debugging statement; to be removed later */

	if (keymap_debug_mask & KEYMAP_SHOW_EVENT_STREAM)
		win_keymap_debug_start_blockio();
#endif

	fs = wmgr_grab_screen(feedbackfd);

	orig_r = *r;

	wmgr_set_cursor(fs, r, event, is_move, grasp);

	if (wmgr_stretch_jump_cursor()) {
		offset.x = event->ie_locx - r->r_left;
		offset.y = event->ie_locy - r->r_top;
	} else	{			/* don't move cursor 	 */
		if (is_move)  {
			offset.x = event->ie_locx - r->r_left;
			offset.y = event->ie_locy - r->r_top;
		} else {
			offset.x = offset.y = 0;
		}
	}

	if (bounds_rect)  {
		bounds = *bounds_rect;
	} else {
		bounds = wmgr_set_bounds(r, &fs->fs_screenrect,
					 grasp, is_move, &offset, 
					 MIN_TOOL_WIDTH, MIN_TOOL_HEIGHT);
	}
	old_x = event->ie_locx;
	old_y = event->ie_locy;
	xor_box(r);			/* put the outline up to start */
	status = Valid;
	while (status == Valid) {
		status = wmgr_get_event(feedbackfd, event, activebut);
		if  (status == Stop || status == Error)  {
			break;
		}
		if (event->ie_locx == old_x && event->ie_locy == old_y)  {
			continue;
		}
		old_r = *r;
		if (is_move) {
			wmgr_move_box(r, &bounds,
				event->ie_locx,
				event->ie_locy,
				&offset,is_iconic);
		} else {
			wmgr_stretch_box(grasp, r, &bounds,
				event->ie_locx, event->ie_locy);
		}
		/* see if the new position is ok */
		if (adjust_position)
			(*adjust_position)(obstacles, swsp, &old_r, r);

		if (!rect_equal(r, &old_r)) {
			xor_box(&old_r);   	/*  remove old feedback	*/
			xor_box(r);		/*  display new feedback  */
		}
		old_x = event->ie_locx;
		old_y = event->ie_locy;
	}

	xor_box(r);			/*	take box down at end	*/
	(void)fullscreen_destroy(fs);

#ifdef KEYMAP_DEBUG
	/* Keymap debugging statement; to be removed later */

	if (keymap_debug_mask & KEYMAP_SHOW_EVENT_STREAM)
		win_keymap_debug_stop_blockio();
#endif

	if (status == Stop)
		*r = orig_r;
}

/* grasp_type: 1 & 0= by position; -1= unconstrained; -2= fully constrained */
WM_Direction
wmgr_compute_grasp(r, event, is_move, is_iconic, grasp_type)
	register Rect		*r;
	Event		    	*event;
	int			 is_move, is_iconic,grasp_type;
{
	register int		 x = event->ie_locx,
				 y = event->ie_locy;
	int			 row, col;

	if (is_move && is_iconic) return WM_None;

	if (grasp_type == -1) {
	    if (x < r->r_left + r->r_width/2)
		col = 0;
	    else 
		col = 2;
	    if (y < r->r_top + r->r_height/2)
		row = 0;
	    else
		row = 2;
	} else if (grasp_type != -2) {
	    if (x < r->r_left + r->r_width/3)
		col = 0;
	    else if (x > r->r_left + (r->r_width/3) * 2)
		col = 2;
	    else
		col = 1;
	    if (y < r->r_top + r->r_height/3)
		row = 0;
	    else if (y > r->r_top + (r->r_height/3) * 2)
		row = 2;
	    else
		row = 1;
	}

	if (grasp_type == -2 || (row == 1 && col == 1 && !is_move)) {
		/*  it's in the center; distribute it to a side
		 *  according to which side of each diagonal it lies on.
		 */
		int	ne = 1, nw = 2;

		if (y > 0 && (x<<10) / y < (r->r_width<<10) / r->r_height) {
			ne = 0;		/* below the main diagonal	*/
		}
		if (x + y > (r->r_width + r->r_height) / 2 ) {
			nw = 0;		/* below the minor diagonal	*/
		}
		switch (ne + nw)  {
		  case 0:	return WM_S;	/* !NE && !NW	*/
		  case 1:	return WM_E;	/*  NE && !NW	*/
		  case 2:	return WM_W;	/* !NE &&  NW	*/
		  case 3:	return WM_N;	/*  NE &&  NW	*/
		}
		/*NOTREACHED*/
	} else {
		return grasps[row][col];
	}
}

/*ARGSUSED*/
Rect
wmgr_set_bounds(w_rect, s_rect, grasp, is_move, offset, min_width, min_height)
	register Rect	*w_rect;	/* window rect */
	Rect		*s_rect;	/* screen rect */
	WM_Direction	 grasp;
	int		 is_move;
	struct pr_pos	*offset;
	int		 min_width, min_height;
{
	Rect		 bounds;

	bounds = *s_rect;
	if (is_move) {			/*	move		*/
#ifdef notdef
 /*  This goes back in when menu & paint are fixed to
  *  handle partially off-screen windows correctly.
  *  (Fix wmgr_move_box, below, at the same time.)
  */
		rect_marginadjust(&bounds, -min_width);
#endif
		/* restrict motion to one dimension
		 * if not at corner.
		 */
		switch (grasp) {
			case WM_N:
			case WM_S:
				bounds.r_left = w_rect->r_left;
				bounds.r_width = w_rect->r_width;
				break;

			case WM_W:
			case WM_E:
				bounds.r_top = w_rect->r_top;
				bounds.r_height = w_rect->r_height;
				break;

			default:
				break;
		}
	} else {			/*	stretch		*/
		switch (grasp) {
		  case WM_W:
		  case WM_NW:
		  case WM_SW:
			bounds.r_width = rect_right(w_rect) + 1
					- min_width
					- bounds.r_left;
			break;
		  case WM_E:
		  case WM_NE:
		  case WM_SE:
			bounds.r_width = rect_right(&bounds) + 1
					 - (w_rect->r_left + min_width);
			bounds.r_left = w_rect->r_left + min_width;
			break;
		}
		switch (grasp) {
		  case WM_N:
		  case WM_NE:
		  case WM_NW:
			bounds.r_height = rect_bottom(w_rect) + 1
					 - MIN_TOOL_WIDTH
					 - bounds.r_top;
			break;
		  case WM_S:
		  case WM_SE:
		  case WM_SW:
			bounds.r_height = rect_bottom(&bounds) + 1
					 - (w_rect->r_top + min_height);
			bounds.r_top = w_rect->r_top + min_height;
			break;
		}
	}
	return bounds;
}

int
wmgr_get_placeholders(irect, orect)
	struct rect	*irect, *orect;
{
	return wmgr_do_placeholders(irect, orect,
				    (void (*)())win_getrect, 
				    (void (*)())win_getsavedrect);
}

int
wmgr_set_placeholders(irect, orect)
	struct rect	*irect, *orect;
{
	return wmgr_do_placeholders(irect, orect,
				    (void (*)())win_setrect, 
				    (void (*)())win_setsavedrect);
}

/*ARGSUSED*/
int
wmgr_figureiconrect(rootfd, rect)
	int	rootfd;
	struct	rect *rect;
{
	(void)wmgr_acquire_next_icon_rect(*rect);
}

/*ARGSUSED*/
int
wmgr_figuretoolrect(rootfd, rect)
	int	rootfd;
	struct	rect *rect;
{
	(void)wmgr_acquire_next_tool_rect(*rect);
}


/*
 * Utilities
 */

static struct fullscreen *
wmgr_grab_screen(fd)
	int		    fd;
{
	int		    fd_flags;
	struct inputmask    im;
	struct fullscreen  *fs;

	fs = fullscreen_init(fd);	/*  Get fullscreen access  */

	/*  but reset to use non-blocking io (fullscreen sets to block)  */

	fd_flags = fcntl(fd, F_GETFL, 0);
	if (fcntl(fd, F_SETFL, fd_flags|FNDELAY))  {
		perror("wmgr_grab_screen (fcntl)");
	}

	/* Make sure input mask allows tracking cursor motion and buttons
	 * (fullscreen has cached current mask, so we don't need to again)
	 */

	(void)win_getinputmask(fd, &im, (int *) 0);
	im.im_flags |= IM_NEGEVENT;
	win_setinputcodebit(&im, LOC_MOVEWHILEBUTDOWN);
	win_setinputcodebit(&im, MS_LEFT);
	win_setinputcodebit(&im, MS_MIDDLE);
	win_setinputcodebit(&im, MS_RIGHT);
	win_setinputcodebit(&im, WIN_STOP);
	(void)win_setinputmask(fd, &im, (struct inputmask *)0, WIN_NULLLINK);

	return fs;
}

static void
wmgr_set_cursor(fs, r, event, is_move, grasp)
	struct fullscreen	*fs;
	struct rect		*r;
	struct inputevent	*event;
	int			 is_move;
	WM_Direction		 grasp;
{

	if (is_move) {
		switch (grasp) {
		    case WM_N:
		    case WM_S:
			(void)win_setcursor(fs->fs_windowfd, &wmgr_moveV_cursor);
			break;
		    case WM_E:
		    case WM_W:
			(void)win_setcursor(fs->fs_windowfd, &wmgr_moveH_cursor);
			break;
		    default:
			(void)win_setcursor(fs->fs_windowfd, &wmgr_move_cursor);
		}
	} else {
		wmgr_attach_cursor(fs->fs_windowfd, grasp, r, event);
		(void)win_setcursor(fs->fs_windowfd, stretch_cursors[(int)grasp]);
	}
}

static Event_return
wmgr_get_event(fd, event, activebut)
	int		 fd;
	Event		*event;
{
	for (;;) {
		if (input_readevent(fd, event)==-1) {
			if (errno == EWOULDBLOCK) {
				return Valid;
			} else {
				return Error;
			}
		}
		if (event_action(event) == ACTION_STOP
			|| event_id(event) == WIN_STOP) {
			return Stop;
		}
		if (win_inputnegevent(event)) {
			if (event_action(event) == activebut) {
				return Done;
			}
		}
	}
	/*NOTREACHED*/
}

static void
wmgr_move_box(r, bounds, ie_x, ie_y, offset, is_iconic)
	struct rect	*r, *bounds;
	int		 ie_x, ie_y, is_iconic;
	struct pr_pos	*offset;
{
	register int	 x, y;

	x = ie_x - offset->x;
	y = ie_y - offset->y;
	if (is_iconic) {	/*	preserve icon alignment	*/
		x += WMGR_ICON_GRID_X / 2;
		x -= x % WMGR_ICON_GRID_X;
		y += WMGR_ICON_GRID_Y / 2;
		y -= y % WMGR_ICON_GRID_Y;
	}
#ifdef notdef
/*  restore these when menu & prompt are fixed to paint properly
 *  with the tool's origin off-screen.
 *  (Fix wmgr_set_bounds at that point, too.)
 */
	if (!rect_includespoint(bounds, ie_x, ie_y))  {
		wmgr_constrain_point(bounds, &ie_x, &ie_y, is_iconic);
	}
	r->r_left = x;
	r->r_top = y;
/*  but for now, keep completely in bounds	*/
#endif
	(void)wmgr_constrainrect(r, bounds, x - r->r_left, y - r->r_top);
}

static void
wmgr_stretch_box(grasp, r, bounds, x, y)
	WM_Direction	 grasp;
	int		 x, y;
	register Rect	*r;
	Rect		*bounds;
{
	if (!rect_includespoint(bounds, x, y))  {
		wmgr_constrain_point(bounds, &x, &y, FALSE);
	}
	switch (grasp) {
	  case WM_NW:	r->r_width += r->r_left - x;
			r->r_left = x;
	  case WM_N:	r->r_height += r->r_top - y;
			r->r_top = y;
			break;

	  case WM_NE:	r->r_height += r->r_top - y;
			r->r_top = y;

	  case WM_E:	r->r_width = x - r->r_left;
			break;

	  case WM_SE:	r->r_width = x - r->r_left + 1;
	  case WM_S:	r->r_height = y - r->r_top + 1;
			break;

	  case WM_SW:	r->r_height = y - r->r_top + 1;
	  case WM_W:	r->r_width += r->r_left - x;
			r->r_left = x;
			break;
	}
}

static void
wmgr_attach_cursor(window, grasp, r, event)
	WM_Direction		 grasp;
	register struct rect	*r;
	struct inputevent	*event;
{
	register int		 x = event->ie_locx,
				 y = event->ie_locy;

	if (wmgr_stretch_jump_cursor()) {
		/*  this section jumps the cursor to the edge for a stretch
		 *  the alternative below pulls the edge to the cursor
		 */
		switch (grasp) {
		  case WM_N:
		  case WM_NE:
		  case WM_NW:	y = r->r_top;
				break;
		  case WM_S:
		  case WM_SE:
		  case WM_SW:	y = rect_bottom(r);
				break;
		}
		switch (grasp) {
		  case WM_NW:
		  case WM_W:
		  case WM_SW:	x = r->r_left;
				break;
		  case WM_NE:
		  case WM_E:
		  case WM_SE:	x = rect_right(r);
				break;
		}
		(void)win_setmouseposition(window, x, y);
		event->ie_locx = x;
		event->ie_locy = y;
	} else {
		switch (grasp) {
		  case WM_N:
		  case WM_NE:
		  case WM_NW:	r->r_height = rect_bottom(r) - y + 1;
				r->r_top = y;
				break;
		  case WM_S:
		  case WM_SE:
		  case WM_SW:	r->r_height = y - r->r_top + 1;
				break;
		}
		switch (grasp) {
		  case WM_NW:
		  case WM_W:
		  case WM_SW:	r->r_width = rect_right(r) - x + 1;
				r->r_left  = x;
				break;
		  case WM_NE:
		  case WM_E:
		  case WM_SE:	r->r_width = x - r->r_left + 1;
				break;
		}
	}
}

static Bool
wmgr_stretch_jump_cursor()
{
    Bool defaults_get_boolean();

    return defaults_get_boolean("/SunView/Jump_cursor_on_resize", 
    	(Bool) (LINT_CAST(FALSE)), (int *)NULL);
}

static void
wmgr_constrain_point(bounds, x, y, is_iconic)
	register struct rect	*bounds;
	register int		*x, *y;
{
	register int		 dx, dy;

	if (is_iconic) {
		dx = WMGR_ICON_GRID_X;
		dy = WMGR_ICON_GRID_Y;
	} else {
		dx = 1;
		dy = 1;
	}
	while (*x > rect_right(bounds))
		*x -= dx;
	while (*x < bounds->r_left)
		*x += dx;
	while (*y > rect_bottom(bounds))
		*y -= dy;
	while (*y < bounds->r_top)
		*y += dy;
}

static int
wmgr_do_placeholders(irect, orect, proc1, proc2)
	struct rect	*irect, *orect;
	void		(*proc1)(), (*proc2)();
{
	char		*filename;
	int		 placefd;

	if ((filename = getenv("WMGR_ENV_PLACEHOLDER")) == (char *) NULL) {
		(void)fprintf(stderr, "No placeholder name in environment?\n");
		return 0;
	}
	if ((placefd = open(filename, O_RDONLY, 0)) == -1) {
		perror("Couldn't open window placeholders");
		return 0;
	}
	if  (win_getuserflags(placefd) & WMGR_ICONIC) {
		proc1(placefd, irect);
		proc2(placefd, orect);
	} else {
		proc1(placefd, orect);
		proc2(placefd, irect);
	}
	(void)close(placefd);
	return 1;
}


/*
 *	I'd like to get rid of this, but not until it's cleared out of above
 */

wmgr_constrainrect(rconstrain, rbound, dx, dy)
	register struct rect *rconstrain, *rbound;
	register dx, dy;
{
	rconstrain->r_left += dx;
	rconstrain->r_top += dy;
	/*
	 * Bias algorithm to have too big rconstrain fall off right/bottom.
	 */
	if (rect_right(rconstrain) > rect_right(rbound)) {
		rconstrain->r_left = rbound->r_left + rbound->r_width
					- rconstrain->r_width;
	}
	if (rconstrain->r_left < rbound->r_left) {
		rconstrain->r_left = rbound->r_left;
	}
	if (rect_bottom(rconstrain) > rect_bottom(rbound)) {
		rconstrain->r_top = rbound->r_top + rbound->r_height
					- rconstrain->r_height;
	}
	if (rconstrain->r_top < rbound->r_top) {
		rconstrain->r_top = rbound->r_top;
	}
}

fullscreen_not_draw_box(pixwin, r, w)
	register struct	pixwin *pixwin;
	register struct rect *r;
	register int	w;
{
/*
 * The need for the two pr_ioctl calls, is an ugly kludge but necessary
 * for 8-bit indexed emulation in a 24-bit environment.  The
 * function is a no-op for all other frame buffers.
 */

#include <sys/ioctl.h>	/* FBIO* */
#include <sun/fbio.h>	/* FBIO* */

	(void)pw_lock(pixwin, r);

	pr_ioctl((Pixrect *) pixwin->pw_opshandle, FBIOSAVWINFD, 0);

	/* Draw top, left, right then bottom */
	(void)fullscreen_pw_write(
	    pixwin, r->r_left, r->r_top, r->r_width, w, PIX_NOT(PIX_DST),
	    (struct pixrect *)0, 0, 0);
	(void)fullscreen_pw_write(pixwin, r->r_left, r->r_top + w,
	    w, r->r_height - 2*w, PIX_NOT(PIX_DST), (struct pixrect *)0, 0, 0);
	(void)fullscreen_pw_write(pixwin, r->r_left + r->r_width - w, r->r_top + w,
	    w, r->r_height - 2*w, PIX_NOT(PIX_DST), (struct pixrect *)0, 0, 0);
	(void)fullscreen_pw_write(pixwin, r->r_left, r->r_top + r->r_height - w,
	    r->r_width, w, PIX_NOT(PIX_DST), (struct pixrect *)0, 0, 0);

	pr_ioctl((Pixrect *) pixwin->pw_opshandle, FBIORESWINFD, 0);

	(void)pw_unlock(pixwin);
	return;
}

