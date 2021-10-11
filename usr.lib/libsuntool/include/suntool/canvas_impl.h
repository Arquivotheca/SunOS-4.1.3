/*      @(#)canvas_impl.h 1.1 92/07/30 SMI      */

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <sunwindow/window_hs.h>
#include <suntool/window.h>
#include <suntool/scrollbar.h>
#include <suntool/help.h>
#include <suntool/canvas.h>

typedef void	(*Function)();

#define	BIT_FIELD(field)	int field : 1

typedef struct {
    Pixwin	*pw;		/* region of window's pixwin */
    Rect	rect;		/* object left, top, width, height */
    int		depth;		/* retained depth */
    int		margin;		/* view pixwin margin */
    Rect	win_rect;	/* cached window width & height */
    Scrollbar	scrollbars[2];	/* 0 -> vertical, 1 -> horizontal */
    caddr_t	help_data;	/* help data handle */
    Function 	repaint_proc;
    Function 	resize_proc;
    struct {
	BIT_FIELD(retained); 		/* retained region or not */
	BIT_FIELD(width);		/* fixed width */
	BIT_FIELD(height);		/* fixed height */
	BIT_FIELD(auto_clear);		/* auto clear canvas for repaint */
	BIT_FIELD(auto_expand);		/* auto expand canvas with window */
	BIT_FIELD(auto_shrink);		/* auto shrink canvas with window */
	BIT_FIELD(in_view);		/* the last event was in the view pw */
	BIT_FIELD(dirty);		/* screen needs repaint */
	BIT_FIELD(fixed_image);		/* canvas is a fixed size image */
	BIT_FIELD(first_resize);	/* resize proc called at least once */
    } status_bits;
} Canvas_info;

#define	status(canvas, field)		((canvas)->status_bits.field)
#define	status_set(canvas, field)	status(canvas, field) = TRUE
#define	status_reset(canvas, field)	status(canvas, field) = FALSE

#define	win_width(canvas)	((canvas)->win_rect.r_width)
#define	win_height(canvas)	((canvas)->win_rect.r_height)

#define	view_rect(canvas)	\
    ((canvas)->pw->pw_clipdata->pwcd_regionrect)
    
#define	view_left(canvas)	(view_rect(canvas)->r_left)
   
#define	view_top(canvas)	(view_rect(canvas)->r_top)
   
#define	view_width(canvas)	(view_rect(canvas)->r_width)
   
#define	view_height(canvas)	(view_rect(canvas)->r_height)
   

#define	canvas_rect(canvas)	(&(canvas)->rect)

#define canvas_left(canvas)	(canvas)->rect.r_left
#define canvas_top(canvas)	(canvas)->rect.r_top
#define canvas_width(canvas)	(canvas)->rect.r_width
#define canvas_height(canvas)	(canvas)->rect.r_height

#define canvas_set_left(canvas, val)	canvas_left(canvas) = val
#define canvas_set_top(canvas, val)	canvas_top(canvas) = val
#define canvas_set_width(canvas, val)	canvas_width(canvas) = val
#define canvas_set_height(canvas, val)	canvas_height(canvas) = val

#ifdef lint
#define canvas_sb(canvas, direction)   \
	((canvas)->scrollbars[0])
#else
#define canvas_sb(canvas, direction)	\
    ((canvas)->scrollbars[((direction) == SCROLL_VERTICAL) ? 0 : 1])
#endif lint

#define	canvas_x_offset(canvas)		\
    ((canvas)->pw->pw_clipdata->pwcd_x_offset)

#define	canvas_y_offset(canvas)		\
    ((canvas)->pw->pw_clipdata->pwcd_y_offset)

#define	canvas_set_x_offset(canvas, value)	canvas_x_offset(canvas) = value
#define	canvas_set_y_offset(canvas, value)	canvas_y_offset(canvas) = value

#define	canvas_pr(canvas)		((canvas->pw->pw_prretained))
#define	canvas_set_pr(canvas, pr)	canvas_pr(canvas) = pr


/* from canvas_resize.c */
extern int		canvas_resize_pixwin();
extern int		canvas_resize_canvas();
extern void		canvas_inform_resize();

/* from canvas_repaint.c */
extern void		canvas_repaint_window();
extern void		canvas_request_repaint();
extern void		canvas_restrict_clipping();

/* from canvas_scroll.c */
extern void		canvas_scroll();
extern void		canvas_setup_scrollbar();
extern void		canvas_update_scrollbars();
extern void		canvas_open_region();
