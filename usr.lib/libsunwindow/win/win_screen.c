#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)win_screen.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 * Win_screen.c: Implement most of the screen functions of the
 * win_struct.h interface.
 */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sun/fbio.h>
#include <errno.h>
#include <stdio.h>
#include <sunwindow/rect.h>
#include <sunwindow/cms.h>
#include <sunwindow/win_screen.h>
#include <sunwindow/win_input.h>
#include <sunwindow/win_ioctl.h>
#include <sunwindow/win_struct.h>
#include <sunwindow/cms_mono.h>
#include <strings.h>

extern	int errno;

#define	SCR_DEFAULTROOTDEV	"/dev/win0"
#define	SCR_DEFAULTKBDDEV	"/dev/kbd"
#define	SCR_DEFAULTMSDEV	"/dev/mouse"
#define	SCR_DEFAULTFBDEV	"/dev/fb"
#define	SCR_NODEV		"NONE"
static	char	*openprob = "problem opening", *ioctlprob = "ioctl err";

extern	unsigned win_screendestroysleep;  /* (See win_screendestroy) */

extern	int winscreen_print;		  /* Debugging flag */

/*
 * Screen creation, inquiry and deletion.
 */
int
win_screennew(screen)
	register struct screen *screen;
{
	register int rootfd, fbfd;
	int	kbdfd, msfd;
	char	*errstr, *errdev;
	struct	usrdesktop udtop;
	struct	fbtype fbtype;

	rootfd = kbdfd = msfd = fbfd = WIN_NODEV;
	errstr = errdev = "";
	/*
	 * Open root window.
	 */
	if (screen->scr_rootname[0] == '\0') {
		(void)strncpy(screen->scr_rootname, SCR_DEFAULTROOTDEV, SCR_NAMESIZE);
	}
	if ((rootfd = open(screen->scr_rootname, O_RDWR|WIN_EXCLOPEN, 0)) < 0) {
		if (errno==EACCES) {
			/*
			 * /dev/win0 is busy so we are opening another root
			 */
			if ((rootfd = win_getnewwindow())<0) {
				errstr = "all windows currently allocated";
				goto Bad;
			}
			(void)win_fdtoname(rootfd, screen->scr_rootname);
		} else {
			errstr = openprob;
			errdev = screen->scr_rootname;
			goto Bad;
		}
	}
	/*
	 * Setup kbd
	 */
	if (scr_setupscrdev(screen->scr_kbdname, &kbdfd,
	    &errstr, &errdev, KBDLDISC, SCR_DEFAULTKBDDEV))
		goto Bad;
	/*
	 * Setup mouse
	 */
	if (scr_setupscrdev(screen->scr_msname, &msfd,
	    &errstr, &errdev, MOUSELDISC, SCR_DEFAULTMSDEV))
		goto Bad;
	/*
	 * Setup frame buffer.
	 */
	if (strncmp(screen->scr_fbname, SCR_NODEV, SCR_NAMESIZE) == 0) {
		screen->scr_fbname[0] = '\0';
	} else {
		if (screen->scr_fbname[0] == '\0')
			(void)strncpy(screen->scr_fbname, SCR_DEFAULTFBDEV,
			    SCR_NAMESIZE);
		if ((fbfd = open(screen->scr_fbname, O_RDWR, 0)) < 0) {
			errstr = openprob;
			errdev = screen->scr_fbname;
			goto Bad;
		}
	}
	/*
	 * Setup rect.
	 */
	if (rect_isnull(&screen->scr_rect)) {
		if (ioctl(fbfd, FBIOGTYPE, (caddr_t)&fbtype)) {
			errstr = ioctlprob;
			errdev = screen->scr_fbname;
			goto Bad;
		}
		rect_construct(&screen->scr_rect, 0, 0, fbtype.fb_width,
		    fbtype.fb_height);
	}
	/*
	 * Setup foreground and background colors.
	 */
	if (screen->scr_foreground.red == screen->scr_background.red &&
	    screen->scr_foreground.green == screen->scr_background.green &&
	    screen->scr_foreground.blue == screen->scr_background.blue) {
		u_char	red[CMS_MONOCHROMESIZE],
		    green[CMS_MONOCHROMESIZE], blue[CMS_MONOCHROMESIZE]; 

		cms_monochromeload(red, green, blue);
		screen->scr_foreground.red = red[BLACK];
		screen->scr_foreground.green = green[BLACK];
		screen->scr_foreground.blue = blue[BLACK];
		screen->scr_background.red = red[WHITE];
		screen->scr_background.green = green[WHITE];
		screen->scr_background.blue = blue[WHITE];
	}
	/*
	 * Make window ioctl.
	 */
	udtop.udt_scr = *screen;
	udtop.udt_fbfd = fbfd;
	udtop.udt_kbdfd = kbdfd;
	udtop.udt_msfd = msfd;
	if (ioctl(rootfd, WINSCREENNEW, &udtop) == -1) {
		(void)werror(-1, WINSCREENNEW);
		goto Bad;
	}
	/*
	 * Cleanup.
	 */
	if (fbfd != -1)
		(void)close(fbfd);
	if (msfd != -1)
		(void)close(msfd);
	if (kbdfd != -1)
		(void)close(kbdfd);
	return(rootfd);
Bad:
	if (fbfd != -1)
		(void)close(fbfd);
	if (msfd != -1)
		(void)close(msfd);
	if (kbdfd != -1)
		(void)close(kbdfd);
	if (rootfd != -1)
		(void)close(rootfd);
	if (winscreen_print)
		(void)fprintf(stderr,
		    "Win_screennew error: %s, device=%s, errno=%ld\n",
		    errstr, errdev, errno);
	return(-1);
}

