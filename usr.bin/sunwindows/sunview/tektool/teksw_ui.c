#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)teksw_ui.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * User interface for teksw and tek
 *
 * Author: Steve Kleiman
 */

#include <sys/types.h>

#include <stdio.h>
#include <signal.h>
#include <setjmp.h>
#include <errno.h>
#include <sys/termios.h>

#include <suntool/sunview.h>
#include <suntool/canvas.h>
#include <suntool/walkmenu.h>
#include <suntool/scrollbar.h>
#include <suntool/panel.h>
#include <suntool/alert.h>

#include "tek.h"
#include "teksw_imp.h"

#define abs(x)		((x) >= 0? (x): -(x))

/*******************
*
* global data
*
*******************/

static struct pr_size teksize = {	/* tektronix 4014 screen size */
	TXSIZE, TYSIZE
};

static struct ttysize tekttysize[4] = {
	{TEKFONT0ROW, TEKFONT0COL},
	{TEKFONT1ROW, TEKFONT1COL},
	{TEKFONT2ROW, TEKFONT2COL},
	{TEKFONT3ROW, TEKFONT3COL},
};

/* wait cursor */
extern        Pixrect hglass_cursor_mpr;

static struct cursor waitcursor = {
	8, 8,
	PIX_SRC|PIX_DST,
	&hglass_cursor_mpr
};

extern struct pixrect stop_mpr;

static struct cursor stop_cursor = {
	7, 5,
	PIX_SRC|PIX_DST,
	&stop_mpr
};

struct pixfont *tekfont[NFONT];

static void resize();
static void tek_size();
static void tek_putchar();
static void tek_usrinput();
static void drawalphacursor();
static void removealphacursor();
static void tek_blinkscreen();
static int goodpos();
static void goodcursor();
static void badcursor();
static int inimage();
static void imagetotek();
static void tektoimage();

/*
 * Main I/O processing.
 */

/*
 * user input
 */
Notify_value
teksw_event(canvas, ep)
	Window canvas;
	register Event *ep;
{
	register struct teksw *tsp;
	int v;

	tsp = (struct teksw *)window_get(canvas, WIN_CLIENT_DATA);
	if (event_is_ascii(ep)) {
		if (goodpos(tsp, ep))
			tek_kbdinput(tsp->temu_data, event_action(ep));
	} else {
		switch (event_action(ep)) {
		default:
			return (NOTIFY_IGNORED);
		case WIN_RESIZE:
			resize(tsp);
			break;
		case KEY_PAGE_RESET:
			if (event_shift_is_down(ep))
				tek_kbdinput(tsp->temu_data, RESET);
			else
				tek_kbdinput(tsp->temu_data, PAGE);
			break;
		case KEY_COPY:
			tek_kbdinput(tsp->temu_data, COPY);
			break;
		case KEY_RELEASE:
			tek_kbdinput(tsp->temu_data, RELEASE);
			break;
		case ACTION_PROPS:
			teksw_props(tsp);
			break;
		case MS_LEFT:
			if (event_is_down(ep)) {
				if (tsp->uiflags & GCURSORON) {
					if (goodpos(tsp, ep))
					    tek_kbdinput(tsp->temu_data, ' ');
				} else if (tsp->uiflags & PAGEFULL) {
					tek_kbdinput(tsp->temu_data, RELEASE);
				}
			}
			break;
		case MS_RIGHT:
			switch (v = (int)menu_show(tsp->menu, canvas,
					canvas_window_event(canvas, ep), 0)) {
			case COPY:
			case RESET:
			case PAGE:
			case RELEASE:
				tek_kbdinput(tsp->temu_data, v);
				break;
			}
			break;
		case LOC_RGNENTER:
		case LOC_MOVE:
			(void) goodpos(tsp, ep);
			break;
		case LOC_RGNEXIT:
			if ((tsp->uiflags & GCURSORON) &&
			    !(tsp->uiflags & BADCURSOR) &&
			    !inimage(tsp, ep->ie_locx, ep->ie_locy) ) {
				badcursor(tsp, ep->ie_locx, ep->ie_locy);
			}
			break;
		}
	}
	if (tsp->cursormode == ALPHACURSOR && !(tsp->uiflags & ACURSORON)) {
		tsp->uiflags |= ACURSORON;
		drawalphacursor(tsp);
	}
	if (tsp->uiflags & RESTARTPTY) {
		tsp->uiflags &= ~RESTARTPTY;
		(void) teksw_pty_input(tsp, tsp->pty);
	}
	return (NOTIFY_DONE);
}

/*
 * pty input
 */
