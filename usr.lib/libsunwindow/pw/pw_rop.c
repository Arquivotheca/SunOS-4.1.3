#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)pw_rop.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Pw_rop.c: Implement the pw_write functions of the pixwin.h interface.
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_util.h>
#include <pixrect/memvar.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/cms.h>
#include <sunwindow/pixwin.h>
#include <sunwindow/pw_util.h>

static	pw_roptouse;
#define	PW_USEROP	0
#define	PW_USEREPLROP	1
#define	PW_USESTENCIL	2
static	struct	pixrect *pw_stencilpr;
static	int	pw_stencilx, pw_stencily;

/*
 * Internal rop loop used to always set PIX_DONTCLIP.
 * Unfortunately, the client may want his source clipped.
 * and always setting PIX_DONTCLIP prevents this.
 * This was pulled and things started breaking.
 * Since it was the last minute befor the 1.1 release,
 * I backed out from the change but left this global that
 * can be toggled to get source clipping.
 */
/* 
 * initialization relocated to _data
 */
extern	int	pw_dontclipflag; 

pwo_rop(pw, xw, yw, width, height, op, pr, xr, yr)
	struct	pixwin *pw;
	int	op, xw, yw, width, height;
	struct	pixrect *pr;
	int	xr, yr;
{
	struct	rect rintersect, rdest;
	extern	struct pixrectops mem_ops;

	if ((caddr_t)pw == (caddr_t)pr) {
		pwo_copy(pw, xw, yw, width, height, op,
		    (struct pixwin *)pr, xr, yr);
		return;
	} else
	if (pw->pw_ops == &mem_ops) {
		pwo_read((struct pixrect *)pw, xw, yw, width, height, op,
		    (struct pixwin *)pr, xr, yr);
		return;
	}
	/*
	 * Adjust non-straight-rop operation's offset because didn't go
	 * thru macro yet.
	 */
	if (pw_roptouse == PW_USEREPLROP) {
		xw = PW_X_OFFSET(pw, xw);
		yw = PW_Y_OFFSET(pw, yw);
	}
	/* Do standard setup */
	PW_SETUP(pw, rdest, DoDraw, xw, yw, width, height);
	/*
	 * Don't clip if op flag is on
	 */
	if (op & PIX_DONTCLIP) {
		switch (pw_roptouse) {
		case PW_USEROP:
			(void)pr_rop(pw->pw_clipdata->pwcd_prmulti,
			    xw, yw, width, height, op, pr, xr, yr);
			break;
		case PW_USEREPLROP:
			(void)pr_replrop(pw->pw_clipdata->pwcd_prmulti,
			    xw, yw, width, height, op, pr, xr, yr);
			break;
		case PW_USESTENCIL:
			(void)pr_stencil(pw->pw_clipdata->pwcd_prmulti,
			    xw, yw, width, height, op, pw_stencilpr,
			    pw_stencilx, pw_stencily, pr, xr, yr);
			break;
		}
		goto TryRetained;
	}
	/*
	 * Loop and clip
	 */
	pw_begincliploop(pw, &rdest, &rintersect);
		/*
		 * Write to rintersect portion of pixrect.
		 * All coordinates are relative to the window.
		 * Note: Don't or PIX_DONTCLIP into op because might need
		 * to clip source too.
		 */
		switch (pw_roptouse) {
		case PW_USEROP:
			(void)pr_rop(pw->pw_clipdata->pwcd_prmulti,
			    rintersect.r_left, rintersect.r_top,
			    rintersect.r_width, rintersect.r_height,
			    op|pw_dontclipflag, pr,
			    xr+(rintersect.r_left-rdest.r_left),
			    yr+(rintersect.r_top-rdest.r_top));
			break;
		case PW_USEREPLROP:
			(void)pr_replrop(pw->pw_clipdata->pwcd_prmulti,
			    rintersect.r_left, rintersect.r_top,
			    rintersect.r_width, rintersect.r_height,
			    op|pw_dontclipflag, pr,
			    xr+(rintersect.r_left-rdest.r_left),
			    yr+(rintersect.r_top-rdest.r_top));
			break;
		case PW_USESTENCIL:
			(void)pr_stencil(pw->pw_clipdata->pwcd_prmulti,
			    rintersect.r_left, rintersect.r_top,
			    rintersect.r_width, rintersect.r_height,
			    op|pw_dontclipflag, pw_stencilpr,
			    pw_stencilx+(rintersect.r_left-rdest.r_left),
			    pw_stencily+(rintersect.r_top-rdest.r_top),
			    pr,
			    xr+(rintersect.r_left-rdest.r_left),
			    yr+(rintersect.r_top-rdest.r_top));
			break;
		}
	/*
	 * Terminate clipping loop.
	 */
	pw_endcliploop();
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
		xw = PW_RETAIN_X_OFFSET(pw, xw);
		yw = PW_RETAIN_Y_OFFSET(pw, yw);
		switch (pw_roptouse) {
		case PW_USEROP:
			(void)pr_rop(pw->pw_prretained,
			    xw, yw, width, height, op, pr, xr, yr);
			break;
		case PW_USEREPLROP:
			(void)pr_replrop(pw->pw_prretained,
			    xw, yw, width, height, op, pr, xr, yr);
			break;
		case PW_USESTENCIL:
			(void)pr_stencil(pw->pw_prretained,
			    xw, yw, width, height, op, pw_stencilpr,
			    pw_stencilx, pw_stencily, pr, xr, yr);
			break;
		}
	}
	return;
}

pwo_stencil(pw, xw, yw, width, height, op, stpr, stx, sty, spr, sx, sy)
	struct	pixwin *pw;
	int	op, xw, yw, width, height;
	struct	pixrect *stpr;
	int	stx, sty;
	struct	pixrect *spr;
	int	sx, sy;
{
	pw_roptouse = PW_USESTENCIL;
	pw_stencilpr = stpr;
	pw_stencilx = stx;
	pw_stencily = sty;
	pwo_rop(pw, xw, yw, width, height, op, spr, sx, sy);
	pw_roptouse = PW_USEROP;
	return;
}

pw_replrop(pw, xw, yw, width, height, op, pr, xr, yr)
	struct	pixwin *pw;
	int	op, xw, yw, width, height;
	struct	pixrect *pr;
	int	xr, yr;
{
	pw_roptouse = PW_USEREPLROP;
	pwo_rop(pw, xw, yw, width, height, op, pr, xr, yr);
	pw_roptouse = PW_USEROP;
	return;
}
