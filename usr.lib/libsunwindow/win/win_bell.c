#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)win_bell.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Win_bell.c: Implement the keyboard bell.
 */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sundev/kbd.h>
#include <sundev/kbio.h>
#include <pixrect/pixrect.h>
#include <sunwindow/defaults.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/cms.h>
#include <sunwindow/pixwin.h>
#include <sunwindow/win_screen.h>
extern void pw_batch();

/* Bell cached defaults */
static	Bool	win_do_audible_bell;
static	Bool	win_do_visible_bell;
static	int	win_bell_done_init;

/*
 * Ring bell and flash window
 */
win_bell(windowfd, tv, pw)
	int 	windowfd;
	struct	timeval tv;
	register struct	pixwin *pw;
{
	struct screen screen;
	Rect r;
	int kbdfd, cmd;

	/* Get defaults for bell if first time */
	if (!win_bell_done_init) {
		win_do_audible_bell = defaults_get_boolean(
		    "/SunView/Audible_Bell", (Bool)TRUE, (int *)NULL);
		win_do_visible_bell = defaults_get_boolean(
		    "/SunView/Visible_Bell", (Bool)TRUE, (int *)NULL);
		win_bell_done_init = 1;
	}
	if (windowfd >= 0 && win_do_audible_bell) {
		/* Get screen keyboard name */
		win_screenget(windowfd, &screen);
		/* Open keyboard */
		if ((kbdfd = open(screen.scr_kbdname, O_RDWR, 0)) < 0)
			return;
	}
	/* Flash pw */
	if (pw && windowfd >= 0 && win_do_visible_bell) {
		(void)win_getsize(windowfd, &r);
		(void)pw_writebackground(pw, 0, 0,
		    r.r_width, r.r_height, PIX_NOT(PIX_DST));
		pw_show(pw);
	}
	if (windowfd >= 0 && win_do_audible_bell) {
		/* Start bell */
		cmd = KBD_CMD_BELL;
		(void) ioctl(kbdfd, KIOCCMD, &cmd);
	}
	/* Wait */
	(void)blocking_wait(tv);
	if (windowfd >= 0 && win_do_audible_bell) {
		/* Stop bell */
		cmd = KBD_CMD_NOBELL;
		(void) ioctl(kbdfd, KIOCCMD, &cmd);
		/* Close keyboard */
		close(kbdfd);
	}
	/* Turn off flash */
	if (pw && windowfd >= 0 && win_do_visible_bell) {
		(void)pw_writebackground(pw, 0, 0,
		    r.r_width, r.r_height, PIX_NOT(PIX_DST));
		pw_show(pw);
	}
}

blocking_wait(wait_tv)
	struct timeval wait_tv;
{
	extern struct timeval ndet_tv_subt(); /* From notifier code */
	struct timeval start_tv, now_tv, waited_tv;
        fd_set bits;    /* ECD fix */

	/* Get starting time */
	(void)gettimeofday(&start_tv, (struct timezone *)0);
	/* Wait */
	while (timerisset(&wait_tv)) {
		/* Wait for awhile in select */
                FD_ZERO(&bits);         /* ECD fix */
		(void) select(0, &bits, &bits, &bits, &wait_tv);
		/* Get current time */
		(void)gettimeofday(&now_tv, (struct timezone *)0);
		/* Compute how long waited */
		waited_tv = ndet_tv_subt(now_tv, start_tv);
		/* Subtract time waited from time left to wait */
		wait_tv = ndet_tv_subt(wait_tv, waited_tv);
	}
}
