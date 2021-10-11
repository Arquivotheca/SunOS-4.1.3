
#ifndef lint
static char     sccsid[] = "@(#)dialtest.c 1.1 92/07/30 Copyright 1988 Sun Micro";
#endif

 
/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <fcntl.h>


#include <suntool/sunview.h>
#include <suntool/panel.h>
#include <suntool/canvas.h>


#include <math.h>

/* make the icon for the tool */
short tool_image[256] =
{
#include "dial_icon.h"
};
DEFINE_ICON_FROM_IMAGE(tool_icon, tool_image);


#define NDIALS 8

/* converts dial units (64th of degrees) to radians */
#define DIAL_TO_RADIANS(n) \
	((float)(n) * 2.0 * M_PI) / (float) DIAL_UNITS_PER_CYCLE

/* these are defined in <sundev/dbio.h>, but since that may not be installed */
/* they are also defined here */

#define DIAL_DEVID		0x7B
#define	event_is_dial(event)	(vuid_in_range(DIAL_DEVID, event->ie_code))
#define DIAL_NUMBER(event_code)	(event_code & 0xFF)
#define DIAL_UNITS_PER_CYCLE (64*360)

#define BLACK 0
#define WHITE 255

Window frame;
Window panel;
Window canvas;

Pixwin *dial_pixwin;

void dial_event_proc(), draw_dials();
extern void diag_notify(), dump_notify();

int win_fd;

main(argc, argv)
	int argc;
	char **argv;
{
	int db_fd;

	dial_reset();	/* initialize the dial data */

	/* make the sunview objects */
	frame = window_create(NULL, FRAME,
		FRAME_ARGS, argc, argv,
		FRAME_LABEL, "Dialtool",
		FRAME_ICON, &tool_icon,
		0);

	panel = window_create(frame, PANEL, 0);
	
	panel_create_item(panel, PANEL_BUTTON, 
		PANEL_LABEL_IMAGE, panel_button_image(panel, "Diagnostics", 5, 0),
		PANEL_NOTIFY_PROC, diag_notify,
		0);

	panel_create_item(panel, PANEL_BUTTON, 
		PANEL_LABEL_IMAGE, panel_button_image(panel, "RAM dump", 5, 0),
		PANEL_NOTIFY_PROC, dump_notify,
		0);

	window_fit_height(panel);

	win_fd = (int)window_get(frame, WIN_FD);		 

	if ((db_fd = open("/dev/dialbox", O_RDWR)) == -1)  {
		perror("Open failed for /dev/dialbox:");
		exit(1);
	}

	win_set_input_device(win_fd, db_fd, "/dev/dialbox");

	close(db_fd); /* window system now has its own copy */

	canvas = window_create(frame, CANVAS,
		CANVAS_WIDTH, 256,
		CANVAS_HEIGHT, 512,
		WIN_HEIGHT, 512,
		WIN_WIDTH, 256,
		CANVAS_RETAINED, FALSE,
		CANVAS_REPAINT_PROC, draw_dials,
		WIN_EVENT_PROC, dial_event_proc,
		0);

	dial_pixwin = canvas_pixwin(canvas);

	window_fit(canvas);
	window_fit(frame);
	
	draw_dials();
	window_main_loop(frame);
}

static float angle[9];

dial_reset()
{
	int i;
	for(i = 0; i < NDIALS; i++)  angle[i] = M_PI;
}

/*
	called by an event on the dial box
*/
void
dial_event_proc(window, event, arg)
	Window window;
	Event *event;
	caddr_t arg;
{
	int i;
	static int delta;

	/* buttonbox may be multiplexed on this line */
	if (event_is_dial(event)) {
		i = DIAL_NUMBER(event->ie_code);
		delta = (int)window_get(canvas, WIN_EVENT_STATE, event->ie_code);
		draw_dial(i, angle[i], BLACK);	/* unmark the old line */
		angle[i] += DIAL_TO_RADIANS(delta);
		draw_dial(i, angle[i], WHITE);	/* remark the new line */
	}
}

/*
	circle centers for an eight dial box
*/
struct pr_pos origins_8[8] =
{
	{64,  64},	
	{64, 192},
	{64, 320},
	{64, 448},
	{192,  64},	
	{192, 192},
	{192, 320},
	{192, 448},
};

/*
	circle centers for an nine dial box
*/
struct pr_pos origins_9[9] =
{
	{64,  64},	{192, 64}, {320, 64},
	{64, 192},	{192, 192}, {320, 192},
	{64, 320},	{192, 320}, {320, 320}
};

struct pr_pos *dial_origin = origins_8;	/* we have an 8 dial box */

void
draw_dials()
{
	int i;
	for(i = 0; i < 8; i++) {
		pw_circle(dial_pixwin, dial_origin[i].x, dial_origin[i].y, 48, WHITE);
		draw_dial(i, angle[i], WHITE);
	}
}

draw_dial(dialnum, angle, color)
	int dialnum;
	float angle;
	int color;
{
	struct pr_pos to;

	to.x = dial_origin[dialnum].x + (int)(sin(-angle) * 46.0);
	to.y = dial_origin[dialnum].y + (int)(cos(-angle) * 46.0);
	pw_vector(dial_pixwin, dial_origin[dialnum].x, dial_origin[dialnum].y,
		to.x, to.y, PIX_SRC, color);
}

