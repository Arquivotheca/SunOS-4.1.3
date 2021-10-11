#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)win_global.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Win_global.c: Implement the functions of the win_struct.h interface
 * that exercise global influence on the whole window environment.
 */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sunwindow/rect.h>
#include <sunwindow/cms.h>
#include <sunwindow/win_screen.h>
#include <sunwindow/win_input.h>
#include <sunwindow/win_ioctl.h>
#include <sunwindow/win_struct.h>

/*
 * When not zero will not actually acquire data lock
 * so that the debugger wouldn't get hung.
 * USE ONLY DURING DEBUGGING WHEN YOU KNOW WHAT YOUR DOING!
 */
int	win_lockdatadebug;
/*
 * When not zero will not actually acquire exclusive io access rights
 * so that the debugger wouldn't get hung.
 * USE ONLY DURING DEBUGGING WHEN YOU KNOW WHAT YOUR DOING!
 */
int	win_grabiodebug;

/*
 * Stack for nesting grabios
 */
#define GRABIOLEVELS 4
static int grabiolevel;
static int grabiofd[GRABIOLEVELS];

/*
 * Kernel operations applying globally.
 */
win_lockdata(windowfd)
	int 	windowfd;
{
	if (win_lockdatadebug)
		return;
	(void)werror(ioctl(windowfd, WINLOCKDATA, 0), WINLOCKDATA);
	return;
}

win_unlockdata(windowfd)
	int 	windowfd;
{
	if (win_lockdatadebug)
		return;
	(void)werror(ioctl(windowfd, WINUNLOCKDATA, 0), WINUNLOCKDATA);
	return;
}

win_computeclipping(windowfd)
	int	windowfd;
{
	(void)werror(ioctl(windowfd, WINCOMPUTECLIPPING, 0), WINCOMPUTECLIPPING);
	return;
}

win_partialrepair(windowfd, rectok)
	int	windowfd;
	struct	rect *rectok;
{
	(void)werror(ioctl(windowfd, WINPARTIALREPAIR, rectok), WINPARTIALREPAIR);
	return;
}

win_grabio(windowfd)
	int 	windowfd;
{
        /*
         * Push outstanding grabio, if any.
         */
        if (grabiolevel > 0) {
                int fd = grabiofd[grabiolevel - 1];

                win_releaseio_local(fd);
                if (!win_grabiodebug)
			(void)werror(ioctl(fd, WINRELEASEIO, 0), WINRELEASEIO);
        }
        grabiofd[grabiolevel++] = windowfd;

	win_grabio_local(windowfd);
	if (win_grabiodebug)
		return;
	(void)werror(ioctl(windowfd, WINGRABIO, 0), WINGRABIO);
	return;
}

win_releaseio(windowfd)
	int 	windowfd;
{
	win_releaseio_local(windowfd);
	if (win_grabiodebug)
		return;
	(void)werror(ioctl(windowfd, WINRELEASEIO, 0), WINRELEASEIO);
        /*
         * If another grabio is outstanding, restore it.
         */
        if (--grabiolevel > 0) {
                int fd = grabiofd[grabiolevel - 1];
 
                win_grabio_local(fd);
                if (!win_grabiodebug)
                        (void)werror(ioctl(fd, WINGRABIO, 0), WINGRABIO);
        }
	return;
}

short
win_getfocusevent(windowfd)
	int	windowfd;
{
	short	eventcode;

	(void)werror(ioctl(windowfd, WINGETFOCUSEVENT,&eventcode), WINGETFOCUSEVENT);
	return(eventcode);
}

win_setfocusevent(windowfd, eventcode)
	int	windowfd;
	short	eventcode;
{

	(void)werror(ioctl(windowfd, WINSETFOCUSEVENT,&eventcode), WINSETFOCUSEVENT);
	return;
}