Notify_value
teksw_pty_input(tsp, fd)
	register struct teksw *tsp;
	int fd;
{
	register char *pbp;
	register int cc;
	extern int errno;

	if (tsp->uiflags & PAGEFULL)
		return (NOTIFY_DONE);
	if (tsp->ptyirp < tsp-> ptyiwp) {
		pbp = tsp->ptyirp;
		cc = tsp->ptyiwp - pbp;
		tsp->ptyirp = tsp->ptyiwp = 0;
	} else {
		pbp = tsp->ptyibuf;
		cc = read(fd, pbp, sizeof (tsp->ptyibuf));
		if (cc < 0) {
			if (errno == EWOULDBLOCK)
				return (NOTIFY_DONE);
			else
				return (NOTIFY_IGNORED);
		} else if (cc > 0) {
			/*
			 * Skip status byte for now.
			 * Should do start/stop here?
			 */
			pbp++, cc--;
		}
	}
	if (cc > 0) {
		pw_batch_on(tsp->pwp);
		while (!(tsp->uiflags & PAGEFULL) && cc-- > 0) {
			tek_ttyinput(tsp->temu_data, *pbp++);
		}
		if (tsp->cursormode == ALPHACURSOR &&
		    !(tsp->uiflags & ACURSORON)) {
			tsp->uiflags |= ACURSORON;
			drawalphacursor(tsp);
		}
		pw_batch_off(tsp->pwp);
		if (tsp->uiflags & PAGEFULL) {
			tsp->ptyiwp = pbp + cc;
			tsp->ptyirp = pbp;
			cc = 0;
		}
	}
	return (cc >= 0? NOTIFY_DONE: NOTIFY_IGNORED);
}

/*ARGSUSED*/
/*
 * pty output
 */
Notify_value
teksw_pty_output(tsp, fd)
	register struct teksw *tsp;
	int fd;
{
	register int cc;

	cc = write(tsp->pty, tsp->ptyorp, tsp->ptyowp - tsp->ptyorp);
	if (cc > 0) {
		tsp->ptyorp += cc;
	}
	if (tsp->ptyorp == tsp->ptyowp) {
		tsp->ptyorp = tsp->ptyowp = tsp->ptyobuf;
		(void) notify_set_output_func(tsp, NOTIFY_FUNC_NULL, tsp->pty);
	}
	return (cc > 0? NOTIFY_DONE: NOTIFY_IGNORED);
}

static void
resize(tsp)
	register struct teksw *tsp;
{

	if (tsp->imagesize.x > (int)window_get(tsp->canvas, WIN_WIDTH)) {
		if (window_get(tsp->canvas, WIN_HORIZONTAL_SCROLLBAR) == NULL)
			window_set(tsp->canvas,
				WIN_HORIZONTAL_SCROLLBAR,
				    scrollbar_create(
					SCROLL_PAGE_BUTTONS, FALSE,
				    0),
			0);
	} else {
		window_set(tsp->canvas,
			WIN_HORIZONTAL_SCROLLBAR, NULL,
		0);
	}
	if (tsp->imagesize.y > (int)window_get(tsp->canvas, WIN_HEIGHT)) {
		if (window_get(tsp->canvas, WIN_VERTICAL_SCROLLBAR) == NULL)
			window_set(tsp->canvas,
				WIN_VERTICAL_SCROLLBAR,
				    scrollbar_create(
					SCROLL_PAGE_BUTTONS, FALSE,
				    0),
			0);
	} else {
		window_set(tsp->canvas,
			WIN_VERTICAL_SCROLLBAR, NULL,
		0);
	}
}

