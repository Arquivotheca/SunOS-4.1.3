#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)emptysw.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Implements library routines for the full screen access facility
 */

#include <sys/types.h>
#include <sys/time.h>
#include <errno.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_util.h>
#include <pixrect/memvar.h>
#include <sunwindow/notify.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/pixwin.h>
#include <sunwindow/win_input.h>
#include <sunwindow/win_struct.h>
#include <sunwindow/win_cursor.h>
#include "sunwindow/sv_malloc.h"
#include <suntool/emptysw.h>
#include <suntool/tool.h>

extern	int errno;

static	short esw_cursorimage[15] = {
	0x0100,	/*       #       */
	0x0100,	/*       #       */
	0x0100,	/*       #       */
	0x0100,	/*       #       */
	0x0100,	/*       #       */
	0x0100,	/*       #       */
	0x0000,	/*               */
	0xfc7e,	/*######   ######*/
	0x0000,	/*               */
	0x0100,	/*       #       */
	0x0100,	/*       #       */
	0x0100,	/*       #       */
	0x0100,	/*       #       */
	0x0100,	/*       #       */
	0x0100,	/*       #       */
};
mpr_static(esw_mpr, 16, 15, 1, esw_cursorimage);
struct	cursor esw_cursor = { 7, 7, PIX_SRC^PIX_DST, &esw_mpr};

short	_esw_imagegray[16]={
	0xaaaa,
	0x5555,
	0xaaaa,
	0x5555,
	0xaaaa,
	0x5555,
	0xaaaa,
	0x5555,
	0xaaaa,
	0x5555,
	0xaaaa,
	0x5555,
	0xaaaa,
	0x5555,
	0xaaaa,
	0x5555
	};
mpr_static(_esw_pixrectgray, 16, 16, 1, _esw_imagegray);


/*ARGSUSED*/
static Notify_value
esw_event(esw, event, arg, type)
	Emptysw *esw;
	Event *event;
	Notify_arg arg;
	Notify_event_type type;
{
	switch (event_id(event)) {
	case WIN_REPAINT:
		esw_display(esw);
		return(NOTIFY_DONE);
	case WIN_RESIZE:
		/* Will clear and reformat on up comming WIN_REPAINT */
		/* VV Fall thru VV */
	default:
		return(NOTIFY_IGNORED);
	}
}

static Notify_value
esw_destroy(esw, status)
	Emptysw *esw;
	Destroy_status status;
{
	if (status != DESTROY_CHECKING) {
		(void)win_unregister((Notify_client)(LINT_CAST(esw)));
		(void)pw_close(esw->em_pixwin);
		free((char *)(LINT_CAST(esw)));
		return(NOTIFY_DONE);
	}
	return(NOTIFY_IGNORED);
}


/*ARGSUSED*/
Emptysw *
esw_begin(tool, name, width, height, string, font)
	struct	tool *tool;
	char	*name;
	short	width, height;
	char	*string;
	struct	pixfont *font;
{
	struct	toolsw *toolsw;
	Emptysw	*esw;

	/* Create subwindow */
	if ((toolsw = tool_createsubwindow(tool, name, width, height)) ==
	    (struct toolsw *)0)
		return((Emptysw *) 0);
	/* Create empty subwindow */
	if ((esw = esw_init(toolsw->ts_windowfd)) ==
	    (Emptysw *)0)
		return((Emptysw *) 0);
	/* Register with window manager */
	if (win_register((Notify_client)(LINT_CAST(esw)), 
		esw->em_pixwin, esw_event, esw_destroy, 0))
		return((Emptysw *) 0);
	/* Tool wouldn't wait for window input if no selected routine */
	toolsw->ts_data = (caddr_t) esw;
	return(esw);
}

struct	emptysubwindow *
esw_init(windowfd)
	int	windowfd;
{
	struct	emptysubwindow *esw;

	esw = (struct emptysubwindow *) (LINT_CAST(
	    sv_calloc(1, sizeof (struct emptysubwindow))));
	esw->em_windowfd = windowfd;
	if ((esw->em_pixwin = pw_open_monochrome(windowfd))==(struct pixwin *)0)
		return((struct  emptysubwindow *)0);
	(void)win_setcursor(windowfd, &esw_cursor);
	return(esw);
}

static
esw_should_not_paint(esw)
	Emptysw *esw;
{
	int	mypid = getpid(), ownerpid = win_getowner(esw->em_windowfd);
	struct	inputmask im, imflush;

	if (mypid != ownerpid) {
		if (ownerpid == 0) {
			/*
			 * Take back window because ownerpid doesn't exit.
			 */
			(void)win_setowner(esw->em_windowfd, mypid);
			/*
			 * Reset input mask and flush input buffer
			 */
			(void)input_imnull(&im);
			imflush = im;
			imflush.im_flags |= IM_ASCII;
			(void)win_setinputmask(esw->em_windowfd, &im, &imflush,
			     WIN_NULLLINK);
			(void)win_setcursor(esw->em_windowfd, &esw_cursor);
		} else
			/*
			 * Ownerpid exists so do nothing.
			 */
			return (1);
	}
	return (0);
}

static
esw_display(esw)
	Emptysw *esw;
{
	struct	rect rect;

	if (esw_should_not_paint(esw))
		return;
	(void)win_getsize(esw->em_windowfd, &rect);
	(void)pw_replrop(esw->em_pixwin, 0, 0,
	    rect.r_width, rect.r_height, PIX_SRC, &_esw_pixrectgray, 0, 0);
	return;
}


esw_done(esw)
	Emptysw *esw;
{
	(void)pw_close(esw->em_pixwin);
	free((char *)(LINT_CAST(esw)));
}


/* Non-notifier based code. */

esw_handlesigwinch(esw)
	Emptysw *esw;
{
	struct	rect rect;

	if (esw_should_not_paint(esw))
		return;
	(void)win_getsize(esw->em_windowfd, &rect);
	(void)pw_damaged(esw->em_pixwin);
	(void)pw_replrop(esw->em_pixwin, 0, 0,
	    rect.r_width, rect.r_height, PIX_SRC, &_esw_pixrectgray, 0, 0);
	(void)pw_donedamaged(esw->em_pixwin);
	return;
}

struct	toolsw *
esw_createtoolsubwindow(tool, name, width, height)
	struct	tool *tool;
	char	*name;
	short	width, height;
{
	struct	toolsw *toolsw;

	/*
	 * Create subwindow
	 */
	toolsw = tool_createsubwindow(tool, name, width, height);
	/*
	 * Setup empty subwindow
	 */
	if ((toolsw->ts_data = (caddr_t) esw_init(toolsw->ts_windowfd)) ==
	    (caddr_t)0)
		return((struct  toolsw *)0);
	toolsw->ts_io.tio_handlesigwinch = esw_handlesigwinch;
	toolsw->ts_destroy = esw_done;
	return(toolsw);
}
