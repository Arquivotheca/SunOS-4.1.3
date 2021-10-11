#ifndef lint
#ifdef sccs
static char sccsid[] = "@(#)teksw.c 1.1 92/07/30 Copyr 1989 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

/*
 * Tektronix 4014 emulator subwindow interface
 *
 * Author: Steve Kleiman
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/file.h>

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <utmp.h>
#include <pwd.h>

#include <suntool/sunview.h>
#include <suntool/canvas.h>
#include <suntool/walkmenu.h>
#include <suntool/panel.h>

#include "teksw.h"
#include "tek.h"
#include "teksw_imp.h"

/* teksw cursor */
static short tekcur_data[16] = {
	0x8000, 0xC000, 0xE000, 0xF000, 0xF800, 0xFC00, 0xFE00, 0xF000,
	0xD800, 0x9800, 0x0C00, 0x0C00, 0x0600, 0x0600, 0x0300, 0x0300
};

mpr_static(tekcur_mpr, 16, 16, 1, tekcur_data);

/*
 * Internal routines
 */
static int tty_init();
static int updateutmp();
static Notify_value catch_sigpipe();

/*
 * Tool subwindow creation of teksw
 */
Window
teksw_create(owner, argc, argv)
	Window owner;
	int argc;
	char **argv;
{
	register struct teksw *tsp;
	extern void teksw_call_props();
	extern char *calloc();

	/*
	 * Create teksw.
	 */
	tsp = (struct teksw *)calloc(1, sizeof (struct teksw));
	if (tsp == NULL) {
		fprintf(stderr, "cannot create tek subwindow\n");
		exit(1);
	}
	tsp->owner = owner;
	/*
	 * Create canvas.
	 */
	tsp->canvas =
	    window_create(owner, CANVAS,
		WIN_CLIENT_DATA,		(caddr_t)tsp,
		WIN_CONSUME_KBD_EVENTS,
			WIN_ASCII_EVENTS, KEY_PAGE_RESET, KEY_COPY, KEY_RELEASE,
			ACTION_PROPS,
			0,
		WIN_EVENT_PROC,			teksw_event,
		CANVAS_FAST_MONO,		TRUE,
		CANVAS_RETAINED,		TRUE,
		CANVAS_FIXED_IMAGE,		TRUE,
		CANVAS_AUTO_EXPAND,		FALSE,
		CANVAS_AUTO_SHRINK,		FALSE,
		CANVAS_MARGIN,			BORDER,
		0);
	/*
	 * Init teksw.
	 */
	tsp->pwp = (struct pixwin *)window_get(tsp->canvas, CANVAS_PIXWIN);
	tsp->cursor =
	    cursor_create(
		CURSOR_IMAGE, &tekcur_mpr,
		CURSOR_XHOT, 0,
		CURSOR_YHOT, 0,
		CURSOR_OP, PIX_SRC|PIX_DST,
		CURSOR_SHOW_CURSOR, TRUE,
		CURSOR_CROSSHAIR_OP, PIX_SRC|PIX_DST,
		0);
	window_set(tsp->canvas,
		WIN_CURSOR, tsp->cursor,
		0);
	if (tekfont[0] == (struct pixfont *) 0)
		teksw_initfont(argc, argv);
	tsp->imagesize.x = (int)window_get(tsp->canvas, WIN_WIDTH);
	tsp->imagesize.y = (int)window_get(tsp->canvas, WIN_HEIGHT);
	tsp->scalesize.x = tsp->imagesize.x - WXSPACE;
	tsp->scalesize.y = tsp->imagesize.y - WYSPACE;
	if (tsp->scalesize.y > ((tsp->scalesize.x * (long) TYSIZE) / TXSIZE)) {
		tsp->scalesize.y =
			((tsp->scalesize.x * (long) TYSIZE) / TXSIZE);
	} else {
		tsp->scalesize.x =
			((tsp->scalesize.y * (long) TXSIZE) / TYSIZE);
	}
	tsp->menu =
	    menu_create(
		MENU_TITLE_ITEM, "4014",
		MENU_STRING_ITEM, "Page", PAGE,
		MENU_STRING_ITEM, "Reset", RESET,
		MENU_STRING_ITEM, "Copy", COPY,
		MENU_STRING_ITEM, "Release", RELEASE,
		MENU_ACTION_ITEM, "Props", teksw_call_props,
		MENU_CLIENT_DATA, (caddr_t)tsp,
		0);
	notify_set_signal_func(tsp, catch_sigpipe, SIGPIPE, NOTIFY_ASYNC);
	/*
	 * initialize variables
	 */
	tsp->uiflags = 0;
	tsp->ptyorp = tsp->ptyowp = tsp->ptyobuf;
	tsp->curfont = tekfont[0];
	tsp->curpos.x = WXMIN(tsp);
	tsp->curpos.y = WYMIN(tsp);
	tsp->straps = getstraps(argc, argv);
	tty_init(tsp);
	tsp->temu_data = tek_create((caddr_t)tsp, tsp->straps);
	return((Window)tsp);
}

