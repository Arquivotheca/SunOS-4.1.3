#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)pw_batchrop.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Pw_batchrop.c
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pixfont.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/cms.h>
#include <sunwindow/pixwin.h>
#include <sunwindow/pw_util.h>

pwo_batchrop(pw, xbasew, ybasew, op, items, n)
	register struct	pixwin *pw;
	int	op, xbasew, ybasew;
	struct	pr_prpos items[];
	int n;
{
	struct	rect rdest;
	register struct	pixwin_prlist *prl;
	register struct	pr_prpos *item;
	register int i, dx, dy, x_right, x_left;
	int y_top, y_bottom;

	if (n == 0)
		return;
	/* Initialize bounding box finders */
	dx = x_left = x_right = xbasew;
	dy = y_top = y_bottom = ybasew;
	item = items;
	for (i = 0; i < n; i++) {
		/* Update advances */
		dx += item->pos.x;
		dy += item->pos.y;
		/* Find bounding box of operation */
		x_right = (x_right < dx + item->pr->pr_width)?
		    dx + item->pr->pr_width: x_right;
		y_bottom = (y_bottom < dy + item->pr->pr_height)?
		    dy + item->pr->pr_height: y_bottom;
		x_left = (x_left > dx)? dx: x_left;
		y_top = (y_top > dy)? dy: y_top;
		/* Go to next item */
		item++;
	}
	PW_SETUP(pw, rdest, DoDraw, x_left, y_top, x_right - x_left,
	    y_bottom - y_top);
	if (op & PIX_DONTCLIP) {
		(void)pr_batchrop(pw->pw_clipdata->pwcd_prmulti,
		    xbasew, ybasew, op, items, n);
		goto TryRetained;
	}
	for (prl = pw->pw_clipdata->pwcd_prl;prl;prl = prl->prl_next) {
		(void)pr_batchrop(prl->prl_pixrect,
		    xbasew-prl->prl_x, ybasew-prl->prl_y,
		    op&(~PIX_DONTCLIP), items, n);
	}
TryRetained:
	(void)pw_unlock(pw);
DoDraw:
	if (pw->pw_prretained) {
		(void)pr_batchrop(pw->pw_prretained, PW_RETAIN_X_OFFSET(pw, xbasew),
		    PW_RETAIN_Y_OFFSET(pw, ybasew), op, items, n);
	}
	return;
}
