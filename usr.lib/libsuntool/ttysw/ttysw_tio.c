#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)ttysw_tio.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Selected and handle SIGWINCH code for non-notifier world.
 */

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <sgtty.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sundev/kbd.h>

#include <pixrect/pixrect.h>
#include <pixrect/pr_util.h>
#include <pixrect/memvar.h>
#include <sunwindow/rect.h>
#include <sunwindow/win_input.h>
#include <sunwindow/win_struct.h>
#include <sunwindow/win_cursor.h>
#include <suntool/ttysw.h>
#include <suntool/ttysw_impl.h>

extern	int errno;
extern	int delaypainting;
static	struct timeval shorttimeout;

#define	TTYSW_USEC_DELAY 100000

/* shorthand - Duplicate of what's in ttysw_main.c */
#define	iwbp	ttysw->ttysw_ibuf.cb_wbp
#define	irbp	ttysw->ttysw_ibuf.cb_rbp
#define	owbp	ttysw->ttysw_obuf.cb_wbp
#define	orbp	ttysw->ttysw_obuf.cb_rbp
#define	iebp	ttysw->ttysw_ibuf.cb_ebp

ttysw_selected(ttysw0, ibits, obits, ebits, timer)
	caddr_t ttysw0;
	register int *ibits, *obits, *ebits;
	register struct timeval **timer;
{
	register struct ttysubwindow *ttysw = (struct ttysubwindow *)LINT_CAST(ttysw0);
	register int wfd = ttysw->ttysw_wfd;
	register int pty = ttysw->ttysw_pty;

	if (delaypainting &&
	    *timer && ((*timer)->tv_sec == 0) && ((*timer)->tv_usec == 0)) {
		/*
		 * Our timer expired so there is no data pending.
		 */
		(void)ttysw_handle_itimer(ttysw);
		*timer = (struct timeval *) 0;
	}
	/* Read input from window */
	if (*ibits & (1<<wfd))
		(void) readwin(ttysw);
	/* Write window input to pty */
	if (*obits & (1<<pty))
		(void) ttysw_pty_output(ttysw, pty);
	/* Read pty's input (which is output from program) */
	if (*ibits & (1<<pty))
		(void) ttysw_pty_input(ttysw, pty);
	/* Send program output to terminal emulator */
	(void)ttysw_consume_output(ttysw);
	/* Setup masks again */
	*ibits = 0;
	*obits = 0;
	*ebits = 0;
	if (iwbp > irbp)
		*obits |= (1<<pty);
	*ibits |= (1<<wfd);
	if (owbp == orbp)
		*ibits |= (1<<pty);
#ifdef notdef
	if (ttysw->ttysw_wcc < 0 && ttysw->ttysw_pcc < 0)
		goto done;
#endif
	/*
	 * Try to optimize displaying by waiting for image to be completely
	 * filled after being cleared (vi(^F ^B) page) before painting.
	 */
	if (delaypainting) {
		shorttimeout.tv_sec = 0;
		shorttimeout.tv_usec = TTYSW_USEC_DELAY;
		*timer = &shorttimeout;
	}
	/*
	 * Check that not polling with zero timer.  I'm not sure how
	 * it gets in this state but it is wrong.
	 * Probably the very first test in the procedure (delaypainting && ...)
	 * fails because delaypainting is false but the timer has gone off.
	 */
	if (*timer && ((*timer)->tv_sec == 0) && ((*timer)->tv_usec == 0))
		*timer = (struct timeval *) 0;
}

/*
 * Window changed signal handler
 */
ttysw_handlesigwinch(ttysw0)
	caddr_t ttysw0;
{
	struct ttysubwindow *ttysw = (struct ttysubwindow *)LINT_CAST(ttysw0);
	int pgrp;
	int pagemode;

	/*
	 * Turn off page mode while potentially changing size.
	 */
	pagemode = ttysw_getopt((caddr_t)ttysw, TTYOPT_PAGEMODE);
	(void)ttysw_setopt((caddr_t)ttysw, TTYOPT_PAGEMODE, 0);
	(void) whandlesigwinch(ttysw);
	(void)ttysw_setopt((caddr_t)ttysw, TTYOPT_PAGEMODE, pagemode);
	/*
	 * Notify process group that terminal has changed.
	 */
	if (ioctl(ttysw->ttysw_tty, TIOCGPGRP, &pgrp) == -1) {
		perror("ttysw_handlesigwinch, can't get tty process group");
		return;
	}
	/*
	 * Only killpg when pgrp is not tool's.  This is the case of
	 * haven't completed ttysw_fork yet (or even tried to do it yet).
	 */
	if (getpgrp(0) != pgrp)
		/*
		 * killpg could return -1 with errno == ESRCH but this is OK.
		 */
		(void) killpg(pgrp, SIGWINCH);
}

/*
 * User input reader
 */
static
readwin(ttysw)
	register struct ttysubwindow *ttysw;
{
	struct inputevent event;
	int error;

	while (iwbp < iebp) {
		error = input_readevent(ttysw->ttysw_wfd, &event);
		if (error < 0)
			if (errno == EWOULDBLOCK)
				break;
			else
				return (-1);
		if (ttysw_handleevent(ttysw, &event) == TTY_DONE)
			break;
	}
	return (0);
}