void
teksw_props(tsp)
	register struct teksw *tsp;
{
	register Window panel;
	extern void hide_props();
	extern void props_proc();
	extern char *getenv();

	if (tsp->props != NULL) {
		panel = (Window)window_get(tsp->props, WIN_CLIENT_DATA);
	} else {
		char *cp;

		tsp->props =
		    window_create(tsp->owner, FRAME,
			FRAME_LABEL, "Tektool Properties",
			FRAME_SHOW_LABEL, TRUE,
			0);
		panel = window_create(tsp->props, PANEL, 0);
		window_set(tsp->props,
			WIN_CLIENT_DATA, panel,
			0);
		panel_create_item(panel, PANEL_BUTTON,
			PANEL_LABEL_Y, ATTR_ROW(0),
			PANEL_LABEL_X, ATTR_COL(0),
			PANEL_LABEL_IMAGE,
			    panel_button_image(panel, "DONE", 0, 0),
			PANEL_NOTIFY_PROC, hide_props,
			PANEL_CLIENT_DATA, tsp,
			0);
		tsp->straps_item =
		    panel_create_item(panel, PANEL_TOGGLE,
			PANEL_LABEL_Y, ATTR_ROW(1),
			PANEL_LABEL_X, ATTR_COL(0),
			PANEL_CHOICE_STRINGS,
			    "LF->CR", "CR->LF", "DEL=>LOY",
			    "Echo", "Auto copy", "Local",
			    "Destructive characters",
			    0,
			PANEL_DISPLAY_LEVEL, PANEL_CURRENT,
			PANEL_NOTIFY_PROC, props_proc,
			PANEL_CLIENT_DATA, tsp,
			0);
		tsp->gin_item =
		    panel_create_item(panel, PANEL_CHOICE,
			PANEL_LABEL_Y, ATTR_ROW(2),
			PANEL_LABEL_X, ATTR_COL(0),
			PANEL_LABEL_STRING, "GIN Terminator:",
			PANEL_CHOICE_STRINGS, "NONE", "CR", "CR,EOT", 0,
			PANEL_DISPLAY_LEVEL, PANEL_CURRENT,
			PANEL_NOTIFY_PROC, props_proc,
			PANEL_CLIENT_DATA, tsp,
			0);
		tsp->marg_item =
		    panel_create_item(panel, PANEL_CHOICE,
			PANEL_LABEL_Y, ATTR_ROW(2),
			PANEL_LABEL_STRING, "Page Full:",
			PANEL_CHOICE_STRINGS, "OFF", "MARGIN 1", "MARGIN 2", 0,
			PANEL_DISPLAY_LEVEL, PANEL_CURRENT,
			PANEL_NOTIFY_PROC, props_proc,
			PANEL_CLIENT_DATA, tsp,
			0);
		tsp->copy_item =
		    panel_create_item(panel, PANEL_TEXT,
			PANEL_LABEL_Y, ATTR_ROW(3),
			PANEL_LABEL_X, ATTR_COL(0),
			PANEL_LABEL_STRING, "Copy:",
			PANEL_VALUE,
			    ((cp = getenv("TEKCOPY"))? cp: DEFAULTCOPY),
			0);
		window_fit(panel);
		window_fit(tsp->props);
	}
	panel_set_value(tsp->straps_item, tsp->straps);
	switch (tsp->straps & GINTERM) {
	case GINNONE:
		panel_set_value(tsp->gin_item, 0);
		break;
	case GINCR:
		panel_set_value(tsp->gin_item, 1);
		break;
	case GINCRE:
		panel_set_value(tsp->gin_item, 2);
		break;
	}
	switch (tsp->straps & MARGIN) {
	case MARGOFF:
		panel_set_value(tsp->marg_item, 0);
		break;
	case MARG1:
		panel_set_value(tsp->marg_item, 1);
		break;
	case MARG2:
		panel_set_value(tsp->marg_item, 2);
		break;
	}
	window_set(tsp->props, WIN_SHOW, TRUE, 0);
}

/*ARGSUSED*/
static void
hide_props(item, ep)
	Panel_item item;
	Event *ep;
{
	register struct teksw *tsp;

	tsp = (struct teksw *)panel_get(item, PANEL_CLIENT_DATA);
	window_set(tsp->props, WIN_SHOW, FALSE, 0);
}

/*ARGSUSED*/
static void
props_proc(item, value, ep)
	Panel_item item;
	unsigned int value;
	Event *ep;
{
	register struct teksw *tsp;

	tsp = (struct teksw *)panel_get(item, PANEL_CLIENT_DATA);
	if (item == tsp->straps_item) {
		tsp->straps &=
		    ~(LFCR|CRLF|DELLOY|AECHO|ACOPY|LOCAL|CRT_CHARS);
		tsp->straps |= value;
	} else if (item == tsp->gin_item) {
		tsp->straps &= ~GINTERM;
		switch(value) {
		case 1:
			tsp->straps |= GINCR;
			break;
		case 2:
			tsp->straps |= GINCRE;
			break;
		}
	} else {
		tsp->straps &= ~MARGIN;
		switch(value) {
		case 1:
			tsp->straps |= MARG1;
			break;
		case 2:
			tsp->straps |= MARG2;
			break;
		}
	}
	tek_set_straps(tsp->temu_data, tsp->straps);
}

/*
 * Pop up an alert with a string
 */
static void
confirm(tsp, s)
	register struct teksw *tsp;
	char *s;
{
	(void) alert_prompt(tsp->owner, NULL,
		ALERT_MESSAGE_STRINGS, s, 0,
		ALERT_BUTTON_YES, "Confirm",
		0);
}

