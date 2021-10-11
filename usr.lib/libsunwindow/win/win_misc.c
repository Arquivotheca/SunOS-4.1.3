#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)win_misc.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 * Win_misc.c: Implement the misc functions of the win_struct.h interface.
 *	Error handling done here.
 */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <sunwindow/rect.h>
#include <sunwindow/cms.h>
#include <sunwindow/win_screen.h>
#include <sunwindow/win_input.h>
#include <sunwindow/win_ioctl.h>
#include <sunwindow/win_struct.h>
extern char *sprintf();

/*
 * Misc operations.
 */
int
win_getuserflags(windowfd)
	int	windowfd;
{
	int	flags;

	(void)werror(ioctl(windowfd, WINGETUSERFLAGS, &flags), WINGETUSERFLAGS);
	return(flags);
}

win_setuserflags(windowfd, flags)
	int	windowfd;
	int	flags;
{
	(void)werror(ioctl(windowfd, WINSETUSERFLAGS, &flags), WINSETUSERFLAGS);
	return;
}

int
win_getowner(windowfd)
	int	windowfd;
{
	int	pid;

	(void)werror(ioctl(windowfd, WINGETOWNER, &pid), WINGETOWNER);
	return(pid);
}

win_setowner(windowfd, pid)
	int	windowfd;
	int	pid;
{
	/*
	 * Don't set if pid is 0
	 */
	if (pid)
		(void)werror(ioctl(windowfd, WINSETOWNER, &pid), WINSETOWNER);
	return;
}

win_print(windowfd)
	int	windowfd;
{
	(void)werror(ioctl(windowfd, WINPRINT, 0), WINPRINT);
}

/*
 * Error handling
 */
int	win_errordefault();
extern	int (*win_error)();

werror(errnum, winopnum)
	int	errnum, winopnum;
{
	win_error(errnum, winopnum);
	return;
}

int
(*win_errorhandler(win_errornew))()
	int	(*win_errornew)();
{
	int	(*win_errortemp)() = win_error;

	win_errortemp = win_error;
	win_error = win_errornew;
	return(win_errortemp);
}

win_errordefault(errnum, winopnum)
	int	errnum, winopnum;
{
	char	message[30];

	switch (errnum) {
	case 0:
		return;
	case -1:
		/*
		 * Probably an ioctl err (could check winopnum)
		 */
		message[0] = '\0';
		(void)sprintf(message, "WIN ioctl number %lx", winopnum);
		perror(message);
		return;
	default:
		(void)fprintf(stderr, "Window operation %d produced error %d\n",
		    winopnum, errnum);
	}
}

/*
 * Utilities
 */
win_setuserflag(windowfd, flag, value)
	int	windowfd;
	int	flag;
	bool	value;
{
	int	flags = win_getuserflags(windowfd);

	if (flag <= WUF_RESERVEDMAX)
		return(-1);
	else {
		if (value)
			flags |= flag;
		else
			flags &= ~flag;
		(void)win_setuserflags(windowfd, flags);
	}
	return(0);
}

