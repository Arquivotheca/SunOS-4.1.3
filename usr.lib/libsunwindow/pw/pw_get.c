#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)pw_get.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Pw_get.c: Implement the pw_get functions of the pixwin.h interface.
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/cms.h>
#include <sunwindow/pixwin.h>
#include <sunwindow/pw_util.h>

int
pwo_get(pw, x0, y0)
	register struct pixwin *pw;
	register int x0, y0;
{
	struct	rect rintersect, rsrc;
	int	val;

	(void)rl_free(&pw->pw_fixup);
	if (pw->pw_prretained) {
		return(pr_get(pw->pw_prretained, PW_RETAIN_X_OFFSET(pw, x0),
		    PW_RETAIN_Y_OFFSET(pw, y0)));
	}
	rect_construct(&rsrc, x0, y0, 1, 1);
	rsrc.r_left = PW_X_FROM_WIN(pw, rsrc.r_left);
	rsrc.r_top = PW_Y_FROM_WIN(pw, rsrc.r_top);
	(void)pw_lock(pw, &rsrc);
	rsrc.r_left = PW_X_TO_WIN(pw, rsrc.r_left);
	rsrc.r_top = PW_Y_TO_WIN(pw, rsrc.r_top);
	pw_begincliploop(pw, &rsrc, &rintersect);
		if (rect_includespoint(&rintersect, x0, y0)) {
		    val = pr_get(pw->pw_clipdata->pwcd_prmulti, x0, y0);
		    break;
		}
	pw_endcliploop();
	(void)pw_unlock(pw);
	/*
	 * Figure fixup list
	 */
	if (pw->pw_clipdata->pwcd_damagedid)
		/*
		 * Simply make the fixup list all of the bits read.
		 * (Uncommon case.  See pw_copy for explanation).
		 */
		(void)rl_initwithrect(&rsrc, &pw->pw_fixup);
	else
		/*
		 * Make fixup all unaccessible bits of window.
		 */
		pw_initfixup(pw, &pw->pw_fixup);
	/*
	 * The intersection of rsrc and the list of rectangles
	 * that couldn't be read will give another list
	 * of rectangles whose contents need to be recreated.
	 */
	(void)rl_rectintersection(&rsrc, &pw->pw_fixup, &pw->pw_fixup);
	return(val);
}