static int
goodpos(tsp, ep)
	register struct teksw *tsp;
	register Event *ep;
{
	struct pr_pos tpos;

	if (tsp->uiflags & GCURSORON) {
		if (inimage(tsp, ep->ie_locx, ep->ie_locy)) {
			tpos.x = ep->ie_locx;
			tpos.y = ep->ie_locy;
			if (tsp->uiflags & BADCURSOR) {
				goodcursor(tsp);
			}
			imagetotek(tsp, &tpos);
			tek_posinput(tsp->temu_data, tpos.x, tpos.y);
		} else if (!(tsp->uiflags & BADCURSOR)) {
			badcursor(tsp, ep->ie_locx, ep->ie_locy);
			return (0);
		}
	}
	return (1);
}

static void
badcursor(tsp, x, y)
	register struct teksw *tsp;
	register int x, y;
{
	register int flags;

	cursor_set(tsp->cursor,
		CURSOR_SHOW_CURSOR, TRUE,
		CURSOR_SHOW_CROSSHAIRS, FALSE,
		0);
	window_set(tsp->canvas,
		WIN_CURSOR, tsp->cursor,
		0);
	flags = 0;
	if (x < WXMIN(tsp)) {
		flags |= BADCURS_XMIN;
		x = WXMIN(tsp);
	} else if (x >= WXMAX(tsp)) {
		flags |= BADCURS_XMAX;
		x = WXMAX(tsp);
	}
	if (flags) {
		pw_vector(tsp->pwp,
			x, WYMIN(tsp),
			x, WYMAX(tsp),
			(PIX_SRC ^ PIX_DST), 1);
	}
	if (y < WYMIN(tsp)) {
		flags |= BADCURS_YMIN;
		y = WYMIN(tsp);
	} else if (y >= WYMAX(tsp)) {
		flags |= BADCURS_YMAX;
		y = WYMAX(tsp);
	}
	if (flags & (BADCURS_YMIN|BADCURS_YMAX)) {
		pw_vector(tsp->pwp,
			WXMIN(tsp), y,
			WXMAX(tsp), y,
			(PIX_SRC ^ PIX_DST), 1);
	}
	tsp->uiflags |= flags;
}

static void
goodcursor(tsp)
	register struct teksw *tsp;
{
	register int flags;
	register int a;

	flags = tsp->uiflags & BADCURSOR;

	if (flags & (BADCURS_XMIN|BADCURS_XMAX)) {
		a = ((flags & BADCURS_XMIN)? WXMIN(tsp): WXMAX(tsp));
		pw_vector(tsp->pwp,
			a, WYMIN(tsp),
			a, WYMAX(tsp),
			(PIX_SRC ^ PIX_DST), 1);
	}
	if (flags & (BADCURS_YMIN|BADCURS_YMAX)) {
		a = ((flags & BADCURS_YMIN)? WYMIN(tsp): WYMAX(tsp));
		pw_vector(tsp->pwp,
			WXMIN(tsp), a,
			WXMAX(tsp), a,
			(PIX_SRC ^ PIX_DST), 1);
	}
	tsp->uiflags &= ~BADCURSOR;
	if (tsp->uiflags & GCURSORON) {
		cursor_set(tsp->cursor,
			CURSOR_SHOW_CURSOR, FALSE,
			CURSOR_SHOW_CROSSHAIRS, TRUE,
			0);
		window_set(tsp->canvas,
			WIN_CURSOR, tsp->cursor,
			0);
	}
}

/*****************************************************************************
}

/*****************************************************************************
 *
 * Tek emulator interface routines
 *
 *****************************************************************************/

void
tek_move(td, x, y)
	caddr_t td;
	int x, y;
{
	register struct teksw *tsp;

	tsp = (struct teksw *) td;
	tsp->curpos.x = x;
	tsp->curpos.y = y;
	tektoimage(tsp, &tsp->curpos);
	if (tsp->uiflags & ACURSORON) {
		tsp->uiflags &= ~ACURSORON;
		removealphacursor(tsp);
	}
}

void
tek_draw(td, x, y)
	caddr_t td;
	int x, y;
{
	register struct teksw *tsp;
	struct pr_pos tpos;

	tsp = (struct teksw *) td;
	tpos.x = x;
	tpos.y = y;
	tektoimage(tsp, &tpos);
	if (tsp->type == VT_WRITETHRU) {
		tek_line(tsp,
			 tsp->curpos.x, tsp->curpos.y,
			 tpos.x, tpos.y,
			 (PIX_SRC ^ PIX_DST),
			 0,
			 tsp->style);
		pw_show(tsp->pwp);
		tek_line(tsp,
			 tsp->curpos.x, tsp->curpos.y,
			 tpos.x, tpos.y,
			 (PIX_SRC ^ PIX_DST),
			 0,
			 tsp->style);
	} else {
		tek_line(tsp,
			 tsp->curpos.x, tsp->curpos.y,
			 tpos.x, tpos.y,
			 PIX_SRC,
			 (tsp->type == VT_DEFOCUSED? 1: 0),
			 tsp->style);
	}
	tsp->curpos = tpos;
}

