#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)gfxsw_input.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Overview: Implement gfxsw_setinputmask
 */

#include <sys/types.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/ttychars.h>
#include <sundev/kbd.h>
#include <signal.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_util.h>
#include <pixrect/memvar.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/pixwin.h>
#include <sunwindow/win_struct.h>
#include <sunwindow/win_environ.h>
#include <sunwindow/win_screen.h>
#include <sunwindow/win_input.h>
#include <suntool/tool.h>
#include <suntool/gfxsw.h>
#include <suntool/gfxswimpl.h>

gfxsw_setinputmask(gfx, im_set, im_flush, nextwindownumber, usems, usekbd)
	struct	gfxsubwindow *gfx;
	int 	nextwindownumber;
	struct	inputmask *im_set, *im_flush;
	int	usems, usekbd;
{
	struct	screen screen;
	struct	gfx_takeover *gt = gfxgettakeover(gfx);

	if (gt && gfxmadescreen(gt)) {
		(void)win_screenget(gfx->gfx_windowfd, &screen);
		/*
		 * NULL name form win_screenget means no device.
		 * Win_set* NULL name means use default.
		 */
		if (screen.scr_msname[0] == '\0' && usems)
			(void)win_setms(gfx->gfx_windowfd, &screen);
		if (screen.scr_kbdname[0] == '\0' && usekbd)
			(void)win_setkbd(gfx->gfx_windowfd, &screen);
	}
	(void)win_setinputmask(gfx->gfx_windowfd, im_set, im_flush, nextwindownumber);
}

/*ARGSUSED*/
gfxsw_inputinterrupts(gfx, ie)
	struct	gfxsubwindow *gfx;
	struct	inputevent *ie;
{
	int	sig = 0;

	/*
	 * Note: should be beefed up to look at tty chars
	 */
	switch (ie->ie_code) {
	case 0x7F:
	case CTRL(c):
		sig = SIGINT;
		goto killit;
	case CTRL(d):
		sig = SIGTERM;
		goto killit;
	case CTRL(z):
		sig = SIGTSTP;
		goto killit;
	case CTRL(\\):
		if (ie->ie_shiftmask & SHIFTMASK) {
			sig = SIGQUIT;
			goto killit;
		}
		break;
	}
	return(0);
killit:
	if (kill(getpid(), sig))
		perror("gfxsw_inputinterrupts (kill error)");
	return(1);
}
