/*
 * @(#)WS_macros.h 1.1 92/07/30 Copyr 1989, 1990 Sun Micro
 */
 
/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */
/*
 *	Sun Microsystems, inc.
 *	Graphics Products Division
 *
 *	Date:	1989
 *	Author:	Patrick-Gilles Maillot
 *
 */


#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <xview/xview.h>
#include <xview/canvas.h>
#include <xview/textsw.h>
#include <xview/panel.h>
#include <xview/cms.h>
#include <xview/xv_xrect.h>
#include <xview/alert.h>

static	Xgl_X_window 	xgl_x_win;
static	Xv_Window 	pw;
static	Display		*display;
static	Window		frame_window;
static	Window		canvas_window;

#define INIT_X11_WG(MY_frame, MY_canvas, MY_event_proc, MY_repaint_proc, MY_resize_proc){\
        Atom	catom;							\
	void	MY_event_proc(), MY_repaint_proc(), MY_resize_proc();	\
									\
        display = (Display *)xv_get(MY_frame, XV_DISPLAY);		\
									\
	pw = (Xv_Window)canvas_paint_window(MY_canvas);			\
	canvas_window = (Window)xv_get(pw, XV_XID);			\
									\
	frame_window = (Window)xv_get(MY_frame, XV_XID);		\
									\
	xv_set(pw, WIN_EVENT_PROC, MY_event_proc, 0);			\
	xv_set(MY_canvas, CANVAS_REPAINT_PROC, MY_repaint_proc,		\
		CANVAS_RESIZE_PROC, MY_resize_proc,			\
		0);							\
									\
	catom = XInternAtom(display, "WM_COLORMAP_WINDOWS", False);	\
	XChangeProperty(display, frame_window, catom, XA_WINDOW, 32, 	\
		PropModeAppend, &canvas_window, 1);			\
									\
	xgl_x_win.X_display = (void *)XV_DISPLAY_FROM_WINDOW(pw);	\
	xgl_x_win.X_window = (Xgl_usgn32)canvas_window;			\
	xgl_x_win.X_screen = (int)DefaultScreen(display);		\
									\
	sleep(2);							\
}

#define EVENT_X11_WG(MY_ras,MY_canvas,MY_event_proc)			\
  void									\
  MY_event_proc(window, event, arg)					\
  Xv_Window	window;							\
  Event		*event;							\
  Notify_arg	arg;							\
  {									\
	switch(event_action(event)) {					\
		break;							\
		default: break;						\
	}								\
  }

#define REPAINT_X11_WG(MY_ras,MY_canvas,MY_repaint_proc,MY_draw_proc)	\
  void									\
  MY_repaint_proc(local_canvas, local_pw, dpy, xwin, xrects)		\
  Canvas          local_canvas;						\
  Xv_Window       local_pw;						\
  Display         *dpy;							\
  Window          xwin;							\
  Xv_xrectlist    *xrects;						\
  {									\
    Xv_Window	w;							\
									\
    	CANVAS_EACH_PAINT_WINDOW(MY_canvas, w)				\
        	if (w == local_pw)					\
            	break;							\
    	CANVAS_END_EACH							\
									\
        MY_draw_proc();							\
  }

#define RESIZE_X11_WG(MY_ras,MY_canvas,MY_resize_proc)			\
  void									\
  MY_resize_proc(local_canvas, width, height)				\
  Canvas	local_canvas;						\
  int	 	width, height;						\
  {									\
	xgl_window_raster_resize(MY_ras);			\
  }

/* this should force the window the be displayed, but it doesn't work????? */
#define EXPOSE_WIN(MY_frame, XD) {				\
	xv_set(MY_frame, XV_SHOW, TRUE, 0);			\
	XFlush(XD);						\
}