void
tek_char(td, c)
	caddr_t td;
	register unsigned char c;
{
	register struct teksw *tsp;

	tsp = (struct teksw *) td;
	/*
	 * turn off cursor while characters are being displayed
	 */
	if (tsp->uiflags & ACURSORON) {
		tsp->uiflags &= ~ACURSORON;
		removealphacursor(tsp);
	}
	c &= 0xff;
	if (tsp->type == VT_WRITETHRU) {
		pw_char(tsp->pwp,
			tsp->curpos.x,
			tsp->curpos.y,
			(PIX_SRC ^ PIX_DST),
			tsp->curfont,
			c);
		pw_show(tsp->pwp);
		pw_char(tsp->pwp,
			tsp->curpos.x,
			tsp->curpos.y,
			(PIX_SRC ^ PIX_DST),
			tsp->curfont,
			c);
	} else {
		pw_char(tsp->pwp,
			tsp->curpos.x,
			tsp->curpos.y,
			(tsp->straps & CRT_CHARS) ?
			    PIX_SRC :
			    (PIX_SRC | PIX_DST),
			tsp->curfont,
			c);
	}
}

void
tek_ttyoutput(td, c)
	caddr_t td;
	char c;
{
	register struct teksw *tsp;

	tsp = (struct teksw *) td;
	if (tsp->ptyorp == tsp->ptyowp) {
		notify_set_output_func(tsp, teksw_pty_output, tsp->pty);
	}
	*tsp->ptyowp++ = c;
	if (tsp->ptyowp >= &tsp->ptyobuf[sizeof (tsp->ptyobuf)]) {
		tsp->ptyowp--;
	}
}

void
tek_displaymode(td, style, type)
	caddr_t td;
	enum vstyle style;
	enum vtype type;
{
	register struct teksw *tsp;

	tsp = (struct teksw *) td;
	tsp->style = style;
	tsp->type = type;
}

void
tek_chfont(td, f)
	caddr_t td;
	int f;
{
	register struct teksw *tsp;

	tsp = (struct teksw *) td;
	tsp->curfont = tekfont[f];
	(void) ioctl(tsp->tty, TIOCSSIZE, &tekttysize[f]);
	if (tsp->uiflags & ACURSORON) {
		tsp->uiflags &= ~ACURSORON;
		removealphacursor(tsp);
	}
}

void
tek_cursormode(td, cmode)
	caddr_t td;
	register enum cursormode cmode;
{
	register struct teksw *tsp;

	tsp = (struct teksw *) td;
	switch (cmode) {
	case NOCURSOR:
		if (tsp->uiflags & ACURSORON) {
			tsp->uiflags &= ~ACURSORON;
			removealphacursor(tsp);
		}
		if (tsp->uiflags & GCURSORON) {
			tsp->uiflags &= ~GCURSORON;
			if (tsp->uiflags & BADCURSOR) {
				goodcursor(tsp);
			}
			cursor_set(tsp->cursor,
				CURSOR_SHOW_CURSOR, TRUE,
				CURSOR_SHOW_CROSSHAIRS, FALSE,
				0);
			window_set(tsp->canvas,
				WIN_CURSOR, tsp->cursor,
				WIN_IGNORE_PICK_EVENT, WIN_IN_TRANSIT_EVENTS,
				0);
		}
		break;
	case ALPHACURSOR:
		if (tsp->uiflags & GCURSORON) {
			tsp->uiflags &= ~GCURSORON;
			if (tsp->uiflags & BADCURSOR) {
				goodcursor(tsp);
			}
			cursor_set(tsp->cursor,
				CURSOR_SHOW_CURSOR, TRUE,
				CURSOR_SHOW_CROSSHAIRS, FALSE,
				0);
			window_set(tsp->canvas,
				WIN_CURSOR, tsp->cursor,
				WIN_IGNORE_PICK_EVENT, WIN_IN_TRANSIT_EVENTS,
				0);
		}
		if (!(tsp->uiflags & ACURSORON)) {
			tsp->uiflags |= ACURSORON;
			drawalphacursor(tsp);
		}
		break;
	case GFXCURSOR:
		if (tsp->uiflags & ACURSORON) {
			tsp->uiflags &= ~ACURSORON;
			removealphacursor(tsp);
		}
		if (!(tsp->uiflags & GCURSORON)) {
			tsp->uiflags |= GCURSORON;
			cursor_set(tsp->cursor,
				CURSOR_SHOW_CURSOR, FALSE,
				CURSOR_SHOW_CROSSHAIRS, TRUE,
				0);
			window_set(tsp->canvas,
				WIN_CURSOR, tsp->cursor,
				WIN_CONSUME_PICK_EVENT, WIN_IN_TRANSIT_EVENTS,
				0);
		}
		break;
	}
	tsp->cursormode = cmode;
}

