#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)pw_minlock.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Overview: Minimal locking routines when only window on screen.
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/pixwin.h>

/*ARGSUSED*/
pwco_1win0mouselock(pw, r)
	struct	pixwin *pw;
	struct	rect *r;
{
	/*
	 * Bump lock count if already locked
	 */
	if (pw->pw_clipdata->pwcd_lockcount) {
		pw->pw_clipdata->pwcd_lockcount++;
		return;
	}
	if ((pw->pw_clipdata->pwcd_state == PWCD_SINGLERECT) ||
	   (pw->pw_clipdata->pwcd_state == PWCD_NULL)) {
		pw->pw_opshandle = (caddr_t) pw->pw_clipdata->pwcd_prsingle;
		pw->pw_ops = pw->pw_pixrect->pr_ops;
		pw->pw_opsx = pw->pw_clipdata->pwcd_clipping.rl_bound.r_left;
		pw->pw_opsy = pw->pw_clipdata->pwcd_clipping.rl_bound.r_top;
	}
}

pwco_1win0mouseunlock(pw)
	struct	pixwin *pw;
{
	extern	struct pixrectops pw_opsstd;

	if (pw->pw_clipdata->pwcd_lockcount==0)
		return;
	if (pw->pw_clipdata->pwcd_lockcount==1) {
		if ((pw->pw_clipdata->pwcd_state == PWCD_SINGLERECT) ||
		   (pw->pw_clipdata->pwcd_state == PWCD_NULL)) {
			pw->pw_opshandle = (caddr_t) pw;
			pw->pw_ops = &pw_opsstd;
			pw->pw_opsx = 0;
			pw->pw_opsy = 0;
		}
	}
	pw->pw_clipdata->pwcd_lockcount--;
}

pwco_1win0mousereset(pw)
	struct	pixwin *pw;
{
	pw->pw_clipdata->pwcd_lockcount = 0;
}

