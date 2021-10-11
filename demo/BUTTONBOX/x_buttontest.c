#ifndef lint
static char     sccsid[] = "@(#)x_buttontest.c 1.1 92/07/30 Copyright 1988, 1990 Sun Micro";

#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sundev/dbio.h>
#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/canvas.h>
#include <xview/notice.h>
#include <xview/xv_error.h>
#include <xview/svrimage.h>
#include <math.h>
#include <errno.h>

extern int errno;

short button_bits[] =
{
#include "button_icon.h"
};

short button_mask_bits[] =
{
#include "button_mask_icon.h"
};
Icon    button_icon;
Server_image icon_image, mask_image;

#include "buttons.h"

#define BUTTON_DEVID		0x7A
#define NBUTTON			32
#define BLACK			0
#define WHITE			255

/* Routines */
void			button_on(), button_off();
void			init_buttons();
void			draw_buttons();
void			no_proc(), button_event_proc();
extern void diag_notify();
extern void generic_alert();

/*Global variables */

Xv_Window	frame = (Xv_Window) 0;
Xv_Window	panel;
Xv_Window	canvas;

Pixwin		 *button_pixwin;
int	win_fd;
int	bb_fd;
long int led_mask;

Display *dpy;


/*Processing begins here */

/*
	added for reading the button box
*/


Notify_value
read_bb(cli, fd)
	Notify_client cli;
	int fd;
{
	int byt_cnt, red;
	char buf[16];

	if(ioctl(fd, FIONREAD, &byt_cnt) == -1 || byt_cnt == 0) return;
	if((red = read(fd, buf, sizeof(buf))) == 16) button_event_proc(0, buf, 0);
	else fprintf(stderr, "read %d bytes\n", red);
}

button_error_proc(obj, avlist)
	Xv_object obj;
	Attr_attribute avlist[ATTR_STANDARD_SIZE];
{
	Attr_avlist attrs;
	Error_layer layer;
	Error_severity sev;
	int n = 0;
	char buf[7][64];
	char *bufp[7];

	for(n = 0; n < 7; n++) bufp[n] = &buf[n][0];

	n = 0;

	for(attrs = avlist; *attrs && n < 5; attrs = attr_next(attrs)) {
		switch((int) attrs[0]) {
		case ERROR_BAD_ATTR:
			/* fprintf(stderr, "bad attribute: %s", attr_name(attrs[1])); */
			sprintf(buf[n++], "bad attribute: %s", attr_name(attrs[1]));
			break;
		case ERROR_BAD_VALUE:
			/* fprintf(stderr, "bad value: 0x%x for attribute: %s",
				attrs[1], attr_name(attrs[2])); */
			sprintf(buf[n++], "bad value: 0x%x for attribute: %s",
				attrs[1], attr_name(attrs[2]));
			break;
		case ERROR_STRING:
			/* fprintf(stderr, "%s", (char*) (attrs[1])); */
			sprintf(buf[n++], "%s", (char*) (attrs[1]));
			break;
		case ERROR_SEVERITY:
			sev = attrs[1];
			break;
		case ERROR_SERVER_ERROR:
			/* fprintf(stderr, "server error: %s", (char*) (attrs[1]));*/
			sprintf(buf[n++], "server error: %s", (char*) (attrs[1]));
			break;
		case ERROR_PKG:
			/* fprintf(stderr, "package: %s", (char*) (attrs[1]));*/
			sprintf(buf[n++], "package: %s", (char*) (attrs[1]));
			break;
		case ERROR_LAYER:
			/* fprintf(stderr, "layer: %s", (char*) (attrs[1]));*/
			sprintf(buf[n++], "layer: %s", (char*) (attrs[1]));
			break;
		case ERROR_CANNOT_SET:
			/* fprintf(stderr, "cannot set error: %s", (char*) (attrs[1]));*/
			sprintf(buf[n++], "cannot set error: %s", (char*) (attrs[1]));
			break;
		case ERROR_CREATE_ONLY:
			/* fprintf(stderr, "create only error: %s", (char*) (attrs[1]));*/
			sprintf(buf[n++], "create only error: %s", (char*) (attrs[1]));
			break;
		case ERROR_CANNOT_GET:
			/* fprintf(stderr, "cannot get error: %s", (char*) (attrs[1]));*/
			sprintf(buf[n++], "cannot get error: %s", (char*) (attrs[1]));
			break;
		case ERROR_INVALID_OBJECT:
			/* fprintf(stderr, "invalid object: %s", (char*) (attrs[1])); */
			sprintf(buf[n++], "invalid object: %s", (char*) (attrs[1]));
			break;
		}
	}
	strcpy(buf[n++], "Core dump?");
	bufp[n] = (char *) 0;
	if(sev == ERROR_NON_RECOVERABLE) {
		if(frame != (Xv_Window) 0 && notice_prompt(frame, (Event *) NULL,
			NOTICE_MESSAGE_STRINGS_ARRAY_PTR, bufp,
			NOTICE_BUTTON_YES, "yes",
			NOTICE_BUTTON_NO, "no",
			0) == NOTICE_YES) abort();
		exit(1);
	}
	
	return(XV_OK);
}


