#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)pw_batch.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Pw_batch.c: Implement the retained image control operations
 *		of the pixwin.h interface.
 */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <stdio.h>
#include <pixrect/pixrect.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/cms.h>
#include <sunwindow/pixwin.h>
#include <sunwindow/pw_util.h>

void
pw_batch(pw, type)
	register Pixwin *pw;
	Pw_batch_type type;
{
	register struct pixwin_clipdata *pwcd = pw->pw_clipdata;

	/* Return if not retained */
	if (pw->pw_prretained == (struct pixrect *)0)
		return;
	/* Switch on type */
	switch (type) {
	case PW_SHOW: {
		Pw_batch_type original_type = pwcd->pwcd_batch_type;

		/* Break into 2 recursive calls */
		pw_batch(pw, PW_NONE);
		pw_batch(pw, original_type);
		return;
		}

	case PW_NONE:
		/* If any batch changes then refresh the screen */
		if (!rect_isnull(&pwcd->pwcd_batchrect))
			/* Update screen */
			(void)pw_repair_batch(pw);
		/* Reset to base state */
		(void)pw_reset_batch(pw);
		return;

	case PW_ALL:
		pwcd->pwcd_batch_type = PW_ALL;
		break;

	default:
		pwcd->pwcd_batch_type = type;
		break;
	}
	pwcd->pwcd_op_limit = (int)pwcd->pwcd_batch_type;
	/* If new op limit is less then current op count the refresh screen */
	if (pwcd->pwcd_op_count >= pwcd->pwcd_op_limit)
		(void)pw_repair_batch(pw);
	return;
}

pw_repair_batch(pw)
	Pixwin *pw;
{
	register struct pixwin_clipdata *pwcd = pw->pw_clipdata;
	Pw_batch_type original_type = pwcd->pwcd_batch_type;
	int original_lockcount = pwcd->pwcd_lockcount;

	/* Set things up so that the repair will actually lock the screen */
	pwcd->pwcd_batch_type = PW_NONE;
	pwcd->pwcd_lockcount = 0;
	(void)pw_repair(pw, &pwcd->pwcd_batchrect);
	/* Reset state to same before repair */
	pwcd->pwcd_lockcount = original_lockcount;
	pwcd->pwcd_batch_type = original_type;
	/* Reset batching stuff */
	pwcd->pwcd_op_count = 0;
	pwcd->pwcd_batchrect = rect_null;
}

pw_update_batch(pw, r)
	Pixwin *pw;
	Rect *r;
{
	register struct pixwin_clipdata *pwcd = pw->pw_clipdata;

	/* Refresh screen if op count has reached limit */
	if (pwcd->pwcd_op_count >= pwcd->pwcd_op_limit)
		(void)pw_repair_batch(pw);
	/* Update op count */
	pwcd->pwcd_op_count++;
	/* Update batch extent */
	pwcd->pwcd_batchrect = rect_bounding(r, &pwcd->pwcd_batchrect);
}

pw_reset_batch(pw)
	Pixwin *pw;
{
	register struct pixwin_clipdata *pwcd = pw->pw_clipdata;

	/* Reset op count */
	pwcd->pwcd_op_count = 0;
	/* Reset op limit */
	pwcd->pwcd_op_limit = 0;
	/* Reset batch extent */
	pwcd->pwcd_batchrect = rect_null;
	/* Reset batch type */
	pwcd->pwcd_batch_type = PW_NONE;
}

void
pw_set_xy_offset(pw, x_offset, y_offset)
	Pixwin *pw;
	int x_offset, y_offset;
{
	register struct pixwin_clipdata *pwcd = pw->pw_clipdata;
	Rect r;

	/* Return if offset the same as currently in force */
	if (pwcd->pwcd_x_offset == x_offset && pwcd->pwcd_y_offset == y_offset)
		return;
	/* Set offset variables */
	pwcd->pwcd_x_offset = x_offset;
	pwcd->pwcd_y_offset = y_offset;
	/* Recompute clipping to take into account the new offsets */
	(void)pw_setup_clippers(pw);
	/* Fixup screen by roping from retained pixwin (if present) */
	(void)win_getsize(pwcd->pwcd_windowfd, &r);
	(void)pw_repair(pw, &r);
}

void
pw_set_x_offset(pw, x_offset)
	Pixwin *pw;
	int x_offset;
{
	pw_set_xy_offset(pw, x_offset, pw_get_y_offset(pw));
}

void
pw_set_y_offset(pw, y_offset)
	Pixwin *pw;
	int y_offset;
{
	pw_set_xy_offset(pw, pw_get_x_offset(pw), y_offset);
}

int
pw_get_x_offset(pw)
	Pixwin *pw;
{
	return(pw->pw_clipdata->pwcd_x_offset);
}

int
pw_get_y_offset(pw)
	Pixwin *pw;
{
	return(pw->pw_clipdata->pwcd_y_offset);
}

