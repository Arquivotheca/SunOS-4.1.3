#ifndef lint
static char     sccsid[] = "@(#)buttontest.c 1.1 92/07/30 Copyright 1988, 1990 Sun Micro";

#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <fcntl.h>
#include <sundev/dbio.h>
#include <suntool/sunview.h>
#include <suntool/panel.h>
#include <suntool/canvas.h>
#include <suntool/alert.h>
#include <math.h>
#include <errno.h>

extern int errno;

/*
	make the icon
*/
short           tool_image[256] =
{
#include "button_icon.h"
};

#include "buttons.h"

/* Local definitions */
DEFINE_ICON_FROM_IMAGE (tool_icon, tool_image);
#define BUTTON_DEVID		0x7A
#define NBUTTON			32
#define BLACK			0
#define WHITE			255

/* Routines */
void            button_on (), button_off ();
void            init_buttons ();
void            draw_buttons ();
void            no_proc (), button_event_proc ();
extern void diag_notify();
extern void generic_alert();

/*Global variables */
Window          frame,
                panel,
                canvas;
Pixwin         *button_pixwin;
int             win_fd,
                bb_fd;
long int        led_mask;


/*Processing begins here */
main (argc, argv)
    int             argc;
    char          **argv;
{

	/* make the sunview objects */
    frame = window_create (NULL, FRAME,
		FRAME_ARGS, argc, argv,
		FRAME_LABEL, "Buttontool",
		FRAME_ICON, &tool_icon,
		0);

    panel = window_create (frame, PANEL, 0);

    panel_create_item (panel, PANEL_BUTTON, 
		PANEL_LABEL_IMAGE, panel_button_image (panel, "Reset", 5, 0),
		PANEL_NOTIFY_PROC, init_buttons,
		0);

	panel_create_item(panel, PANEL_BUTTON,
		PANEL_LABEL_IMAGE, panel_button_image(panel, "Diagnostics", 5, 0),
		PANEL_NOTIFY_PROC, diag_notify,
		0);

    window_fit_height (panel);

    win_fd = (int) window_get (frame, WIN_FD);

    if ((bb_fd = open ("/dev/dialbox", O_RDWR)) == -1) {
		perror ("Open failed for /dev/buttonbox:");
		exit (1);
    }

    win_set_input_device (win_fd, bb_fd, "/dev/dialbox");

    canvas = window_create (frame, CANVAS,
		CANVAS_WIDTH, 448,
		CANVAS_HEIGHT, 448,
		WIN_HEIGHT, 448,
		WIN_WIDTH, 448,
		CANVAS_RETAINED, FALSE,
		CANVAS_REPAINT_PROC, draw_buttons,
		WIN_EVENT_PROC, button_event_proc,
		0);

    button_pixwin = canvas_pixwin (canvas);

    window_fit (canvas);
    window_fit (frame);

    draw_buttons ();
    init_buttons();
    window_main_loop (frame);
}

void
button_event_proc (window, event, arg)
    Window          window;
    Event          *event;
    caddr_t         arg;
{
    int             button;
    int             down;

	/* since the dialbox and button box may be multiplexed on one line,
		make sure the event is for us */
    if (event_is_32_button (event)) {
		down = (int) window_get (window, WIN_EVENT_STATE, event->ie_code);
		button = BUTTON_MAP((event->ie_code - (BUTTON_DEVID << 8)));
		if (down) {
			button_on (button);
		}
		else {
			button_off (button);
		}
    }
}

/* button locations in pixels on the screen */
struct pr_pos   origins_32[33] =
{
    {0, 0},
    {104, 40},
    {168, 40},
    {232, 40},
    {296, 40},
    {40, 104},
    {104, 104},
    {168, 104},
    {232, 104},
    {296, 104},
    {360, 104},
    {40, 168},
    {104, 168},
    {168, 168},
    {232, 168},
    {296, 168},
    {360, 168},
    {40, 232},
    {104, 232},
    {168, 232},
    {232, 232},
    {296, 232},
    {360, 232},
    {40, 296},
    {104, 296},
    {168, 296},
    {232, 296},
    {296, 296},
    {360, 296},
    {104, 360},
    {168, 360},
    {232, 360},
    {296, 360},
};


struct pr_pos  *but_origin = origins_32;

void
draw_buttons ()
{
    int             i;

    for (i = 1; i < NBUTTON + 1; i++) {
		pw_rop (button_pixwin, but_origin[i].x, but_origin[i].y, 64, 64,
			PIX_SRC, &off_off_icon, 0, 0);
    }
}
void
init_buttons()
{
    led_mask = 0;

    if (ioctl (bb_fd, DBIOBUTLITE, &led_mask) == -1) {
		if(errno == ENOTTY) generic_alert("dbconfig has not been run ", "Continue?");
		else perror("Ioctl(DBIOBUTLITE)");
    }	
    draw_buttons();
}

void
button_on (but_num)
    int             but_num;
{
    int             x1, y1, x2, y2;

    led_mask |= LED_MAP(but_num);

    if (ioctl (bb_fd, DBIOBUTLITE, &led_mask) == -1) {
		if(errno == ENOTTY) generic_alert("dbconfig has not been run ", "Continue?");
		else perror("Ioctl(DBIOBUTLITE)");
    }
    pw_rop (button_pixwin, but_origin[but_num].x, but_origin[but_num].y,
		64, 64, PIX_SRC, &on_on_icon, 0, 0);
}

void
button_off (but_num)
    int             but_num;
{
    int             x1, y1, x2, y2;

    led_mask &= (~LED_MAP(but_num));

    if (ioctl (bb_fd, DBIOBUTLITE, &led_mask) == -1) {
		if(errno == ENOTTY) generic_alert("dbconfig has not been run ", "Continue?");
		else perror("Ioctl(DBIOBUTLITE)");
    }
    pw_rop (button_pixwin, but_origin[but_num].x, but_origin[but_num].y,
		64, 64, PIX_SRC, &off_off_icon, 0, 0);
}

static void
generic_alert(msg1, msg2)
	char	*msg1;
	char	*msg2;
{
	int status;

	status = alert_prompt(frame, (Event *)NULL,
		ALERT_MESSAGE_STRINGS,
			msg1,
			msg2,
			0,
		ALERT_BUTTON_YES, "Continue",
		ALERT_BUTTON_NO, "Quit",
		0);

	if (status == ALERT_NO) exit(0);
}

