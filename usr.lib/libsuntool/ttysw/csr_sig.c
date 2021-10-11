#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)csr_sig.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Character screen signal handling.
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/time.h>
#include <signal.h>
#include <pixrect/pixrect.h>
#include <pixrect/pixfont.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/pixwin.h>
#include <suntool/ttysw_impl.h>
#include <suntool/charimage.h>
#include <suntool/charscreen.h>

extern	wfd;
extern	struct	pixwin *csr_pixwin;
extern	void	ttysel_hilite();
extern	void	ttynullselection();

int
whandlesigwinch(ttysw)
	Ttysw *ttysw;
{
	struct	rect r_new;

	/*
	 * See if size changed
	 */
	(void)win_getsize(wfd, &r_new);
	if (winwidthp!=r_new.r_width ||
	winheightp!=r_new.r_height) {
		winwidthp = r_new.r_width;
		winheightp = r_new.r_height;
		/*
		 * imagerepair redraws the exposed image so toss damaged list
		 * before imagerepair call in order to avoid race.
		 */
		(void)pw_damaged(csr_pixwin);
		(void)pw_donedamaged(csr_pixwin);
		/*
		 * Don't currently support selections across size changes
		 */
		ttynullselection(ttysw);
		/*
		 * Fix image and redraw the screen.
		 */
		(void)imagerepair(ttysw);
		return(1/*size has changed*/);
	}
	/*
	 * Fix screen
	 */
	(void)pw_damaged(csr_pixwin);
	(void)saveCursor();
	(void)prepair(wfd, &csr_pixwin->pw_clipdata->pwcd_clipping);
	/*
	 * If just hilite the selection part that is damaged then the other
	 * non-damaged selection parts should still be visible, thus creating
	 * the entire selection image.
	 */
	ttysel_hilite(ttysw);
	(void)restoreCursor();
	(void)pw_donedamaged(csr_pixwin);
	return(0);
}