void
tek_clearscreen(td)
	caddr_t td;
{
	register struct teksw *tsp;

	tsp = (struct teksw *) td;
	pw_writebackground(tsp->pwp,
		0, 0, tsp->imagesize.x, tsp->imagesize.y,
		PIX_CLR);
	if (tsp->uiflags & ACURSORON)
		drawalphacursor(tsp);
}

void
tek_bell(td)
	caddr_t td;
{
	register struct teksw *tsp;

	tsp = (struct teksw *) td;
	window_bell(tsp->canvas);
}

void
tek_makecopy(td)
	caddr_t td;
{
	register struct teksw *tsp;
	char *cmd;
	FILE *copy;
	void (*s)();
	extern FILE *popen();
	extern char *getenv();

	tsp = (struct teksw *) td;

	if (tsp->props != NULL)
		cmd = (char *)panel_get_value(tsp->copy_item);
	else
		cmd = getenv("TEKCOPY");
	if (cmd == NULL || *cmd == '\0')
		cmd = DEFAULTCOPY;
	if ((copy = popen(cmd, "w")) == NULL) {
		confirm(tsp, "cannot fork copy command");
		return;
	}
	window_set(tsp->canvas,
		WIN_CURSOR, &waitcursor,
		0);
	if (pr_dump(tsp->pwp->pw_prretained, copy, NULL, RT_BYTE_ENCODED, 1)) {
		confirm(tsp, "copy command failed");
	}
	pclose(copy);
	if (tsp->uiflags & PAGEFULL) {
		window_set(tsp->canvas,
			WIN_CURSOR, &stop_cursor,
			0);
	} else {
		window_set(tsp->canvas,
			WIN_CURSOR, tsp->cursor,
			0);
	}
}

void
tek_pagefull_on(td)
	caddr_t td;
{
	register struct teksw *tsp;

	tsp = (struct teksw *) td;
	tsp->uiflags |= PAGEFULL;
	notify_set_input_func(tsp, NOTIFY_FUNC_NULL, tsp->pty);
	window_set(tsp->canvas,
		WIN_CURSOR, &stop_cursor,
		0);
}

void
tek_pagefull_off(td)
	caddr_t td;
{
	register struct teksw *tsp;

	tsp = (struct teksw *) td;
	tsp->uiflags &= ~PAGEFULL;
	tsp->uiflags |= RESTARTPTY;
	notify_set_input_func(tsp, teksw_pty_input, tsp->pty);
	window_set(tsp->canvas,
		WIN_CURSOR, tsp->cursor,
		0);
}

/*
 * support routines
 */

static void
drawalphacursor(tsp)
	register struct teksw *tsp;
{
	tsp->alphacursorpos = tsp->curpos;
	tsp->alphacursorsize = tsp->curfont->pf_defaultsize;
	tsp->alphacursorpos.y += tsp->curfont->pf_char['A'].pc_home.y;
	pw_writebackground(tsp->pwp,
		tsp->alphacursorpos.x, tsp->alphacursorpos.y,
		tsp->alphacursorsize.x, tsp->alphacursorsize.y,
		PIX_NOT(PIX_DST));
}

static void
removealphacursor(tsp)
	register struct teksw *tsp;
{
	pw_writebackground(tsp->pwp,
		tsp->alphacursorpos.x, tsp->alphacursorpos.y,
		tsp->alphacursorsize.x, tsp->alphacursorsize.y,
		PIX_NOT(PIX_DST));
}

/*
 * Tek line drawing routine.
 * Calculations are done in fixed point arithmetic.
 */
#define HALF		(1 << 9)			/* 1/2 in fixed point */
#define FIX(X)		((long)((X) << 10))		/* int to fixed pt */
#define UNFIX(X)	((int)(((X) + HALF) >> 10))	/* fixed pt to int */

/*
 * vector styles (out of the 4014 hardware manual)
 */
