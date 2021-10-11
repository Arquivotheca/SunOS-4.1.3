#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)win_cursor.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Win_cursor.c: Implement the mouse cursor operation functions
 *	of the win_struct.h interface.
 */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <pixrect/pixrect.h>
#include <sunwindow/rect.h>
#include <sunwindow/cms.h>
#include <sunwindow/win_screen.h>
#include <sunwindow/win_input.h>
#include <sunwindow/win_ioctl.h>
#include <sunwindow/win_struct.h>
#include <sunwindow/win_cursor.h>

/*
 * Mouse cursor operations.
 */
win_setmouseposition(windowfd, x, y)
	int	windowfd;
	short	x, y;
{
	struct	mouseposition mouseposition;

	mouseposition.msp_x = x;
	mouseposition.msp_y = y;
	(void)werror(ioctl(windowfd, WINSETMOUSE, &mouseposition), WINSETMOUSE);
	return;
}

#ifdef notdef
win_setcursor(windowfd, cursor_client)
	int	windowfd;
	Cursor	cursor_client;
{
	struct old_cursor *cursor;

	cursor = (struct old_cursor *)(LINT_CAST(cursor_client));
	(void)werror(ioctl(windowfd, WINSETCURSOR, cursor), WINSETCURSOR);
	return;
}

win_getcursor(windowfd, cursor_client)
	int	windowfd;
	Cursor cursor_client;
{
	struct old_cursor *cursor;

	cursor = (struct old_cursor *)(LINT_CAST(cursor_client));
	(void)werror(ioctl(windowfd, WINGETCURSOR, cursor), WINGETCURSOR);
	return;
}

/* These are provided to allow use of the new
 * crosshair features.  This will be changed to the above two
 * procedures when our new kernel is released.
 */
win_set_locator(windowfd, cursor)
	int	windowfd;
	struct	cursor *cursor;
{
	(void)werror(ioctl(windowfd, WINSETLOCATOR, cursor), WINSETLOCATOR);
	return;
}

win_get_locator(windowfd, cursor)
	int	windowfd;
	struct	cursor *cursor;
{
	(void)werror(ioctl(windowfd, WINGETLOCATOR, cursor), WINGETLOCATOR);
	return;
}
#else
win_setcursor(windowfd, cursor)
	int	windowfd;
	struct	cursor *cursor;
{
	(void)werror(ioctl(windowfd, WINSETLOCATOR, cursor), WINSETLOCATOR);
	return;
}

win_getcursor(windowfd, cursor)
	int	windowfd;
	struct	cursor *cursor;
{
	(void)werror(ioctl(windowfd, WINGETLOCATOR, cursor), WINGETLOCATOR);
	return;
}
#endif notdef


win_findintersect(windowfd, x, y)
	int	windowfd;
	short	x, y;
{
	struct	winintersect winintersect;

	winintersect.wi_x = x;
	winintersect.wi_y = y;
	(void)werror(ioctl(windowfd, WINFINDINTERSECT, &winintersect),
	    WINFINDINTERSECT);
	return(winintersect.wi_link);
}