main(argc, argv)
	int argc;
	char **argv;
{
	int i;

	/* added for button box IO */


	Notify_client client = (Notify_client) 0x1234;
    
	xv_init(
		XV_INIT_ARGC_PTR_ARGV, &argc, argv,
		XV_ERROR_PROC, button_error_proc,
		0);

    icon_image = xv_create(0, SERVER_IMAGE,
		SERVER_IMAGE_BITS, button_bits,
		SERVER_IMAGE_DEPTH, 1,
		XV_WIDTH, 64,
		XV_HEIGHT, 64,
		0);

    mask_image = xv_create(0, SERVER_IMAGE,
		SERVER_IMAGE_BITS, button_mask_bits,
		SERVER_IMAGE_DEPTH, 1,
		XV_WIDTH, 64,
		XV_HEIGHT, 64,
		0);

    button_icon = xv_create(NULL, ICON,
		ICON_IMAGE, icon_image,
		ICON_MASK_IMAGE, mask_image,
		ICON_TRANSPARENT, TRUE,
		0);

	frame = xv_create(NULL, FRAME,
		FRAME_ARGS, argc, argv,
		XV_LABEL, "Buttontool",
		FRAME_ICON, button_icon,
		0);

	dpy = (Display *) xv_get(frame, XV_DISPLAY);

	panel = xv_create(frame, PANEL, 0);

	xv_create(panel, PANEL_BUTTON, 
		PANEL_LABEL_STRING, "Reset",
		PANEL_NOTIFY_PROC, init_buttons,
		XV_X, xv_col(panel, 0),
		XV_Y, xv_row(panel, 0),
		0);

	xv_create(panel, PANEL_BUTTON, 
		PANEL_LABEL_STRING, "Diagnostics",
		PANEL_NOTIFY_PROC, diag_notify,
		XV_X, xv_col(panel, 43),
		XV_Y, xv_row(panel, 0),
		0);

	window_fit_height(panel);

	if((bb_fd = open("/dev/dialbox", O_RDWR)) == -1) {
		perror("Open failed for /dev/buttonbox:");
		exit(1);
	}
	
	notify_set_input_func(client, read_bb, bb_fd);
	
	canvas = xv_create(frame, CANVAS,
		CANVAS_WIDTH, 448,
		CANVAS_HEIGHT, 448,
		XV_HEIGHT, 448,
		XV_WIDTH, 448,
		CANVAS_RETAINED, FALSE,
		CANVAS_REPAINT_PROC, draw_buttons,
		WIN_EVENT_PROC, button_event_proc,
		0);

	button_pixwin = canvas_pixwin(canvas);
	
	draw_buttons();

	window_fit_width(panel);

	window_fit_height(frame);
	window_fit_width(frame);
	window_fit(frame);

	init_buttons();
	window_main_loop(frame);
}

void
button_event_proc(window, event, arg)
	Xv_Window window;
	Event *event;
	caddr_t arg;
{
	int button, down;

#ifdef BUTTONTEST_DEBUG
	printf("bep: c %04x f %04x m %04x x %04x y %04x\n",
        event -> ie_code, event -> ie_flags, event -> ie_shiftmask,
        event -> ie_locx, event -> ie_locy);
#endif BUTTONTEST_DEBUG

	if(event_is_32_button(event)) {
		/*
		down = (int) xv_get(window, WIN_EVENT_STATE, event->ie_code);
		*/
		down = (int) event -> ie_locx & 1;	/* don't do this at home */
		button = BUTTON_MAP((event->ie_code - (BUTTON_DEVID << 8)));

#ifdef BUTTONTEST_DEBUG
		printf("\td: %d b: %d\n", down, button);
#endif BUTTONTEST_DEBUG

		if(down) {
			button_on(button);
		}
		else {
			button_off(button);
		}
	}
}


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
draw_buttons()
{
	int	i;

	for(i = 1; i < NBUTTON + 1; i++) {
		xv_rop(button_pixwin, but_origin[i].x, but_origin[i].y, 64, 64,
			PIX_SRC, &off_off_icon, 0, 0);
	}
}
void
init_buttons()
{
	led_mask = 0;

	if(ioctl(bb_fd, DBIOBUTLITE, &led_mask) == -1) {
		if(errno == ENOTTY) generic_alert("dbconfig has not been run ", "Continue?");
		else perror("Ioctl(DBIOBUTLITE)");
	}	

	draw_buttons();
}

void
button_on(but_num)
	int but_num;
{
	led_mask |= LED_MAP(but_num);

	if(ioctl(bb_fd, DBIOBUTLITE, &led_mask) == -1) {
		if(errno == ENOTTY) generic_alert("dbconfig has not been run ", "Continue?");
		else perror("Ioctl(DBIOBUTLITE)");
	}

	xv_rop(button_pixwin,
		but_origin[but_num].x, but_origin[but_num].y,
		64, 64,
		PIX_SRC, 
		&on_on_icon, 0,
		0);

	XFlush(dpy);
}

void
button_off(but_num)
	int but_num;
{
	int x1, y1, x2, y2;

	led_mask &= (~LED_MAP(but_num));
	if(ioctl(bb_fd, DBIOBUTLITE, &led_mask) == -1) {
		perror("Ioctl(DBIOBUTLITE)");
	}
	xv_rop(button_pixwin, but_origin[but_num].x,
		but_origin[but_num].y, 64, 64,
		PIX_SRC, &off_off_icon, 0, 0);
	XFlush(dpy);
}

static void
generic_alert(msg1, msg2)
	char	*msg1;
	char	*msg2;
{
	int status;

	status = notice_prompt(frame, (Event *)NULL,
		NOTICE_MESSAGE_STRINGS,
			msg1,
			msg2, 
			0,
		NOTICE_BUTTON_YES, "Continue",
		NOTICE_BUTTON_NO, "Quit",
		0);
	
	if (status == NOTICE_NO) exit(0);
}