teksw_initfont(argc, argv)
	int argc;
	char **argv;
{
	if (tekfont[0] == (struct pixfont *) 0) {
		char font[MAXFONTNAMELEN];
		register int i;
		register char *cp;
		extern char *getenv();

		while (cp = *argv++) {
			if (strncmp(cp, "-f", 2) == 0) {
				cp += 2;
				if (*cp == NULL) {
					cp = *argv++;
					if (cp == NULL || *cp == NULL)
						cp = DEFAULTFONTDIR;
				}
				goto foundfonts;
			}
		}
		if ((cp = getenv("TEKFONTS")) == NULL)
			cp = DEFAULTFONTDIR;
foundfonts:
		strncpy(font, cp, MAXFONTNAMELEN - 10);
		strcat(font, "/tekfont0");
		cp = &font[strlen(font) - 1];
		for (i = 0; i < NFONT; i++) {
			*cp = i + '0';
			if ((tekfont[i] = pf_open(font)) == NULL) {
				fprintf(stderr, "cannot open font %s\n", font);
				return;
			}
		}
	}
}

static int
getstraps(argc, argv)
	int argc;
	register char **argv;
{
	register char *cp;
	register int straps;
	extern char *getenv();

	/*
	 * Look for straps in command line, or in environment.
	 */
	while (cp = *argv++) {
		if (strncmp(cp, "-s", 2) == 0) {
			cp += 2;
			if (*cp == NULL) {
				cp = *argv++;
				if (cp == NULL || *cp == NULL)
					return (DEFAULTSTRAPS);
			}
			goto foundstraps;
		}
	}
	if ((cp = getenv("TEKSTRAPS")) == NULL) {
		return (DEFAULTSTRAPS);
	}
foundstraps:
	straps = 0;
	while (*cp) {
		switch (*cp) {
		case 'l':
			straps |= LFCR;
			break;
		case 'c':
			straps |= CRLF;
			break;
		case 'd':
			straps |= DELLOY;
			break;
		case 'e':
			straps |= AECHO;
			break;
		case 'g':
			cp++;
			if (*cp == 'c')
				straps |= GINCR;
			else if (*cp == 'e')
				straps |= GINCRE;
			break;
		case 't':
			straps |= CRT_CHARS;
			break;
		case 'm':
			cp++;
			switch (*cp) {
			default:
			case '0':
				break;
			case '1':
				straps |= MARG1;
				break;
			case '2':
				straps |= MARG2;
				break;
			}
			break;
		case 'a':
			straps |= ACOPY;
			break;
		}
		cp++;
	}
	return (straps);
}

/*
 * Fork the teksw's driving tty process.
 */
