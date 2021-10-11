/***************************************************************************/
#ifndef lint
static char sccsid[] = "@(#)resize_demo.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif
/***************************************************************************/

#include <suntool/sunview.h>
#include <suntool/canvas.h>
#include <suntool/scrollbar.h>

Canvas Canvas_1, Canvas_2, Canvas_3, Canvas_4;
Pixwin *Pixwin_1, *Pixwin_2, *Pixwin_3, *Pixwin_4;
Rect framerect;
Pixfont *font;

extern char * sprintf();
/*
 * font macros:
 *	font_offset(font) gives the vertical distance between
 *			  the font origin and the top left corner
 *			  of the bounding box of the string displayed
 *			  (see Text Facilities for Pixrects in the
 *			  Pixrect Reference Manual)
 *	font_height(font) gives the height of the font
 */

#define font_offset(font)	(-font->pf_char['n'].pc_home.y)
#define font_height(font)	(font->pf_defaultsize.y)

/*
 * SunView-dependent size definitions
 */

#define LEFT_MARGIN	5		/* margin on left side of frame */
#define RIGHT_MARGIN	5		/* margin on right side of frame */
#define BOTTOM_MARGIN	5		/* margin on bottom of frame */
#define SUBWINDOW_SPACING	5	/* space in between adjacent
					   subwindows */

/*
 * application-dependent size definitions
 */

#define CANVAS_1_WIDTH	320		/* width in pixels of canvas 1 */
#define CANVAS_1_HEIGHT	160		/* height in pixels of canvas 1 */
#define CANVAS_3_COLUMNS	30	/* width in characters of canvas 3 */


main(argc, argv)
int argc;
char **argv;
{
	Frame frame;
	static Notify_value catch_resize();
	static void draw_canvas_1(), draw_canvas_3();

	/*
	 * create the frame and subwindows, and open the font
	 * no size attributes are given yet
	 */

	frame = window_create(NULL, FRAME,
			FRAME_ARGS, argc, argv,
			WIN_ERROR_MSG, "Can't create tool frame",
			FRAME_LABEL, "Resize Demo",
			0);
	Canvas_1 = window_create(frame, CANVAS,
			CANVAS_RESIZE_PROC, draw_canvas_1,
			0);
	Canvas_2 = window_create(frame, CANVAS,
			0);
	Canvas_3 = window_create(frame, CANVAS,
			WIN_VERTICAL_SCROLLBAR, scrollbar_create(
				SCROLL_PLACEMENT,	SCROLL_EAST,
				0),
			CANVAS_RESIZE_PROC, draw_canvas_3,
			0);
	Canvas_4 = window_create(frame, CANVAS,
			0);
	Pixwin_1 = canvas_pixwin(Canvas_1);
	Pixwin_2 = canvas_pixwin(Canvas_2);
	Pixwin_3 = canvas_pixwin(Canvas_3);
	Pixwin_4 = canvas_pixwin(Canvas_4);
	font = pf_default();

	/*
	 * now that the frame and font sizes are known, set the initial
	 * subwindow sizes
	 */

	resize(frame);

	/*
	 * insert an interposer so that whenever the window changes
	 * size we will know about it and handle it ourselves
	 */

	(void) notify_interpose_event_func(frame, catch_resize, NOTIFY_SAFE);

	/*
	 * start execution
	 */

	window_main_loop(frame);
	exit(0);
	/* NOTREACHED */
}

/*
 * catch_resize
 *
 * interposed function which checks all input events passed to the frame
 * for resize events; if it finds one,  resize() is called to refit
 * the subwindows; checking is done AFTER the frame processes the
 * event because if the frame changes its size due to this event (because
 * the window has been opened or closed for instance) we want to fit
 * the subwindows to the new size
 */

static Notify_value
catch_resize(frame, event, arg, type)
    Frame frame;
    Event *event;
    Notify_arg arg;
    Notify_event_type type;
{
	Notify_value value;

	value = notify_next_event_func(frame, event, arg, type);
	if (event_action(event) == WIN_RESIZE)
		resize(frame);
	return(value);
}

/*
 * resize
 *
 * fit the subwindows of the frame to its new size
 */

resize(frame)
    Frame frame;
{
	Rect *r;
	int canvas_3_width;	/* the width in pixels of canvas 3 */
	int stripeheight;	/* the height of the frame's name stripe */

	/* if the window is iconic, don't do anything */

	if ((int)window_get(frame, FRAME_CLOSED))
		return;

	/* find out our new size parameters */

	r = (Rect *) window_get(frame, WIN_RECT);
	framerect = *r;
	stripeheight = (int) window_get(frame, WIN_TOP_MARGIN);

	canvas_3_width = CANVAS_3_COLUMNS * font->pf_defaultsize.x
		+ (int) scrollbar_get(SCROLLBAR, SCROLL_THICKNESS);
	window_set(Canvas_2,
		WIN_X,		0,
		WIN_Y,		0,
		WIN_WIDTH,	framerect.r_width - canvas_3_width
				- LEFT_MARGIN - SUBWINDOW_SPACING
				- RIGHT_MARGIN,
		WIN_HEIGHT,	framerect.r_height - CANVAS_1_HEIGHT
				- stripeheight - SUBWINDOW_SPACING -
				BOTTOM_MARGIN,
		0);
	window_set(Canvas_1,
		WIN_X,		0,
		WIN_Y,		framerect.r_height - CANVAS_1_HEIGHT -
				SUBWINDOW_SPACING - stripeheight,
		WIN_WIDTH,	CANVAS_1_WIDTH,
		WIN_HEIGHT,	CANVAS_1_HEIGHT,
		0);
	window_set(Canvas_4,
		WIN_X,		CANVAS_1_WIDTH + SUBWINDOW_SPACING,
		WIN_Y,		framerect.r_height - CANVAS_1_HEIGHT
				- SUBWINDOW_SPACING - stripeheight,
		WIN_WIDTH,	framerect.r_width - canvas_3_width
				- CANVAS_1_WIDTH - LEFT_MARGIN
				- 2 * SUBWINDOW_SPACING - RIGHT_MARGIN, 
		WIN_HEIGHT,	CANVAS_1_HEIGHT,
		0);
	window_set(Canvas_3,
		WIN_X,		framerect.r_width - canvas_3_width
				- LEFT_MARGIN - SUBWINDOW_SPACING,
		WIN_Y,		0,
		WIN_WIDTH,	canvas_3_width,
		WIN_HEIGHT,	framerect.r_height - stripeheight
				- BOTTOM_MARGIN,
		0);
}

/*
 * draw_canvas_1
 * draw_canvas_3
 *
 * draw simple messages in the canvases
 */

static void
draw_canvas_1()
{
	char buf[64];

	sprintf(buf, "%d by %d pixels",
					CANVAS_1_WIDTH, CANVAS_1_HEIGHT);
	pw_text(Pixwin_1, 5, font_offset(font), PIX_SRC, font, 
					"This subwindow is always ");
	pw_text(Pixwin_1, 5, font_offset(font) + font_height(font),
					PIX_SRC, font, buf);
}

static void
draw_canvas_3()
{
	char buf[64];

	sprintf(buf, "%d characters wide",
					CANVAS_3_COLUMNS);
	pw_text(Pixwin_3, 5, font_offset(font), PIX_SRC, font,
					"This subwindow is always ");
	pw_text(Pixwin_3, 5, font_offset(font) + font_height(font), PIX_SRC,
					font, buf);
}
