#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)pw_copy.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Pw_copy.c: Implement the pw_copy functions of the pixwin.h interface.
 */
#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <pixrect/pixrect.h>
#include <pixrect/memvar.h>
#include <pixrect/pr_planegroups.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/cms.h>
#include <sunwindow/pixwin.h>
#include <sunwindow/pw_util.h>
#include <sunwindow/win_screen.h>
#include <sunwindow/win_input.h>
#include <sunwindow/win_ioctl.h>

/*ARGSUSED*/
pwo_copy(pw, xd, yd, width, height, op, pws, xs, ys)
	register struct	pixwin *pw;
	struct	pixwin *pws/*Ignored*/;
	int	op, width, height;
	register int	xd, yd, xs, ys;
{
	struct	rect raccess, rsrc;
	short left, top;
	int	planes;
	struct	colormapseg cms;
	int	ignore_planes = 0;
	struct	rectlist rl;

	(void)rl_free(&pw->pw_fixup);
	if (pw->pw_prretained) {
		(void)pr_rop(pw->pw_prretained,
		    PW_RETAIN_X_OFFSET(pw, xd), PW_RETAIN_Y_OFFSET(pw, yd),
		    width, height, op, pw->pw_prretained,
		    PW_RETAIN_X_OFFSET(pw, xs), PW_RETAIN_Y_OFFSET(pw, ys));
		rect_construct(&raccess, xd, yd, width, height);
		if (pw->pw_clipdata->pwcd_batch_type == PW_NONE)
			(void)pw_repair(pw, &raccess);
		else
			(void)pw_update_batch(pw, &raccess);
		return;
	} else if (xs != xd && ys != yd) {
		/* Diagonal copy of non-retained pixwin using pw_read() */
		struct pixrect *pr;
		struct rect		r;
		struct rectlist rlprime;
		struct rectnode *rn;

		/* Copy source pixwin into a new memory pixrect */
		pr = mem_create(width, height, pw->pw_pixrect->pr_depth);
		pw_read(pr, 0, 0, width, height, PIX_SRC, pw, xs, ys);
		
		if (rl_equal(&pw->pw_fixup, &rl_null) || pw->pw_clipdata->pwcd_damagedid) {
			/* Null fixup list means nothing is unaccessible or
			 * else damage events caused pw_read to punt and
			 * and set the fixup list to the source rectangle.
			 */
			pw_write(pw, xd, yd, width, height, op, pr, 0, 0);
		} else {
			/* Determine what can be accessed from fixup list */
			rect_construct(&r, xs, ys, width, height);
			rl = rl_null;
			rlprime = rl_null;
			rl_initwithrect(&r, &rlprime);
			/* rl is the source rect minus the fixup list */
			rl_difference(&rlprime, &pw->pw_fixup, &rl);
		
			/* Copy the accessible rectangles */
			if (!rl_equal(&rl, &rl_null))
				for (rn = rl.rl_head; rn; rn = rn->rn_next) {
					int	xRelSrc, yRelSrc;
				
					xRelSrc = rn->rn_rect.r_left - xs;
					yRelSrc = rn->rn_rect.r_top  - ys;
				
					pw_write(pw, xd + xRelSrc, yd + yRelSrc,
						rn->rn_rect.r_width, rn->rn_rect.r_height,
						op,
						pr, xRelSrc, yRelSrc);
				}

			rl_free(&rl);
			rl_free(&rlprime);
		}
		pr_destroy(pr);

		return;
	}
	/*
	 * Standard pixrect access setup
	 */
	left = min(xd, xs);
	top = min(yd, ys);
	PW_SETUP(pw, raccess, No_op, left, top,
		width+(max(xd, xs)-left), height+(max(yd, ys)-top));
	/*
	 * Additional "standard" setup
	 * Make rest of destination data pixrect relative
	 */
	rect_construct(&rsrc, xs, ys, width, height);

	/* Optimize 8 bit copy */
	if (pw->pw_clipdata->pwcd_prmulti->pr_depth == 8) {
		int	fullplanes = PIX_ALL_PLANES;

		(void)pw_getcmsdata(pw, &cms, &planes);
		if (cms.cms_size == 2) {
			struct  winclip winclip;

			/* Don't ignore planes if there is damage */
			rl = rl_null;
			win_getwinclip(pw->pw_windowfd, 1, &winclip, &rl);
			if (rl_empty(&rl)) {
				(void)pr_putattributes(pw->pw_clipdata->pwcd_prmulti,
				    &fullplanes);
				ignore_planes = 1;
			} else
				(void)rl_free(&rl);
		}
	}
	if (xs!=xd)
		_pw_copyhorizontal(pw, xs, xd, &pw->pw_fixup, &rsrc, op);
	if (ys!=yd)
		_pw_copyvertical(pw, ys, yd, &pw->pw_fixup, &rsrc, op);
	/* Undo optimization of 8 bit copy */
	if (ignore_planes)
		(void)pr_putattributes(pw->pw_clipdata->pwcd_prmulti, &planes);
	/*
	 * Normalize fixup cause _pw_copy* changed offsets
	 */
	(void)rl_normalize(&pw->pw_fixup);
	/*
	 * The intersection of rsrc (now equal to the destination) and the
	 * list of rectangles that couldn't be moved will give another list
	 * of rectangles that need to be refreshed.
	 */
	(void)rl_rectintersection(&rsrc, &pw->pw_fixup, &pw->pw_fixup);
	/*
	 * Further reduce the list to be refreshed by intersecting it with
	 * the exposed portions of the window.
	 */
	(void)rl_intersection(&pw->pw_fixup, &pw->pw_clipdata->pwcd_clipping,
	    &pw->pw_fixup);
	/*
	 * Make data in pixwin window relative again
	 */
	(void)pw_unlock(pw);
	/*
	 * Copy depends on original screen image being correct.
	 * Otherwise, copy will move around garbage bits that haven't been
	 * repaired yet.  Most user programs wouldn't detect this.
	 * There is no convenient way to update a current damaged list
	 * (different from fixup list) with the knowledge of the copy.
	 * If the damagedid in not 0 then we
	 * simply make the fixup list all of the bits moved.
	 */
	if (pw->pw_clipdata->pwcd_damagedid) {
		(void)rl_free(&pw->pw_fixup);
		(void)rl_initwithrect(&rsrc, &pw->pw_fixup);
	}
No_op:	/* Change fix up to client coordinate space */
	PW_FIXUP_TRANSLATE(pw);
	return;
}