int
teksw_fork(teksw, argc, argv)
	Window teksw;
	int argc;
	char **argv;
{
	register struct teksw *tsp;
	char *args[4];
	char **argstoexec;
	char *shell;
	int child_pid;
	int windowfd;
	void (*presigval)();
	extern char *getenv();

	tsp = (struct teksw *)teksw;
	windowfd = (int)window_get(tsp->canvas, WIN_FD);
	child_pid = fork();
	if (child_pid < 0) {
		perror("teksw");
		return (-1);
	}
	if (child_pid) {
		int on = 1;

		/*
		 * parent process.
		 */
		(void) fndelay(windowfd);
		(void) fndelay(tsp->pty);
		ioctl(tsp->pty, TIOCPKT, &on);
		(void) signal(SIGTSTP, SIG_IGN);
		/*
		 * the child is going to change its process group to its
		 * process id.
		 */
		return (child_pid);
	}
	/*
	 * child process. Change process group so its signal stuff doesn't
	 * affect the terminal emulator.
	 */
	(void) signal(SIGWINCH, SIG_DFL); /* we don't care about SIGWINCH's */
	child_pid = getpid();
	presigval = signal(SIGTTOU, SIG_IGN);
	if ((ioctl(tsp->tty, TIOCSPGRP, &child_pid)) == -1)
		perror("teksw_fork - TIOCSPGRP");
	setpgid(0, child_pid);
	(void) signal(SIGTTOU, presigval);
	/*
	 * Set up file descriptors
	 */
	close(windowfd);
	close(tsp->pty);
	dup2(tsp->tty, 0);
	dup2(tsp->tty, 1);
	dup2(tsp->tty, 2);
	close(tsp->tty);
	/*
	 * setup environment
	 */
	setenv("TERM", "4014");
	/*
	 * Determine what shell to run.
	 */
	shell = getenv("SHELL");
	if (!shell || !*shell)
		shell = "/bin/sh";
	args[0] = shell;
	/*
	 * Setup remainder of arguments
	 */
	args[1] = 0;
	argstoexec = args;
	if ((argv != NULL) && (*argv != NULL)) {
		while (*argv) {
			if (strcmp("-c", *argv) == 0) {
				/*
				 * The '-c' flag tells the shell to run next
				 * arg
				 */
				args[1] = *argv;
				args[2] = *(++argv);
				args[3] = 0;
				break;
			} else if (strcmp("-r", *argv) == 0) {
				/*
				 * The '-r' flag runs the next arg with next
				 * args after that as args
				 */
				argstoexec = ++argv;
				break;
			}
			argv++;
		}
	}
	execvp(*argstoexec, argstoexec);
	/*NOTREACHED*/
	perror("teksw_fork - cannot exec");
	sleep(5);	/* let user read message on teksw */
	exit(1);
}

void
teksw_destroy(teksw)
	Window teksw;
{
	register struct teksw *tsp;
	int i;

	tsp = (struct teksw *)teksw;
	for (i = 0; i < NFONT; i++) {
		pf_close(tekfont[i]);
	}
	(void) updateutmp("", tsp->ttyslot, tsp->tty);
	tek_destroy(tsp);
	window_destroy(tsp->canvas);
	window_destroy(tsp->menu);
/*
	cursor_destroy(tsp->cursor);
*/
	if (tsp->props)
		window_destroy(tsp->props);
	cfree(teksw);
}

/*
 * Internal Routines
 */

/*ARGSUSED*/
static void
teksw_call_props(menu, item)
	Menu menu;
	Menu_item item;
{
	teksw_props((struct teksw *)menu_get(menu, MENU_CLIENT_DATA));
}

static int
fndelay(fd)
	int fd;
{
	int fdflags;

	if ((fdflags = fcntl(fd, F_GETFL, 0)) == -1)
		return (-1);
	fdflags |= FNDELAY;
	if (fcntl(fd, F_SETFL, fdflags) == -1)
		return (-1);
	return (0);
}

/*
 * Do tty/pty setup
 */
