#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)gfxsw_nocur.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Overview: Implement the abstraction defined in gfxsw.h
 */

#include <sys/types.h>
#include <sys/file.h>
#include <signal.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_util.h>
#include <pixrect/memvar.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/pixwin.h>
#include <sunwindow/win_struct.h>
#include <sunwindow/win_environ.h>
#include <sunwindow/win_cursor.h>
#include <suntool/tool.h>
#include <suntool/gfxsw.h>
#include <suntool/gfxswimpl.h>

mpr_static(blank_mpr, 0, 0, 0, 0);
struct  cursor blank_cursor = { 0, 0, PIX_SRC^PIX_DST, &blank_mpr};

gfxsw_notusingmouse(gfx)
	struct	gfxsubwindow *gfx;
{
	struct	gfx_takeover *gt = (struct gfx_takeover *) 0;

	if (gt && gfxmadescreen(gt)) {
		int	pwco_1win0mouselock(), pwco_1win0mouseunlock(),
			    pwco_1win0mousereset();

		/*
		 * Caller is not using the mouse so we can turn off
		 * the image and thusly can safely avoid doing locking.
		 */
		(void)win_setcursor(gfx->gfx_windowfd, &blank_cursor);
		gfx->gfx_pixwin->pw_clipops->pwco_lock =
		    (int (*)()) pwco_1win0mouselock;
		gfx->gfx_pixwin->pw_clipops->pwco_unlock =
		    (int (*)()) pwco_1win0mouseunlock;
		gfx->gfx_pixwin->pw_clipops->pwco_reset =
		    (int (*)()) pwco_1win0mousereset;
	}
}

