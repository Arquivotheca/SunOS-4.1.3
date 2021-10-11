#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)csr_init.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Character screen initialization and cleanup routines.
 */
#include <sys/types.h>
#include <sys/file.h>
#include <sys/time.h>
#include <signal.h>
#include <pixrect/pixrect.h>
#include <pixrect/pixfont.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/pixwin.h>
#include <sunwindow/win_environ.h>
#include <sunwindow/win_struct.h>
#include <sunwindow/win_input.h>
#include <suntool/charimage.h>
#include <suntool/charscreen.h>

extern	wfd;

struct	pixwin *csr_pixwin;

/*
 * Character screen initialization
 */
wininit(window_fd, maximagewidth, maximageheight)
	int     window_fd, *maximagewidth, *maximageheight;
{
	struct	inputmask im;
	struct  pixwin *pw;
	struct	rect rect;
	extern	struct pixfont *pw_pfsysopen();

	/*
	 * Set input masks
	 */
	(void)win_get_kbd_mask(window_fd, &im);
	im.im_flags |= IM_EUC;
	im.im_flags |= IM_META;
	win_setinputcodebit(&im, KBD_USE);
	win_setinputcodebit(&im, KBD_DONE);
	(void)win_set_kbd_mask(window_fd, &im);
	(void)win_get_pick_mask(window_fd, &im);
	im.im_flags |= IM_NEGEVENT;
	win_setinputcodebit(&im, MS_LEFT);
	win_setinputcodebit(&im, MS_MIDDLE);
	win_setinputcodebit(&im, MS_RIGHT);
	win_setinputcodebit(&im, KBD_REQUEST);
	(void)win_set_pick_mask(window_fd, &im);
	/*
	 * Setup pixwin
	 */
	if ((pw = pw_open_monochrome(window_fd)) == (struct pixwin *)0)
		return(0);

	csr_pixwin = pw;
	/*
	 * Setup max image sizes.
	 */
	*maximagewidth = csr_pixwin->pw_pixrect->pr_width - chrleftmargin;
	if (*maximagewidth < 0)
		*maximagewidth = 0;
	*maximageheight = csr_pixwin->pw_pixrect->pr_height;
	/*
	 * Determine sizes
	 */
	pixfont = pf_open((char *)0);
	chrwidth = pixfont->pf_defaultsize.x;
	chrheight = pixfont->pf_defaultsize.y;
	chrbase = -(pixfont->pf_char['n'].pc_home.y);
	(void)win_getsize(window_fd, &rect);
	winwidthp = rect.r_width;
	winheightp = rect.r_height;
	return(1);
}

/*
 * Character screen cleanup.
 */
winCleanup()
{
	if (csr_pixwin)
		(void)pw_close(csr_pixwin);
}