win_screendestroy(windowfd)
	int 	windowfd;
{
	(void)werror(ioctl(windowfd, WINSCREENDESTROY, 0), WINSCREENDESTROY);
	/*
	 * Note: Wait so as not to aggrevate a subtle bug that
	 * causes the kernel to kill the process that owns the console
	 * (ownership is often being transferred between a ttysw,
	 * as it dies, and the login shell).  A common symptom is
	 * logging out a user as he exits suntools.  Assuming that
	 * suntools called this routine, we wait so as to give a
	 * ttysw, with the console redirected to it, the chance to
	 * die cleanly.  Thus, we reduce the frequency of this race
	 * condition.
	 */
	if (win_screendestroysleep == 0)
		/*
		 * We insist that win_screendestroysleep be non-zero
		 * so that we can get rid of the initialization to 3
		 * that used to exist so that we can move all the
		 * initialized data into the text segment.
		 */
		win_screendestroysleep = 3;
	usleep(win_screendestroysleep << 20);
	return;
}

win_setscreenpositions(windowfd, neighbors)
	int 	windowfd;
	int	neighbors[SCR_POSITIONS];
{
	(void)werror(ioctl(windowfd, WINSCREENPOSITIONS, neighbors),
	    WINSCREENPOSITIONS);
	return;
}

win_getscreenpositions(windowfd, neighbors)
	int 	windowfd;
	int	neighbors[SCR_POSITIONS];
{
	(void)werror(ioctl(windowfd, WINGETSCREENPOSITIONS, neighbors),
	    WINGETSCREENPOSITIONS);
	return;
}

int
win_setkbd(windowfd, screen)
	int	windowfd;
	register struct screen *screen;
{
	int	kbdfd = WIN_NODEV;
	char	*errstr, *errdev;
	struct	usrdesktop udtop;

	errstr = errdev = "";
	/*
	 * Setup kbd
	 */
	if (scr_setupscrdev(screen->scr_kbdname, &kbdfd,
	    &errstr, &errdev, KBDLDISC, SCR_DEFAULTKBDDEV))
		goto Bad;
	/*
	 * Make window ioctl.
	 */
	udtop.udt_scr = *screen;
	udtop.udt_kbdfd = kbdfd;
	if (ioctl(windowfd, WINSETKBD, &udtop) == -1) {
		(void)werror(-1, WINSETKBD);
		goto Bad;
	}
	/*
	 * Cleanup.
	 */
	if (kbdfd != -1)
		(void)close(kbdfd);
	return(0);
Bad:
	if (kbdfd != -1)
		(void)close(kbdfd);
	if (winscreen_print)
		(void)fprintf(stderr, "Win_setkbd error: %s, device=%s, errno=%ld\n",
		    errstr, errdev, errno);
	return(-1);
}

