#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)pw_read.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Pw_read.c: Implement the read function of pw_rop of the pixwin.h interface.
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/cms.h>
#include <sunwindow/pixwin.h>
#include <sunwindow/pw_util.h>

pwo_read(pr, xr, yr, width, height, op, pw, xw, yw)
	register struct	pixwin *pw;
	register int	xw, yw, width, height;
	int	op, xr, yr;
	struct	pixrect *pr;
{
	struct	rect rintersect, rsrc;

	(void)rl_free(&pw->pw_fixup);
	/*
	 * Read from retained pixrect if have one.
	 */
	if (pw->pw_prretained) {
		(void)pr_rop(pr, xr, yr, width, height, op, pw->pw_prretained,
		    PW_RETAIN_X_OFFSET(pw, xw), PW_RETAIN_Y_OFFSET(pw, yw));
		return;
	}
	/*
	 * Construct window relative rectangle that will be reading from
	 */
	rect_construct(&rsrc, xw, yw, width, height);
	rsrc.r_left = PW_X_FROM_WIN(pw, rsrc.r_left);
	rsrc.r_top = PW_Y_FROM_WIN(pw, rsrc.r_top);
	(void)pw_lock(pw, &rsrc);
	rsrc.r_left = PW_X_TO_WIN(pw, rsrc.r_left);
	rsrc.r_top = PW_Y_TO_WIN(pw, rsrc.r_top);
	/*
	 * Don't clip if op flag is on
	 */
	if (op & PIX_DONTCLIP) {
		(void)pr_rop(pr, xr, yr, width, height, op,
		    pw->pw_clipdata->pwcd_prmulti, xw, yw); 
		goto Unlock;
	}
	/*
	 * Loop and clip
	 */
	pw_begincliploop(pw, &rsrc, &rintersect);
		(void)pr_rop(pr, xr+(rintersect.r_left-rsrc.r_left),
		    yr+(rintersect.r_top-rsrc.r_top),
		    rintersect.r_width, rintersect.r_height,
		    op, pw->pw_clipdata->pwcd_prmulti,
		    rintersect.r_left, rintersect.r_top);
	pw_endcliploop();
Unlock:
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
	PW_FIXUP_TRANSLATE(pw);
	return;
}

