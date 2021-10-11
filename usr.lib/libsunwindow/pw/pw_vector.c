#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)pw_vector.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Pw_vector.c: Implement the pw_vector functions
 *	of the pixwin.h interface.
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/cms.h>
#include <sunwindow/pixwin.h>
#include <sunwindow/pw_util.h>

pwo_vector(pw, x0, y0, x1, y1, op, cms_index)
	register struct	pixwin *pw;
	int	op;
	register int	x0, y0, x1, y1;
	int	cms_index;
{
	register struct	pixwin_prlist *prl;
	struct	rect rdest;
	short left, top;

	/* Do std setup */
	left = min(x0, x1);
	top = min(y0, y1);
	PW_SETUP(pw, rdest, DoDraw, left, top,
	    max(x0, x1)+1-left, max(y0, y1)+1-top);
	/*
	 * See if user wants to bypass clipping.
	 */
	if (op & PIX_DONTCLIP) {
		(void)pr_vector(pw->pw_clipdata->pwcd_prmulti,
		    x0, y0, x1, y1, op ,cms_index);
		goto TryRetained;
	}
	/*
	 * Loop and clip
	 */
	for (prl = pw->pw_clipdata->pwcd_prl;prl;prl = prl->prl_next) {
		(void)pr_vector(prl->prl_pixrect,
		    x0-prl->prl_x, y0-prl->prl_y, x1-prl->prl_x, y1-prl->prl_y,
		    op, cms_index);
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
		(void)pr_vector(pw->pw_prretained,
		    PW_RETAIN_X_OFFSET(pw, x0), PW_RETAIN_Y_OFFSET(pw, y0),
		    PW_RETAIN_X_OFFSET(pw, x1), PW_RETAIN_Y_OFFSET(pw, y1),
		    op, cms_index);
	}
	return;
}

