#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)pw_put.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Pw_put.c: Implement the pw_put functions of the pixwin.h interface.
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/cms.h>
#include <sunwindow/pixwin.h>
#include <sunwindow/pw_util.h>

pwo_put(pw, x0, y0, cms_index)
	register struct	pixwin *pw;
	register int	x0, y0, cms_index;
{
	struct	rect rintersect, rdest;

	PW_SETUP(pw, rdest, DoDraw, x0, y0, 1, 1);
	pw_begincliploop(pw, &rdest, &rintersect);
		if (rect_includespoint(&rintersect, x0, y0)) {
		    (void)pr_put(pw->pw_clipdata->pwcd_prmulti, x0, y0, cms_index);
		    break;
		}
	pw_endcliploop();
	(void)pw_unlock(pw);
DoDraw:
	if (pw->pw_prretained) {
		(void)pr_put(pw->pw_prretained, PW_RETAIN_X_OFFSET(pw, x0),
		    PW_RETAIN_Y_OFFSET(pw, y0), cms_index);
	}
	return;
}