#define DOT	3		/* line style dot in tekpts (guess) */

/*
 * One pixel is subtracted from the on pattern beacause vector drawing includes
 * the end pixel. The off pattern is incremented for the same reason.
 */
#define ON(X)		FIX(((X) * DOT) - 1)
#define OFF(X)		FIX(((X) * DOT) + 1)

static long pattern[5][5] = {
	0, 0, 0, 0, 0,					/* solid */
	ON(1), OFF(1), ON(1), OFF(1), FIX(4 * DOT),	/* dotted */
	ON(5), OFF(1), ON(1), OFF(1), FIX(8 * DOT),	/* dash dot */
	ON(3), OFF(1), ON(3), OFF(1), FIX(8 * DOT),	/* short dash */
	ON(6), OFF(2), ON(6), OFF(2), FIX(16 * DOT)	/* long dash */
};

#undef DOT
#undef ON
#undef OFF

static long psx[4], psy[4];		/* pattern length vector */
static long lsx, lsy;			/* last segement remainder x,y */
static int lstyle = 0;
static int lx, ly;
static long patrem;			/* pattern remainder */
static char lps;
static char cont;			/* continuation vector flag */
static char width;
static int rop;
static struct pixwin *destpwp;

static void texturedline();
static void plotline();
static void dot();
static int length();
static int isqrt();

tek_line(tsp, x0, y0, x1, y1, op, w, style)
	register struct teksw *tsp;
	register int x0, y0, x1, y1;	/* window coords */
	int op;
	int w;				/* line width */
	int style;
{
	register int dx, dy;		/* vector components */

	rop = op | PIX_COLOR(-1);
	destpwp = tsp->pwp;
	cont = (style && lstyle == style && lx == x0 && ly == y0 && patrem);
	width = w;
	dx = x1 - x0;
	dy = y1 - y0;
	lstyle = style;
	lx = x1;
	ly = y1;

	if ((dx == 0) && (dy == 0)) {
		/*
		 * single point
		 */
		if (cont) {
			register long pl;	/* piece length */
			register long *pp;
			register long segrem;

			pp = &pattern[style][0];
			segrem = patrem;
			for (lps = 0; lps < 4; lps++) {
				pl = pp[lps];
				if (segrem < pl) {
					break;
				}
				segrem -= pl;
			}
			if ((lps & 1) == 0)
				dot(x0, y0);
		} else {
			dot(x0, y0);
		}
		return (0);
	}
	if (style) {
		register long vecl;	/* vector length */
		register long pl;	/* piece length */
		register long *pp;
		register int i;

		/*
		 * textured line
		 * compute the x and y projections of the texture pattern
		 * placed on the line
		 */
		vecl = length(dx, dy);
		/* make four part pattern cycle */
		pp = &pattern[style][0];
		for (i = 0; i < 4; i++) {
			pl = pp[i];
			psx[i] = (dx * pl) / vecl;
			psy[i] = (dy * pl) / vecl;
		}
		if (cont) {
			register long segrem;

			segrem = patrem;
			for (lps = 0; lps < 4; lps++) {
				pl = pp[lps];
				if (segrem < pl) {
					segrem = pl - segrem;
					break;
				}
				segrem -= pl;
			}
			lsx = (dx * segrem) / vecl;
			lsy = (dy * segrem) / vecl;
			patrem = (FIX(vecl) + patrem) % pp[4];
		} else {
			patrem = FIX(vecl) % pp[4];
		}
	} else {
		/*
		 * solid line
		 */
		psx[0] = FIX(dx);
		psy[0] = FIX(dy);
		patrem = 0;
	}
	texturedline(x0, y0, x1, y1);
	return (0);
}

static void
texturedline(x0, y0, x1, y1)
	int x0, y0, x1, y1;
{
	register long dx, dy;		/* line length counters */
	register long sx, sy;		/* x, y segment length */
	register long xb, yb, xe, ye;	/* begin and end pts */
	register int ps;		/* current segment number */

	xb = xe = FIX(x0);
	yb = ye = FIX(y0);
	dx = FIX(abs(x1 - x0));
	dy = FIX(abs(y1 - y0));
	if (cont) {
		sx = lsx;
		sy = lsy;
		ps = lps;
	} else {
		ps = 0;
		sx = psx[0];
		sy = psy[0];
	}
	while (dx >= abs(sx) && dy >= abs(sy)) {
		xe += sx;
		ye += sy;
		dx -= abs(sx);
		dy -= abs(sy);
		if ((ps & 1) == 0) {
			plotline(UNFIX(xb), UNFIX(yb), UNFIX(xe), UNFIX(ye));
		}
		ps = (ps + 1) & 03;	/* four part pattern cycle */
		sx = psx[ps];
		sy = psy[ps];
		xb = xe;
		yb = ye;
	}
	if (dx != 0 || dy != 0) {
		/*
		 * finish off segment
		 */
		if ((ps & 1) == 0) {
			plotline(UNFIX(xe), UNFIX(ye), x1, y1);
		}
	}
}