static int
tty_init(tsp)
	register struct teksw *tsp;
{
	int tt, tmpfd, ptynum = 0;
	char *ptyp = "pqrstuvwxyzPQRST";
	char linebuf[20];
	char *line;
	struct stat stb;

	line = &linebuf[0];
	/*
	 * find unopened pty
	 */
needpty:
	while (*ptyp) {
		strcpy(line, "/dev/ptyXX");
		line[strlen("/dev/pty")] = *ptyp;
		line[strlen("/dev/ptyp")] = '0';
		if (stat(line, &stb) < 0)
			break;
		while (ptynum < 16) {
			line[strlen("/dev/ptyp")] = "0123456789abcdef"[ptynum];
			tsp->pty = open(line, 2);
			if (tsp->pty > 0)
				goto gotpty;
			ptynum++;
		}
		ptyp++;
		ptynum = 0;
	}
	fprintf(stderr, "All pty's in use\n");
	return (0);

gotpty:
	line[strlen("/dev/")] = 't';
	tt = open("/dev/tty", 2);
	if (tt > 0) {
		ioctl(tt, TIOCNOTTY, 0);
		close(tt);
	}
	tsp->tty = open(line, 2);
	if (tsp->tty < 0) {
		ptynum++;
		close(tsp->pty);
		goto needpty;
	}
	ttysw_restoreparms(tsp->tty);
	/*
	 * Copy stdin. Set stdin to tty so ttyslot in updateutmp will think
	 * this is the control terminal. Restore state. Note: ttyslot should
	 * have companion ttyslotf(fd).
	 */
	tmpfd = dup(0);
	close(0);
	dup(tsp->tty);
	tsp->ttyslot = updateutmp(0, 0, tsp->tty);
	close(0);
	dup(tmpfd);
	close(tmpfd);
	if (tsp->ttyslot == 0) {
		close(tsp->tty);
		close(tsp->pty);
		return (0);
	}
	notify_set_output_func(tsp, teksw_pty_output, tsp->pty);
	notify_set_input_func(tsp, teksw_pty_input, tsp->pty);
	return (1);
}

/*
 * Make entry in /etc/utmp for ttyfd. Note: this is dubious usage of /etc/utmp
 * but many programs (e.g. sccs) look there when determining who is logged in
 * to the pty.
 */
static int
updateutmp(username, ttyslotuse, ttyfd)
	char *username;
	int ttyslotuse;
	int ttyfd;
{
	/*
	 * Update /etc/utmp
	 */
	struct utmp utmp;
	struct passwd *passwdent;
	extern struct passwd *getpwuid();
	int f;
	char *ttyn;
	extern char *ttyname(), *rindex();

	if (!username) {
		/*
		 * Get login name
		 */
		if ((passwdent = getpwuid(getuid())) == 0) {
			return (0);
		}
		username = passwdent->pw_name;
	}
	utmp.ut_name[0] = '\0';	/* Set in case *username is 0 */
	strncpy(utmp.ut_name, username, sizeof (utmp.ut_name));
	/*
	 * Get line (tty) name
	 */
	ttyn = ttyname(ttyfd);
	if (ttyn == (char *) 0)
		ttyn = "/dev/tty??";
	strncpy(utmp.ut_line, rindex(ttyn, '/') + 1, sizeof (utmp.ut_line));
	/*
	 * Set host to be empty
	 */
	strncpy(utmp.ut_host, "", sizeof (utmp.ut_host));
	/*
	 * Get start time
	 */
	time(&utmp.ut_time);
	/*
	 * Put utmp in /etc/utmp
	 */
	if (ttyslotuse == 0)
		ttyslotuse = ttyslot();
	if (ttyslotuse == 0)
		return (0);
	if ((f = open("/etc/utmp", 1)) >= 0) {
		lseek(f, (long) (ttyslotuse * sizeof (utmp)), 0);
		write(f, (char *) &utmp, sizeof (utmp));
		close(f);
	} else {
		perror("/etc/utmp (make sure that you can write it!)");
		exit(1);
	}
	return (ttyslotuse);
}

/*ARGSUSED*/
static Notify_value
catch_sigpipe(tsp, sig, when)
	Notify_client tsp;
	int sig;
	Notify_signal_mode when;
{
	/*
	 * This is a no-op function meant only to prevent us from
	 * being killed because we didn't have a SIGPIPE handler.
	 */
	return (NOTIFY_IGNORED);
}
