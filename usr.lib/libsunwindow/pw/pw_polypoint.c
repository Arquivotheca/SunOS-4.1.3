#ifndef lint
#ifdef sccs
static char sccsid[] = "@(#)pw_polypoint.c 1.1 92/07/30";
#endif
#endif

/*
 * Copyright 1986 by Sun Microsystems, Inc.
 */
 

#include <sys/types.h>
#include <pixrect/pr_line.h>
#include <pixrect/pixrect.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/cms.h>
#include <sunwindow/pixwin.h>
#include <sunwindow/pw_util.h>


pw_polypoint(pw, dx, dy, npts, ptlist, op)
    register int dx, dy, npts, op;
    register Pixwin		*pw;
    register struct pr_pos	*ptlist;
{
    register struct pixwin_prlist 	*prl;
    struct rect 		rdest;

/*
 * Adjust non-straight-rop operation's offset because didn't go
 * thru macro yet.
 */
    dx = PW_X_OFFSET(pw, dx);
    dy = PW_Y_OFFSET(pw, dy);
/* 
 * Do std setup where we are locking a region that is twice the screen size,
 * since the pixwin can have it's origin to the left of the screen, and be
 * bigger than the screen.
 */
    PW_SETUP(pw, rdest, DoDraw, 0, 0,
	pw->pw_pixrect->pr_size.x<<1, pw->pw_pixrect->pr_size.y<<1);
/*
 * See if user wants to bypass clipping.
 */
    if (op & PIX_DONTCLIP) {
	pr_polypoint(pw->pw_clipdata->pwcd_prmulti, dx, dy,
	    npts, ptlist, op);
	goto TryRetained;
    }
/*
 * Loop and clip
 */
    for (prl = pw->pw_clipdata->pwcd_prl;prl;prl = prl->prl_next) {
	pr_polypoint(prl->prl_pixrect, dx - prl->prl_x, dy - prl->prl_y,
	    npts, ptlist, op);
    }
TryRetained:
/*
 * Unlock screen
 */
    (void)pw_unlock(pw);
/*
 * Write to retained pixrect if have one.
 */
DoDraw:
    if (pw->pw_prretained) {
	pr_polypoint(pw->pw_prretained, 
	    dx + PW_RETAIN_X_OFFSET(pw, 0), 
	    dy + PW_RETAIN_Y_OFFSET(pw, 0),
	    npts, ptlist, op);
    }
    return;
} /* pw_polypoint */

