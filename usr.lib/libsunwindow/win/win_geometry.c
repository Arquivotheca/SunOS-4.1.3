#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)win_geometry.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Win_geometry.c: Implement the geometry operation functions
 *	of the win_struct.h interface.
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
 * Geometry operations.
 */
win_getrect_from_source(windowfd, rect)
	int	windowfd;
	struct	rect *rect;
{
	(void)werror(ioctl(windowfd, WINGETRECT, rect), WINGETRECT);
	return;
}

win_setrect_at_source(windowfd, rect)
	int	windowfd;
	struct	rect *rect;
{
        (void)werror(ioctl(windowfd, WINSETRECT, rect), WINSETRECT);
	return;
}

win_getrect(windowfd, rect)
	int	windowfd;
	struct	rect *rect;
{
	win_getrect_local(windowfd, rect);
	return;
}

win_setrect(windowfd, rect)
	int	windowfd;
	struct	rect *rect;
{
	win_setrect_local(windowfd, rect);
	return;
}

win_setsavedrect(windowfd, rect)
	int	windowfd;
	struct	rect *rect;
{
	(void)werror(ioctl(windowfd, WINSETSAVEDRECT, rect), WINSETSAVEDRECT);
	return;
}

win_getsavedrect(windowfd, rect)
	int	windowfd;
	struct	rect *rect;
{
	(void)werror(ioctl(windowfd, WINGETSAVEDRECT, rect), WINGETSAVEDRECT);
	return;
}

/*
 * Utilities
 */
win_getsize(windowfd, rect)
	int	windowfd;
	struct	rect *rect;
{
	(void)win_getrect(windowfd, rect);
	rect->r_left = 0;
	rect->r_top = 0;
	return;
}

coord
win_getheight(windowfd)
	int	windowfd;
{
	struct	rect rect;

	(void)win_getrect(windowfd, &rect);
	return(rect.r_height);
}

coord
win_getwidth(windowfd)
	int	windowfd;
{
	struct	rect rect;

	(void)win_getrect(windowfd, &rect);
	return(rect.r_width);
}

