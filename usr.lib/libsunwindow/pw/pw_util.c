#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)pw_util.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Pw_util.c: Implement the functions of the pw_util.h interface.
 */

#include <sys/types.h>
#include "pixrect/pixrect.h"
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/cms.h>
#include <sunwindow/pixwin.h>

pw_initfixup(pw, rl_fixup)
	struct	pixwin *pw;
	struct	rectlist *rl_fixup;
{
	struct	rect rect;

	/*
	 * Create a rectlist of the bounding box of the entire window
	 */
	rect_construct(&rect, 0, 0, pw->pw_clipdata->pwcd_prmulti->pr_width,
	    pw->pw_clipdata->pwcd_prmulti->pr_height);
	(void)rl_initwithrect(&rect, rl_fixup);
	/*
	 * Remove currently exposed part of window to yield a list of all the
	 * rects overlapping the window that don't belong to the window
	 */
	(void)rl_difference(rl_fixup, &pw->pw_clipdata->pwcd_clipping, rl_fixup);
	return;
}