_pw_copyvertical(pw, ys, yd, rl_fixup, rsrc, op)
	register struct	pixwin *pw;
	register int	ys, yd;
	struct	rectlist *rl_fixup;
	struct	rect *rsrc;
	int	op;
{
	struct	rectlist rl_saved;

	/*
	 * Make pw->pw_clipping a sorted list and initialize rl_fixup
	 */
	if (ys < yd)
		(void)_pw_sortedrl(pw, RECTS_BOTTOMTOTOP, &rl_saved);
	if (ys > yd)
		(void)_pw_sortedrl(pw, RECTS_TOPTOBOTTOM, &rl_saved);
	if (rl_equal(rl_fixup, &rl_null))
		pw_initfixup(pw, rl_fixup);
	/*
	 * Copy data
	 */
	_pw_copyloop(pw, rsrc, 0, ys-yd, op);
	/*
	 * Reflect new position of bits in source
	 */
	rsrc->r_top -= ys-yd;
	/*
	 * Shift description of those bits that couldn't move.
	 */
	rl_fixup->rl_y -= ys-yd;
	/*
	 * Restore original clipping
	 */
	pw->pw_clipdata->pwcd_clipping = rl_saved;
}

_pw_copyhorizontal(pw, xs, xd, rl_fixup, rsrc, op) 
	register struct	pixwin *pw;
	register int	xs, xd;
	struct	rectlist *rl_fixup;
	struct	rect *rsrc;
	int	op;
{
	struct	rectlist rl_saved;

	/*
	 * Make pw->pw_clipping a sorted list and initialize rl_fixup
	 */
	if (xs < xd)
		(void)_pw_sortedrl(pw, RECTS_RIGHTTOLEFT, &rl_saved);
	if (xs > xd)
		(void)_pw_sortedrl(pw, RECTS_LEFTTORIGHT, &rl_saved);
	pw_initfixup(pw, rl_fixup);
	/*
	 * Copy data
	 */
	_pw_copyloop(pw, rsrc, xs-xd, 0, op);
	/*
	 * Reflect new position of bits in source
	 */
	rsrc->r_left -= xs-xd;
	/*
	 * Shift description of those bits that couldn't move.
	 */
	rl_fixup->rl_x -= xs-xd;
	/*
	 * Restore original clipping
	 */
	pw->pw_clipdata->pwcd_clipping = rl_saved;
}

_pw_copyloop(pw, rsrc, deltax, deltay, op)
	register struct	pixwin *pw;
	register struct	rect *rsrc;
	short	deltax, deltay;
	int	op;
{
	struct	rect rsrcinter, rdst, rdstinter;

	/*
	 * Loop on clipping to determine what src you can move.
	 */
	pw_begincliploop(pw, rsrc, &rsrcinter);
		rdst = rsrcinter;
		rdst.r_left -= deltax;
		rdst.r_top -= deltay;
		/*
		 * Loop on clipping to determine what dest you can move into.
		 */
		pw_begincliploop2(pw, &rdst, &rdstinter);
			/*
			 * Copy src to dst.
			 */
			(void)pr_rop(pw->pw_clipdata->pwcd_prmulti,
			    rdstinter.r_left, rdstinter.r_top,
			    rdstinter.r_width, rdstinter.r_height,
			    op, pw->pw_clipdata->pwcd_prmulti,
			    rsrcinter.r_left+(rdstinter.r_left-rdst.r_left),
			    rsrcinter.r_top+(rdstinter.r_top-rdst.r_top));
		pw_endcliploop();
	pw_endcliploop();
	return;
}

_pw_sortedrl(pw, direction, rl_saved)
	register struct	pixwin *pw;
	int	direction;
	struct	rectlist *rl_saved;
{
	/*
	 * create pw_pw_clippingsorted[direction] if doesn't exist
	 */
	if (rl_empty(&pw->pw_clipdata->pwcd_clippingsorted[direction]))
		(void)rl_sort(&pw->pw_clipdata->pwcd_clipping,
		    &pw->pw_clipdata->pwcd_clippingsorted[direction],
		    direction);
	/*
	 * Save original clipping and make sorted clipping the current clipping
	 */
	*rl_saved = pw->pw_clipdata->pwcd_clipping;
	pw->pw_clipdata->pwcd_clipping =
		pw->pw_clipdata->pwcd_clippingsorted[direction];
	return(direction);
}