int
win_setms(windowfd, screen)
	int	windowfd;
	register struct screen *screen;
{
	int	msfd = WIN_NODEV;
	char	*errstr, *errdev;
	struct	usrdesktop udtop;

	errstr = errdev = "";
	/*
	 * Setup ms
	 */
	if (scr_setupscrdev(screen->scr_msname, &msfd,
	    &errstr, &errdev, MOUSELDISC, SCR_DEFAULTMSDEV))
		goto Bad;
	/*
	 * Make window ioctl.
	 */
	udtop.udt_scr = *screen;
	udtop.udt_msfd = msfd;
	if (ioctl(windowfd, WINSETMS, &udtop) == -1) {
		(void)werror(-1, WINSETMS);
		goto Bad;
	}
	/*
	 * Cleanup.
	 */
	if (msfd != -1)
		(void)close(msfd);
	return(0);
Bad:
	if (msfd != -1)
		(void)close(msfd);
	if (winscreen_print)
		(void)fprintf(stderr, "Win_setms error: %s, device=%s, errno=%ld\n",
		    errstr, errdev, errno);
	return(-1);
}

int
scr_setupscrdev(scrdev, fd, errstr, errdev, ldisc, devdefault)
	char	*scrdev;
	int	*fd;
	char	**errstr, **errdev;
	int	ldisc;
	char	*devdefault;
{
	if (strncmp(scrdev, SCR_NODEV, SCR_NAMESIZE) == 0) {
		scrdev[0] = '\0';
	} else {
		if (scrdev[0] == '\0')
			(void)strncpy(scrdev, devdefault, SCR_NAMESIZE);
		if ((*fd = open(scrdev, O_RDONLY, 0)) < 0) {
			*errstr = openprob;
			*errdev = scrdev;
			return(-1);
		}
		if (ioctl(*fd, TIOCSETD, (caddr_t)&ldisc)) {
			*errstr = ioctlprob;
			*errdev = scrdev;
			return(-1);
		}
	}
	return(0);
}

win_initscreenfromargv(screen, argv)
	register struct screen *screen;
	char    **argv;
{
	int	nextisfbname = 0, nextiskbdname = 0, nextismsname = 0,
		    nextisbkgrd = 0, nextisforegrd = 0;
	char	**args;

	bzero((caddr_t)screen, sizeof (*screen));
	if (!argv)
		return;
	for (args = ++argv;*args;args++) {
		if (nextisbkgrd) {
			if (win_getsinglecolor(&args, &screen->scr_background))
				continue;
			nextisbkgrd = 0;
		} else if (nextisforegrd) {
			if (win_getsinglecolor(&args, &screen->scr_foreground))
				continue;
			nextisforegrd = 0;
		} else if (nextisfbname) {
			(void) sscanf(*args, "%s", screen->scr_fbname);
			nextisfbname = 0;
		} else if (nextiskbdname) {
			(void) sscanf(*args, "%s", screen->scr_kbdname);
			nextiskbdname = 0;
		} else if (nextismsname) {
			(void) sscanf(*args, "%s", screen->scr_msname);
			nextismsname = 0;
		} else if ((strcmp(*args, "-b") == 0) && *(args+1))
			nextisbkgrd = 1;
		else if ((strcmp(*args, "-f") == 0) && *(args+1))
			nextisforegrd = 1;
		else if ((strcmp(*args, "-d") == 0) && *(args+1))
			nextisfbname = 1;
		else if ((strcmp(*args, "-k") == 0) && *(args+1))
			nextiskbdname = 1;
		else if ((strcmp(*args, "-m") == 0) && *(args+1))
			nextismsname = 1;
		else if (strcmp(*args, "-i") == 0)
			screen->scr_flags |= SCR_SWITCHBKGRDFRGRD;
		else if (strcmp(*args, "-toggle_enable") == 0)
			screen->scr_flags |= SCR_TOGGLEENABLE;
		else if (strcmp(*args, "-8bit_color_only") == 0)
			screen->scr_flags |= SCR_8BITCOLORONLY;
		else if (strcmp(*args, "-overlay_only") == 0)
			screen->scr_flags |= SCR_OVERLAYONLY;
	}
}

int
win_getsinglecolor(argsptr, scolor)
	char    ***argsptr;
	struct	singlecolor *scolor;
{
	char	*cptr = (caddr_t) scolor;
	int	buf;
	register char	**args;
	register int	error = -1;

	for (args = *argsptr;*args;args++) {
		error = -1;
		if (sscanf(*args, "%d", &buf) != 1)
			break;
		*cptr = buf;
		if (cptr++ == (caddr_t)&scolor->blue) {
			error = 0;
			break;
		}
	}
	if (error)
		(void)fprintf(stderr, "problem parsing color triplet\n");
	*argsptr = args;
	return(error);
}

