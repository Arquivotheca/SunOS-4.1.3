#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)msgsw.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Implements library routines for the msg subwindow
 */

#include <sys/types.h>
#include <sys/time.h>
#include <errno.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_util.h>
#include <pixrect/memvar.h>
#include <pixrect/pixfont.h>
#include <sunwindow/notify.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/pixwin.h>
#include <sunwindow/win_input.h>
#include "sunwindow/sv_malloc.h"
#include <suntool/msgsw.h>
#include <suntool/tool.h>

extern	int errno;

/*ARGSUSED*/
Notify_value
msgsw_event(msgsw, event, arg, type)
	Msgsw *msgsw;
	Event *event;
	Notify_arg arg;
	Notify_event_type type;
{
	switch (event_id(event)) {
	case WIN_REPAINT:
		(void)msgsw_display(msgsw);
		return(NOTIFY_DONE);
	case WIN_RESIZE:
		/* Will clear and reformat on up comming WIN_REPAINT */
		/* VV Fall thru VV */
	default:
		return(NOTIFY_IGNORED);
	}
}

Notify_value
msgsw_destroy(msgsw, status)
	Msgsw *msgsw;
	Destroy_status status;
{
	if (status != DESTROY_CHECKING) {
		(void)win_unregister((Notify_client)(LINT_CAST(msgsw)));
		(void)pw_close(msgsw->msg_pixwin);
		free((char *)(LINT_CAST(msgsw)));
		return(NOTIFY_DONE);
	}
	return(NOTIFY_IGNORED);
}

Msgsw *
msgsw_create(tool, name, width, height, string, font)
	struct	tool *tool;
	char	*name;
	short	width, height;
	char	*string;
	struct	pixfont *font;
{
	struct	toolsw *toolsw;
	Msgsw	*msgsw;

	/* Create subwindow */
	if ((toolsw = tool_createsubwindow(tool, name, width, height)) ==
	    (struct toolsw *)0)
		return(MSGSW_NULL);
	/* Create msg subwindow */
	if ((msgsw = msgsw_init(toolsw->ts_windowfd, string, font)) ==
	    (struct msgsubwindow *)0)
		return(MSGSW_NULL);
	/* Register with window manager */
	if (win_register((Notify_client)(LINT_CAST(msgsw)), 
		msgsw->msg_pixwin, msgsw_event, msgsw_destroy,
	    0))
		return(MSGSW_NULL);
	/* Tool wouldn't wait for window input if no selected routine */
	toolsw->ts_data = (caddr_t) msgsw;
	return(msgsw);
}

struct	msgsubwindow *
msgsw_init(windowfd, string, font)
	int	windowfd;
	char	*string;
	struct	pixfont *font;
{
	struct	msgsubwindow *msgsw;
	char *calloc();

	msgsw = (struct msgsubwindow *) (LINT_CAST(
	    sv_calloc(1, sizeof (struct msgsubwindow))));
	msgsw->msg_windowfd = windowfd;
	if ((msgsw->msg_pixwin = pw_open_monochrome(windowfd)) ==
	    (struct pixwin *)0)
		return((struct  msgsubwindow *)0);
	msgsw->msg_string = string;
	msgsw->msg_font = font;
	(void)win_getsize(msgsw->msg_windowfd, &msgsw->msg_rectcache);
	return(msgsw);
}

msgsw_done(msgsw)
	struct	msgsubwindow *msgsw;
{
	(void) msgsw_destroy(msgsw, DESTROY_CLEANUP);
}

msgsw_handlesigwinch(msgsw)
	struct	msgsubwindow *msgsw;
{
	(void)pw_damaged(msgsw->msg_pixwin);
	(void)pw_donedamaged(msgsw->msg_pixwin);
	(void)msgsw_display(msgsw);
}

msgsw_display(msgsw)
	struct	msgsubwindow *msgsw;
{
	struct	rect rect;

	(void)win_getsize(msgsw->msg_windowfd, &rect);
	(void)pw_writebackground(msgsw->msg_pixwin,
	    0, 0, (int)rect.r_width, (int)rect.r_height, PIX_CLR);
	(void)formatstringtorect(msgsw->msg_pixwin, msgsw->msg_string,
	    msgsw->msg_font, &rect);
	return;
}

msgsw_setstring(msgsw, string)
	struct	msgsubwindow *msgsw;
	char	*string;
{
	msgsw->msg_string = string;
	(void)msgsw_display(msgsw);
}

struct	toolsw *
msgsw_createtoolsubwindow(tool, name, width, height, string, font)
	struct	tool *tool;
	char	*name;
	short	width, height;
	char	*string;
	struct	pixfont *font;
{
	struct	toolsw *toolsw;

	/*
	 * Create subwindow
	 */
	toolsw = tool_createsubwindow(tool, name, width, height);
	/*
	 * Setup msg subwindow
	 */
	if((toolsw->ts_data = (caddr_t) msgsw_init(toolsw->ts_windowfd,
	    string, font)) == (caddr_t)0)
		return((struct  toolsw *)0);
	toolsw->ts_io.tio_handlesigwinch = msgsw_handlesigwinch;
	toolsw->ts_destroy = msgsw_done;
	return(toolsw);
}