/*
 * Fat line end pixrect.
 */
static short fat_end[] = {0x4000, 0xe000, 0x4000};
mpr_static(fat_end_mpr, 3, 3, 1, fat_end);

static void
plotline(x0, y0, x1, y1)
	int x0, y0, x1, y1;
{

	if (width) {
		register struct pr_pos *plp;
		register int rx0, ry0, rx1, ry1;
		register int dx, dy;

		rx0 = x0; ry0 = y0; rx1 = x1; ry1 = y1;
		pw_vector(destpwp, rx0, ry0, rx1, ry1, rop, 1);
		dx = rx1 - rx0;
		dy = ry1 - ry0;
		if (abs(dx) < abs(dy)) {
			pw_vector(destpwp, rx0+1, ry0, rx1+1, ry1, rop, 1);
			pw_vector(destpwp, rx0-1, ry0, rx1-1, ry1, rop, 1);
		} else {
			pw_vector(destpwp, rx0, ry0+1, rx1, ry1+1, rop, 1);
			pw_vector(destpwp, rx0, ry0-1, rx1, ry1-1, rop, 1);
		}
		pw_stencil(destpwp, rx0-1, ry0-1, 3, 3,
			   rop,
			   &fat_end_mpr, 0, 0,
			   NULL, 0, 0);
		pw_stencil(destpwp, rx1-1, ry1-1, 3, 3,
			   rop,
			   &fat_end_mpr, 0, 0,
			   NULL, 0, 0);
	} else {
		pw_vector(destpwp, x0, y0, x1, y1, rop, 1);
	}
}

/*
 * Fat dot pixrect.
 */
static short fat_dot[] = {0xe000, 0xe000, 0xe000};
mpr_static(fat_dot_mpr, 3, 3, 1, fat_dot);

static void
dot(x0, y0)
	register short x0, y0;
{
	if (width) {
		pw_stencil(destpwp, x0-1, y0-1, 3, 3,
			   rop,
			   &fat_dot_mpr, 0, 0,
			   NULL, 0, 0);
	} else {
		pw_rop(destpwp, x0, y0, 1, 1, rop, NULL, 0, 0);
	}
}

/*
 * Length and integer square root.
 */

static int
length(dx, dy)
{
	if (dx == 0)
		return (abs(dy));
	if (dy == 0)
		return (abs(dx));
	return (isqrt((dx * dx) + (dy * dy)));
}

static int
isqrt(n)
	register int n;
{
	register int q, r, x2, x;
	register int t;

	if (n < 0)
		abort();
	if (n < 2)
		return (n);		/* or this case */
	t = x = n;
	while (t >>= 2)
		x >>= 1;
	x++;
	while (1) {
		q = n / x;
		r = n % x;
		if (x <= q) {
			x2 = x + 2;
			if (q < x2 || q == x2 && r == 0)
				break;
		}
		x = (x + q) >> 1;
	}
	return (x);
}

#undef HALF
#undef FIX
#undef UNFIX

#define scale(pp, fs, ts) \
	(pp)->x = \
	    ( ((long)(pp)->x * (long)ts.x) + ((long)fs.x / 2) ) / (long)fs.x; \
	(pp)->y = \
	    ( ((long)(pp)->y * (long)ts.y) + ((long)fs.y / 2) ) / (long)fs.y;

static int
inimage(tsp, x, y)
	register struct teksw *tsp;
	register int x, y;
{

	return (x >= WXMIN(tsp) && x < WXMAX(tsp) &&
		    y >= WYMIN(tsp) && y < WYMAX(tsp));
}

static void
imagetotek(tsp, posp)
	register struct teksw *tsp;
	struct pr_pos *posp;
{
	posp->x -= WXMIN(tsp);
	posp->y -= WYMIN(tsp);
	scale(posp, tsp->scalesize, teksize);
	posp->y = teksize.y - posp->y;
}

static void
tektoimage(tsp, posp)
	register struct teksw *tsp;
	struct pr_pos *posp;
{
	posp->y = teksize.y - posp->y;
	scale(posp, teksize, tsp->scalesize);
	posp->x += WXMIN(tsp);
	posp->y += WYMIN(tsp);
}
